/**
 * ============================================================
 *  Smart Fire Fighting System (SFFS) — ESP32 Firmware
 * ============================================================
 * Target: CLASSIC ESP32 WROOM-32 (38-pin)
 * Subsystems:
 *   - Sensor telemetry  (MQ-2/5/6/7, DHT22, IR flame, HC-SR04)
 *   - EMA noise filtering
 *   - WiFi auto-reconnect
 *   - Non-blocking MQTT (JSON publish + AI command subscribe)
 *   - PCA9685 servo driver + pump/LED actuators
 *   - Autonomous failsafe state machine with pump dry-run protection
 *
 * The loop never blocks: every subsystem is millis()-gated. Emergency
 * actuators are EDGE-triggered (applied once on entry) to protect the
 * servos and the power rail from brown-outs.
 */

#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <Wire.h>
#include "config.h"
#include "sensors.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "actuators.h"
#include "failsafe.h"

// ==========================================
// Global Module Instances
// ==========================================
SensorsManager     sensors;
WifiManager        wifi(WIFI_SSID, WIFI_PASSWORD, WIFI_RETRY_DELAY);
MqttHandler        mqttHandler(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);
ActuatorController actuators;
FailsafeManager    failsafe(FAILSAFE_CRITICAL_TIMEOUT_MS, FAILSAFE_RECOVERY_HOLD_MS);

// ==========================================
// Timing & Edge-Detection State
// ==========================================
unsigned long lastSensorPublish  = 0;
unsigned long lastHeartbeat       = 0;
unsigned long lastTelemetryPrint  = 0;

bool manualAlarmActive = false;   // a manual button armed the current latch (→ manual_trigger)
bool btn1WasPressed    = false;
bool btn2WasPressed    = false;

// UNIFIED 5-second non-blocking FIRE latch. ANY active trigger — manual button,
// local gas/smoke, local FLAME, or AI camera FIRE — holds the full-suppression set.
// The 5s window (ALARM_LATCH_MS) REFRESHES while a trigger is sustained and restarts
// on a fresh press; exactly 5s after the LAST active trigger clears, the node
// auto-returns to SAFE (anti-flood / anti-burnout, and frees the alarm from hold-time).
bool          fireAlarmActive  = false;
unsigned long fireAlarmStart   = 0;
bool          aiFireActive      = false;  // latest AI command == FIRE (while online + NORMAL)

// Failsafe edge latches (apply emergency / safe actuators ONCE)
bool standaloneApplied = false;
bool recoveryApplied   = false;

// B6: pump dry-run protection
bool pumpsCutForWater  = false;
bool refillAlertActive = false;

// ==========================================
// Prototypes
// ==========================================
void processAiCommands();
void processManualButtons();
void updateFireAlarm();
void processFailsafe();
void managePumpWaterSafety();
void publishSensorData();
void publishHeartbeat();
void printTelemetry();

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(DEBUG_BAUD);
    delay(1000);
    Serial.println("\n =========================================");
    Serial.println(" SFFS — Smart Fire Fighting System v2.1");
    Serial.println(" Classic ESP32 (38-pin) Integrated Firmware");
    Serial.println(" =========================================\n");

    pinMode(BOOT_LED_PIN, OUTPUT);
    digitalWrite(BOOT_LED_PIN, HIGH);  // LED ON during boot

    // Hardware I2C bus (dedicated to the PCA9685 @0x40). Begin it here; the PCA9685
    // is brought up in actuators.begin().
    Wire.begin(I2C_SDA, I2C_SCL);

    sensors.begin();
    actuators.begin(BUZZER_PIN,
                    PUMP1_PIN, PUMP2_PIN,
                    LED_GREEN_PIN, LED_ORANGE_PIN, LED_RED_PIN);
    failsafe.begin();

    wifi.begin();                       // bounded blocking (max 10s) at boot only

    // ALWAYS configure the MQTT client (setServer/setCallback/setSocketTimeout
    // all live in begin()). It must run even if WiFi isn't up yet, otherwise the
    // client has no broker address and no command callback, and update()'s later
    // reconnects can never succeed. begin() does one bounded connect attempt;
    // if WiFi is still down it simply fails and update() retries with backoff.
    mqttHandler.begin();
    if (!wifi.isConnected()) {
        Serial.println("[System] WiFi not up at boot — MQTT will auto-connect once WiFi is ready.");
    }

    digitalWrite(BOOT_LED_PIN, LOW);
    Serial.println("\n[System] ✅ Subsystems initialized. Entering main loop.\n");
}

