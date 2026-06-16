/**
 * ============================================================
 *  Smart Fire Fighting System (SFFS) — ESP32 Firmware
 * ============================================================
 * Target: CLASSIC ESP32 WROOM-32 (38-pin)pio run --target clean
 * Subsystems:
 *   - Sensor telemetry  (MQ-2/5/6/7, DHT22, MPU6050, IR flame, HC-SR04)
 *   - EMA noise filtering
 *   - WiFi auto-reconnect
 *   - Non-blocking MQTT (JSON publish + AI command subscribe)
 *   - Servo/pump/LED actuators
 *   - Verified, retrying GSM SMS alerts (SIM800L)
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
// #include "gsm.h"          // SIM800L REMOVED — re-include to restore GSM
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "actuators.h"
#include "failsafe.h"

// ==========================================
// Global Module Instances
// ==========================================
SensorsManager     sensors;
// GsmController      gsm(Serial2);  // SIM800L REMOVED (its UART2 pins 16/17 are now the buttons)
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

// UNIFIED 5-second non-blocking FIRE alarm latch. EVERY trigger — manual button,
// local gas/smoke/flame, or AI camera FIRE — arms a single 5s full-suppression
// burst (ALARM_LATCH_MS) that auto-clears to SAFE. Caps pump runtime (anti-flood/
// burnout) and decouples the alarm from button hold-time. A fresh trigger edge
// re-arms (restarts) the 5s window.
bool          fireAlarmActive  = false;
unsigned long fireAlarmStart   = 0;
bool          sensorDangerPrev = false;   // rising-edge tracking: local gas/smoke (→ FIRE)
bool          aiFirePrev       = false;   // rising-edge tracking: AI FIRE command

// Local early-warning tier (FLAME or TILT/earthquake) → SENSOR_ALERT, NOT FIRE.
// Edge-applied so the intermittent buzzer isn't re-initialised every loop.
bool          localAlertActive = false;   // flame or tilt currently asserted
bool          localAlertPrev   = false;   // edge tracking for the SENSOR_ALERT display

// Failsafe edge latches (apply emergency / safe actuators ONCE)
bool standaloneApplied = false;
bool recoveryApplied   = false;

// B6: pump dry-run protection
bool pumpsCutForWater  = false;
bool refillAlertActive = false;

// SIM800L REMOVED — GSM SMS delivery bookkeeping disabled.
// bool gsmSmsConfirmed   = false;
// int  gsmSmsAttempts    = 0;
// bool gsmGaveUpLogged   = false;

// ==========================================
// Prototypes
// ==========================================
void processAiCommands();
void processManualButtons();
void processLocalSensors();
void armFireAlarm(const char* source);
void updateFireAlarm();
void processFailsafe();
void managePumpWaterSafety();
// void manageStandaloneSms();   // SIM800L REMOVED
// void resetSmsState();          // SIM800L REMOVED
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

    // Shared hardware I2C bus — begin it ONCE so BOTH the MPU6050 (@0x68, in
    // sensors.begin) and the PCA9685 (@0x40, in actuators.begin) attach to it.
    Wire.begin(I2C_SDA, I2C_SCL);

    sensors.begin();
    // gsm.begin();   // SIM800L REMOVED
    actuators.begin(BUZZER_PIN,
                    PUMP1_PIN, PUMP2_PIN, PUMP3_PIN,
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
    // gsm.update();   // SIM800L REMOVED
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

    // 3. Alarm triggers
    processManualButtons();    // manual buttons → 5s FIRE latch
    processLocalSensors();     // gas/smoke → 5s FIRE latch; flame/tilt → SENSOR_ALERT
    processAiCommands();       // AI FIRE arms the latch; AI SAFE/SENSOR_ALERT applied live

    // 4. Auto-shutoff: return to SAFE exactly 5s after the latch was last armed
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

    // In failsafe (offline) modes the ESP32 is autonomous — ignore AI entirely.
    // Reset the edge tracker so a still-FIRE AI re-arms on return to NORMAL.
    if (failsafe.getState() != FailsafeManager::STATE_NORMAL) {
        aiFirePrev = false;
        return;
    }

    bool aiFire = (strcmp(cmd.state, "FIRE") == 0);

    if (aiFire) {
        // A fresh AI→FIRE transition arms the 5s latch (manual press takes priority).
        // A sustained FIRE does NOT keep re-arming → 5s auto-shutoff still applies.
        if (!aiFirePrev && !manualAlarmActive) {
            armFireAlarm("AI command");
        }
    } else {
        // Non-FIRE AI states (SAFE / SENSOR_ALERT) are display/warning states: apply
        // them live, but never override a FIRE latch, a manual alarm, or a local
        // flame/tilt SENSOR_ALERT (which processLocalSensors owns).
        if (!fireAlarmActive && !manualAlarmActive && !localAlertActive) {
            actuators.applyCommands(cmd.gasValve, cmd.doors, cmd.pump1, cmd.pump2);
            actuators.applyState(cmd.state);   // LED color + buzzer pattern from state
            Serial.print("[AI CMD] Executed: State=");
            Serial.print(cmd.state);
            Serial.print(" Conf=");
            Serial.println(cmd.confidence, 2);
        }
    }
    aiFirePrev = aiFire;
}

// ==========================================
// Local Sensors — priority matrix
// ==========================================
//   GAS / SMOKE (MQ)      → full FIRE latch (unified 5s), rising-edge armed.
//   FLAME (IR) or TILT    → SENSOR_ALERT (orange LED + intermittent buzzer); does
//   (MPU6050 earthquake)    NOT trip FIRE — only gas/AI camera/manual buttons do.
// FIRE outranks SENSOR_ALERT; the warning display is edge-applied so the
// intermittent buzzer pattern isn't re-initialised every loop.
void processLocalSensors() {
    // Stand down ONLY while the failsafe is actively driving the actuators
    // (STANDALONE_ALERT). In NORMAL, DEGRADED and RECOVERY the local sensors stay
    // LIVE — this is the Bug-3 fix: previously a missing MQTT broker dropped the node
    // into DEGRADED and the early-return killed the flame/tilt alert on the bench.
    if (failsafe.shouldActAutonomously()) {
        sensorDangerPrev = false;
        localAlertPrev   = false;
        localAlertActive = false;
        return;
    }

    // --- Priority 1: GAS/SMOKE → FIRE latch (rising edge). Warm-up gated: the MQ
    //     heaters need ~60 s to stabilise before their readings are trustworthy. ---
    bool gasDanger = sensors.isWarmedUp() && sensors.isGasDanger();
    if (gasDanger && !sensorDangerPrev && !manualAlarmActive) {
        armFireAlarm("local gas/smoke");
    }
    sensorDangerPrev = gasDanger;

    // --- Priority 2: FLAME or TILT → SENSOR_ALERT (warning only). NOT warm-up
    //     gated: the IR flame sensor and the MPU6050 need no heater warm-up, so they
    //     must respond instantly (Bug-3: flame was being ignored during warm-up). ---
    localAlertActive = (sensors.isFlameDetected() || sensors.isTiltDetected());

    // A confirmed FIRE (gas/AI/manual) outranks the warning. Reset the edge so a
    // still-present flame/tilt re-asserts SENSOR_ALERT once the FIRE clears.
    if (fireAlarmActive || manualAlarmActive) {
        localAlertPrev = false;
        return;
    }

    if (localAlertActive && !localAlertPrev) {
        actuators.applyState("SENSOR_ALERT");   // edge: 🟠 orange LED + intermittent buzzer
        Serial.println("[Sensor] FLAME/TILT detected → SENSOR_ALERT (orange + intermittent buzzer).");
    } else if (!localAlertActive && localAlertPrev) {
        actuators.applyState("SAFE");           // edge: clear back to 🟢 green / silent
        Serial.println("[Sensor] FLAME/TILT cleared → SAFE.");
    }
    localAlertPrev = localAlertActive;
}

// ==========================================
// Unified 5-Second FIRE Alarm Latch (non-blocking)
// ==========================================
// armFireAlarm() drives the FULL suppression set (valve CLOSE, doors OPEN, both
// pumps ON, red LED, continuous buzzer) and (re)starts the 5s timer. Every FIRE
// trigger — manual button, local gas/smoke/flame, or AI FIRE — routes through it.
void armFireAlarm(const char* source) {
    actuators.setEmergency();
    fireAlarmStart = millis();
    Serial.print(fireAlarmActive ? "[Alarm] FIRE latch RE-ARMED (5s) — "
                                  : "[Alarm] 🔥 FIRE latch ARMED (5s) — ");
    Serial.println(source);
    fireAlarmActive = true;
}

// updateFireAlarm() enforces the timeout. After exactly ALARM_LATCH_MS it returns
// the node to SAFE. If the failsafe has taken over (offline standalone), it hands
// the actuators off WITHOUT forcing SAFE so the two mechanisms never fight.
void updateFireAlarm() {
    if (!fireAlarmActive) return;

    if (failsafe.shouldActAutonomously()) {
        fireAlarmActive   = false;
        manualAlarmActive = false;   // failsafe owns the actuators now
        return;
    }

    if (millis() - fireAlarmStart >= ALARM_LATCH_MS) {
        fireAlarmActive   = false;
        manualAlarmActive = false;   // also clears manual_trigger for the AI
        actuators.setAllSafe();      // valve OPEN, doors CLOSE, pumps OFF, green ON, buzzer off
        publishSensorData();         // tell the AI manual_trigger is cleared
        Serial.println("[Alarm] ⏱️ 5s elapsed → auto-shutoff. Returned to SAFE.");
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

        // managePumpWaterSafety() now runs every loop (see loop step 5) — Pump 3 is
        // continuous, so its dry-run guard can't live in the offline-only path.
        // manageStandaloneSms();  // SIM800L REMOVED — standalone alert still trips actuators/buzzer
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
        // resetSmsState();   // SIM800L REMOVED
    } else if (st == FailsafeManager::STATE_NORMAL) {
        recoveryApplied = false;
        // resetSmsState();   // SIM800L REMOVED
    }
}

// ==========================================
// B6: Pump Dry-Run Protection — UNIFIED for Pumps 1, 2 & 3 (runs EVERY loop)
// ==========================================
// All three pumps are now active-suppression pumps (ON only in EMERGENCY). The
// dry-run cutoff applies to them identically: if the tank drops below the cutoff
// while any pump is running, ALL three are shut off; once refilled past the resume
// threshold, all three are re-enabled together — but ONLY while a fire emergency
// still requires suppression. Hysteresis (PUMP_DRY_CUTOFF_PCT → WATER_REFILL_RESUME_PCT)
// prevents relay chatter around the threshold.
void managePumpWaterSafety() {
    float level    = sensors.getWaterLevelPct();
    bool  dry      = (level < PUMP_DRY_CUTOFF_PCT);
    bool  refilled = (level >= WATER_REFILL_RESUME_PCT);

    if (!pumpsCutForWater && dry &&
        (actuators.isPump1Active() || actuators.isPump2Active() || actuators.isPump3Active())) {
        // Empty tank while pumping → cut all three to prevent dry-running.
        actuators.pump1Off();
        actuators.pump2Off();
        actuators.pump3Off();
        pumpsCutForWater  = true;
        refillAlertActive = true;
        Serial.print("[Safety] WATER LOW (");
        Serial.print(level, 1);
        Serial.println("%) → ALL pumps OFF (dry-run prevention). REFILL ALERT raised.");
    } else if (pumpsCutForWater && refilled) {
        // Re-enable all three together ONLY if a fire emergency is still active.
        if (fireAlarmActive || failsafe.shouldActAutonomously()) {
            actuators.pump1On();
            actuators.pump2On();
            actuators.pump3On();
            Serial.print("[Safety] Water restored (");
            Serial.print(level, 1);
            Serial.println("%) → all pumps re-enabled.");
        }
        pumpsCutForWater  = false;
        refillAlertActive = false;
    }
}

// ==========================================
// B6: Verified SMS Delivery with Bounded Retries  — SIM800L REMOVED
// ==========================================
// The entire SMS path is disabled while the SIM800L is unplugged. The failsafe
// still trips local suppression (actuators + buzzer) when offline; it just no
// longer sends an SMS. Re-enable by restoring the gsm object in this file and
// uncommenting this block.
/*
void manageStandaloneSms() {
    if (!sensors.isWarmedUp()) return;

    // Only trust SUCCESS that belongs to THIS episode (attempts > 0); a result
    // left over from a previous standalone episode must not suppress a new alert.
    if (gsmSmsAttempts > 0 && gsm.getResult() == GsmController::GSM_RESULT_SUCCESS) {
        if (!gsmSmsConfirmed) {
            gsmSmsConfirmed = true;
            Serial.println("[Failsafe] SMS delivery CONFIRMED by SIM800L.");
        }
        return;
    }
    if (gsmSmsConfirmed) return;
    if (gsm.isBusy())    return;   // an attempt (or its cooldown) is still running

    if (gsmSmsAttempts >= MAX_SMS_RETRIES) {
        if (!gsmGaveUpLogged) {
            gsmGaveUpLogged = true;
            Serial.print("[Failsafe] SMS FAILED after ");
            Serial.print(MAX_SMS_RETRIES);
            Serial.println(" attempts. Giving up (buzzer/pumps remain active).");
        }
        return;
    }

    // Build the message in a fixed stack buffer (no heap String).
    char sms[161];   // 160 GSM chars + NUL
    snprintf(sms, sizeof(sms),
        "SFFS STANDALONE ALERT! Temp:%.1fC Gas:%d Flame:%s Water:%.0f%% WiFi:DOWN MQTT:DOWN",
        sensors.getTemperature(), (int)sensors.getGas1(),
        sensors.isFlameDetected() ? "YES" : "NO", sensors.getWaterLevelPct());

    if (gsm.sendSMSAsync(GSM_PHONE_NUMBER, sms)) {
        gsmSmsAttempts++;
        Serial.print("[Failsafe] SMS attempt ");
        Serial.print(gsmSmsAttempts);
        Serial.print("/");
        Serial.println(MAX_SMS_RETRIES);
    }
}

void resetSmsState() {
    if (gsmSmsAttempts != 0 || gsmSmsConfirmed) {
        Serial.println("[Failsafe] SMS state reset (connectivity restored).");
    }
    gsmSmsAttempts  = 0;
    gsmSmsConfirmed = false;
    gsmGaveUpLogged = false;
}
*/

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
        sensors.getAccelX(), sensors.getAccelY(), sensors.getAccelZ(),
        sensors.isTiltDetected(), sensors.isFlameDetected(),
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

    Serial.print("📐 Tilt | Angle: ");  Serial.print(sensors.getTiltAngle(), 1);
    Serial.print("° | ");               Serial.println(sensors.isTiltDetected() ? "TILT ⚠️" : "Level");

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
        Serial.print("s left");
    } else {
        Serial.print("—");
    }
    Serial.println(" | GSM/SMS: SIM REMOVED");

    Serial.print("⚙️ Actuators | Valve: "); Serial.print(actuators.isGasValveOpen() ? "OPEN" : "CLOSED");
    Serial.print(" | Doors: ");              Serial.print(actuators.isDoorOpen() ? "OPEN" : "CLOSED");
    Serial.print(" | Buzzer: ");             Serial.print(actuators.isBuzzerActive() ? "ON 🔊" : "OFF");
    Serial.print(" | Pump1: ");              Serial.print(actuators.isPump1Active() ? "ON 💧" : "OFF");
    Serial.print(" | Pump2: ");              Serial.print(actuators.isPump2Active() ? "ON 💧" : "OFF");
    Serial.print(" | Pump3: ");              Serial.println(actuators.isPump3Active() ? "ON 💧" : "OFF");

    Serial.print("💾 Heap: ");            Serial.print(ESP.getFreeHeap());
    Serial.print(" bytes | Uptime: ");    Serial.print(millis() / 1000);
    Serial.println("s");

    Serial.println("────────────────────────────────────────\n");
}