// ==========================================
// MAIN LOOP (fully non-blocking)
// ==========================================
void loop() {
    // 1. Update module state machines (none of these block)
    sensors.update();
    wifi.update();
    mqttHandler.update();   // B5: non-blocking, WiFi-gated reconnect w/ backoff
    actuators.updateBuzzer();   // tick the non-blocking buzzer pattern (intermittent/continuous)

    // 2. Composite local danger (only meaningful after warm-up)
    bool sensorDanger = false;
    if (sensors.isWarmedUp()) {
        sensorDanger = sensors.isGasDanger()
                    || sensors.isFlameDetected()
                    || (sensors.getTemperature() > TEMP_THRESHOLD);
    }

    // 3. Alarm trigger inputs
    processManualButtons();    // manual buttons → MQTT manual_trigger + latch input
    processAiCommands();       // AI FIRE → latch input; AI SAFE/SENSOR_ALERT applied live

    // 4. Unified 5s FIRE latch: ANY sustained trigger (manual / gas / FLAME / AI) holds
    //    full suppression; auto-returns to SAFE 5s after the LAST trigger clears.
    updateFireAlarm();

    // 5. Pump dry-run protection (every loop — Pump 3 runs continuously)
    managePumpWaterSafety();

    // 6. Failsafe FSM + autonomous handling
    failsafe.update(wifi.isConnected(), mqttHandler.isConnected(), sensorDanger);
    processFailsafe();

    // 7. Periodic publishes
    if (millis() - lastSensorPublish >= SENSOR_PUBLISH_INTERVAL) {
        lastSensorPublish = millis();
        publishSensorData();
    }
    if (millis() - lastHeartbeat >= HEARTBEAT_INTERVAL) {
        lastHeartbeat = millis();
        publishHeartbeat();
    }

    // 8. Serial dashboard for the VS Code monitor
    if (millis() - lastTelemetryPrint >= TELEMETRY_PRINT_INTERVAL) {
        lastTelemetryPrint = millis();
        printTelemetry();
    }
}

// ==========================================
// AI Command Processing
// ==========================================
void processAiCommands() {
    if (!mqttHandler.hasNewCommand()) return;

    ActuatorCommand cmd = mqttHandler.getLatestCommand();
    if (!cmd.isValid) return;

    // In failsafe (offline) modes the ESP32 is autonomous — ignore AI entirely and
    // drop the FIRE level so a stale command can't hold the latch.
    if (failsafe.getState() != FailsafeManager::STATE_NORMAL) {
        aiFireActive = false;
        return;
    }

    // Track AI FIRE as a LEVEL — it feeds the unified latch (a sustained AI FIRE holds
    // suppression; updateFireAlarm() applies the 5s auto-shutoff once it clears).
    aiFireActive = (strcmp(cmd.state, "FIRE") == 0);

    // Non-FIRE AI states (SAFE / SENSOR_ALERT) are display/warning states: apply them
    // live, but never override an active FIRE latch or a held manual button.
    if (!aiFireActive && !fireAlarmActive && !manualAlarmActive) {
        actuators.applyCommands(cmd.gasValve, cmd.doors, cmd.pump1, cmd.pump2);
        actuators.applyState(cmd.state);   // LED color + buzzer pattern from state
        Serial.print("[AI CMD] Executed: State=");
        Serial.print(cmd.state);
        Serial.print(" Conf=");
        Serial.println(cmd.confidence, 2);
    }
}

// ==========================================
// Unified 5-Second FIRE Alarm Latch (non-blocking, level-based)
// ==========================================
// Fuses ALL emergency triggers and holds the FULL suppression set (valve CLOSE,
// doors OPEN, both pumps ON, red LED, continuous buzzer) for ALARM_LATCH_MS:
//   • Manual buttons (GPIO 16/17)      — held or pulsed
//   • Local GAS/SMOKE (MQ, warm-up gated)
//   • Local FLAME (IR, instant)
//   • AI camera FIRE command
// The 5s window REFRESHES every loop a trigger is active, so a SUSTAINED trigger
// (held button / persistent gas / continuous AI FIRE) keeps suppression on, and a
// fresh press restarts it. Exactly 5s after the LAST trigger clears → auto-SAFE.
// While the failsafe is driving actuators (offline STANDALONE), this stands down so
// the two mechanisms never fight.
void updateFireAlarm() {
    if (failsafe.shouldActAutonomously()) {
        fireAlarmActive = false;
        aiFireActive    = false;     // failsafe owns the actuators now
        return;
    }
    if (!mqttHandler.isConnected()) {
        aiFireActive = false;        // no live AI → a stale FIRE must not hold the latch
    }

    // Level-based trigger fusion — ANY active source keeps the window open.
    bool manualTrig = manualAlarmActive;                          // button(s) held
    bool gasTrig    = sensors.isWarmedUp() && sensors.isGasDanger();
    bool flameTrig  = sensors.isFlameDetected();                  // IR flame → FIRE (no warm-up)
    bool triggerActive = manualTrig || gasTrig || flameTrig || aiFireActive;

    unsigned long now = millis();

    if (triggerActive) {
        fireAlarmStart = now;                 // refresh while sustained / on each fresh press
        if (!fireAlarmActive) {
            fireAlarmActive = true;
            actuators.setEmergency();         // valve CLOSE, doors OPEN, BOTH pumps ON, red LED, continuous buzzer
            Serial.print("[Alarm] 🔥 FIRE latch ARMED (5s) — src:");
            Serial.print(manualTrig    ? "MANUAL " : "");
            Serial.print(gasTrig       ? "GAS "    : "");
            Serial.print(flameTrig     ? "FLAME "  : "");
            Serial.println(aiFireActive ? "AI"     : "");
        }
    } else if (fireAlarmActive && (now - fireAlarmStart >= ALARM_LATCH_MS)) {
        fireAlarmActive = false;
        actuators.setAllSafe();               // valve OPEN, doors CLOSE, pumps OFF, green ON, buzzer off
        publishSensorData();                  // tell the AI the node returned to SAFE
        Serial.println("[Alarm] ⏱️ 5s elapsed, no active trigger → auto-shutoff. Returned to SAFE.");
    }
}

// ==========================================
// Failsafe Autonomous Actions (EDGE-triggered)
// ==========================================
void processFailsafe() {
    FailsafeManager::FailsafeState st = failsafe.getState();

    if (failsafe.shouldActAutonomously()) {
        // Apply the emergency actuator set ONCE on entry — never hammer the
        // servos/relays every loop (that causes brown-outs and servo wear).
        if (!standaloneApplied) {
            actuators.setEmergency();   // valve CLOSE, doors OPEN, buzzer ON, pumps ON, red LED
            standaloneApplied = true;
            recoveryApplied   = false;
            pumpsCutForWater  = false;
            Serial.println("[Failsafe] STANDALONE actuators latched (edge).");
        }
        // Pump dry-run protection runs every loop (loop step 5) and covers the
        // standalone state too — no per-pump handling needed here.
    } else {
        standaloneApplied = false;
    }

    // Return to safe ONCE on recovery.
    if (st == FailsafeManager::STATE_RECOVERY) {
        if (!recoveryApplied) {
            actuators.setAllSafe();   // also sets green LED + silences buzzer
            recoveryApplied = true;
            Serial.println("[Failsafe] RECOVERY: actuators returned to safe (edge).");
        }
    } else if (st == FailsafeManager::STATE_NORMAL) {
        recoveryApplied = false;
    }
}