// ==========================================
// Manual Alarm Buttons — arm the unified 5s latch (edge-detected)
// ==========================================
// A brief click (no hold needed) (re)arms the 5s FIRE latch via armFireAlarm().
// The latch auto-shuts off after 5s (updateFireAlarm). Skipped only while the
// failsafe is already driving actuators (STANDALONE).
void processManualButtons() {
    bool btn1Now = sensors.isButton1Pressed();
    bool btn2Now = sensors.isButton2Pressed();

    bool btn1Rising = (btn1Now && !btn1WasPressed);
    bool btn2Rising = (btn2Now && !btn2WasPressed);

    if ((btn1Rising || btn2Rising) && !failsafe.shouldActAutonomously()) {
        const char* room = btn1Rising ? "Room 1" : "Room 2";
        manualAlarmActive = true;            // suppresses AI path + sets manual_trigger
        armFireAlarm("MANUAL button");       // full suppression for 5s (re-armed on each press)
        mqttHandler.publishManualAlarm(room, true);
        publishSensorData();                 // push manual_trigger=1 to the AI immediately
        Serial.print("[MANUAL] 🚨 ");
        Serial.print(room);
        Serial.println(" button → 5s FIRE latch armed.");
    }

    btn1WasPressed = btn1Now;
    btn2WasPressed = btn2Now;
}