// ==========================================
// B6: Pump Dry-Run Protection — Pumps 1 & 2 (runs EVERY loop)
// ==========================================
// Both pumps are active-suppression pumps (ON only in EMERGENCY). If the tank drops
// below the cutoff while either is running, both are shut off; once refilled past the
// resume threshold they are re-enabled together — but ONLY while a fire emergency is
// still active. Hysteresis (PUMP_DRY_CUTOFF_PCT → WATER_REFILL_RESUME_PCT) prevents
// relay chatter around the threshold.
void managePumpWaterSafety() {
    float level    = sensors.getWaterLevelPct();
    bool  dry      = (level < PUMP_DRY_CUTOFF_PCT);
    bool  refilled = (level >= WATER_REFILL_RESUME_PCT);

    if (!pumpsCutForWater && dry &&
        (actuators.isPump1Active() || actuators.isPump2Active())) {
        // Empty tank while pumping → cut both to prevent dry-running.
        actuators.pump1Off();
        actuators.pump2Off();
        pumpsCutForWater  = true;
        refillAlertActive = true;
        Serial.print("[Safety] WATER LOW (");
        Serial.print(level, 1);
        Serial.println("%) → pumps OFF (dry-run prevention). REFILL ALERT raised.");
    } else if (pumpsCutForWater && refilled) {
        // Re-enable both ONLY if a fire emergency is still active.
        if (fireAlarmActive || failsafe.shouldActAutonomously()) {
            actuators.pump1On();
            actuators.pump2On();
            Serial.print("[Safety] Water restored (");
            Serial.print(level, 1);
            Serial.println("%) → pumps re-enabled.");
        }
        pumpsCutForWater  = false;
        refillAlertActive = false;
    }
}

// ==========================================
// MQTT Publishing
// ==========================================
void publishSensorData() {
    if (!mqttHandler.isConnected()) return;

    // Gas-alarm telemetry: if ANY MQ sensor (MQ2/MQ5/MQ6/MQ7) is over threshold,
    // surface it in the JSON's `meta.mode` field as "GAS_ALARM"; otherwise report
    // the normal failsafe state. (isGasDanger() already OR's all four channels and
    // respects the warm-up gate.)
    const char* mode = failsafe.getStateString();
    if (sensors.isWarmedUp() && sensors.isGasDanger()) {
        mode = "GAS_ALARM";
    }

    mqttHandler.publishSensorData(
        sensors.getGas1(), sensors.getGas2(),
        sensors.getGas3(), sensors.getGas4(),
        sensors.getTemperature(), sensors.getHumidity(),
        sensors.isFlameDetected(),
        sensors.getWaterLevelPct(), sensors.getWaterDistanceCm(),
        manualAlarmActive,                       // manual_trigger → AI forces FIRE
        wifi.getRSSI(), mode                      // meta.mode = "GAS_ALARM" when gas detected
    );
    lastSensorPublish = millis();   // keep the periodic cadence aligned after forced sends
}

void publishHeartbeat() {
    if (!mqttHandler.isConnected()) return;
    mqttHandler.publishHeartbeat();
}

// ==========================================
// Serial Telemetry Dashboard (every 2s)
// ==========================================
void printTelemetry() {
    Serial.println("────────────────────────────────────────");

    if (!sensors.isWarmedUp()) {
        Serial.print("⏳ MQ Warmup: ");
        Serial.print(sensors.getWarmupTimeRemaining());
        Serial.println("s remaining...");
    }

    Serial.print("💨 Gas  | MQ2: ");  Serial.print(sensors.getGas1(), 0);
    Serial.print(" | MQ5: ");          Serial.print(sensors.getGas2(), 0);
    Serial.print(" | MQ6: ");          Serial.print(sensors.getGas3(), 0);
    Serial.print(" | MQ7: ");          Serial.print(sensors.getGas4(), 0);
    Serial.println(sensors.isWarmedUp() && sensors.isGasDanger() ? "  🚨 GAS_ALARM" : "");

    Serial.print("🌡️ Env  | Temp: ");  Serial.print(sensors.getTemperature(), 1);
    Serial.print("°C | Hum: ");         Serial.print(sensors.getHumidity(), 1);
    Serial.println("%");

    Serial.print("🔥 Flame: ");         Serial.println(sensors.isFlameDetected() ? "DETECTED! 🚨" : "Clear");

    Serial.print("💧 Water | Level: "); Serial.print(sensors.getWaterLevelPct(), 1);
    Serial.print("% | Distance: ");      Serial.print(sensors.getWaterDistanceCm(), 1);
    Serial.print(" cm");
    if (refillAlertActive) Serial.print("  ⚠️ REFILL NEEDED (pumps cut)");
    Serial.println();

    Serial.print("🔘 Buttons | Room1: "); Serial.print(sensors.isButton1Pressed() ? "PRESSED" : "Released");
    Serial.print(" | Room2: ");            Serial.println(sensors.isButton2Pressed() ? "PRESSED" : "Released");

    Serial.print("📶 WiFi: ");
    if (wifi.isConnected()) {
        Serial.print("Connected (");
        Serial.print(wifi.getRSSI());
        Serial.print(" dBm)");
    } else {
        Serial.print("OFFLINE");
    }
    Serial.print(" | MQTT: ");
    Serial.println(mqttHandler.isConnected() ? "Connected" : "OFFLINE");

    Serial.print("🛡️ Mode: ");           Serial.print(failsafe.getStateString());
    Serial.print(" | Alarm: ");
    if (fireAlarmActive) {
        long remainMs = (long)ALARM_LATCH_MS - (long)(millis() - fireAlarmStart);
        if (remainMs < 0) remainMs = 0;
        Serial.print(manualAlarmActive ? "MANUAL " : "FIRE ");
        Serial.print(remainMs / 1000.0f, 1);
        Serial.println("s left");
    } else {
        Serial.println("—");
    }

    Serial.print("⚙️ Actuators | Valve: "); Serial.print(actuators.isGasValveOpen() ? "OPEN" : "CLOSED");
    Serial.print(" | Doors: ");              Serial.print(actuators.isDoorOpen() ? "OPEN" : "CLOSED");
    Serial.print(" | Buzzer: ");             Serial.print(actuators.isBuzzerActive() ? "ON 🔊" : "OFF");
    Serial.print(" | Pump1: ");              Serial.print(actuators.isPump1Active() ? "ON 💧" : "OFF");
    Serial.print(" | Pump2: ");              Serial.println(actuators.isPump2Active() ? "ON 💧" : "OFF");

    Serial.print("💾 Heap: ");            Serial.print(ESP.getFreeHeap());
    Serial.print(" bytes | Uptime: ");    Serial.print(millis() / 1000);
    Serial.println("s");

    Serial.println("────────────────────────────────────────\n");
}

// ==========================================
// Manual Alarm Buttons — feed the unified 5s latch (level + edge MQTT)
// ==========================================
// Sets manualAlarmActive (level) so updateFireAlarm() arms/refreshes the 5s FIRE
// latch — a brief click fires it, a sustained hold keeps it refreshed. Button edges
// also push manual_trigger to the AI immediately. The latch auto-shuts off 5s after
// release (updateFireAlarm), and stands down while the failsafe owns the actuators.
void processManualButtons() {
    bool btn1Now = sensors.isButton1Pressed();
    bool btn2Now = sensors.isButton2Pressed();

    // manual_trigger (for the AI) and the latch input both reflect whether EITHER
    // button is currently held. The 5s latch is armed/refreshed by updateFireAlarm(),
    // so a quick pulse fires it AND a sustained hold keeps it refreshed.
    manualAlarmActive = btn1Now || btn2Now;

    // Announce each edge to the AI immediately (don't wait for the 2s cadence).
    if (btn1Now != btn1WasPressed) {
        mqttHandler.publishManualAlarm("Room 1", btn1Now);
        publishSensorData();                 // push manual_trigger now
        if (btn1Now) Serial.println("[MANUAL] 🚨 Room 1 button → 5s FIRE latch.");
    }
    if (btn2Now != btn2WasPressed) {
        mqttHandler.publishManualAlarm("Room 2", btn2Now);
        publishSensorData();
        if (btn2Now) Serial.println("[MANUAL] 🚨 Room 2 button → 5s FIRE latch.");
    }

    btn1WasPressed = btn1Now;
    btn2WasPressed = btn2Now;
}
