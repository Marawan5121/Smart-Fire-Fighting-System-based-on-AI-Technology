#include <Arduino.h>
#include <string.h>
#include "config.h"
#include "sensors.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "actuators.h"
#include "failsafe.h"

// ==========================================
// Global module instances
// ==========================================
SensorsManager     sensors;
WifiManager        wifi(WIFI_SSID, WIFI_PASSWORD, WIFI_RETRY_DELAY);
MqttHandler        mqttHandler(MQTT_BROKER, MQTT_PORT, MQTT_CLIENT_ID);
ActuatorController actuators;
FailsafeManager    failsafe(FAILSAFE_RECOVERY_HOLD_MS);

// ==========================================
// Timing
// ==========================================
unsigned long lastSensorPublish = 0;
unsigned long lastHeartbeat      = 0;
unsigned long lastTelemetryPrint = 0;

// ==========================================
// Fused zonal command + appliedState edge-tracking
// ==========================================
struct ZoneCommand {
    bool anyFire;
    bool roomFire[ROOM_COUNT];
    bool pumpOn[ROOM_COUNT];
};

ZoneCommand appliedCmd;          // zero-initialized (static storage)
bool        appliedInit = false; // force the very first apply
bool        pumpsAllowed = true; // water-level hysteresis gate
bool        btnPrev[ROOM_COUNT] = { false, false, false, false };

const char* const ROOM_NAMES[ROOM_COUNT] = { "Room 1", "Room 2", "Room 3", "Room 4" };

// ==========================================
// Prototypes
// ==========================================
ZoneCommand evaluateZones();
void        updateWaterGate();
void        applyZoneCommand(const ZoneCommand& c);
void        publishManualButtonEdges();
void        publishSensorData();
void        publishHeartbeat();
void        printTelemetry(const ZoneCommand& c);
bool        anyGasDanger();

// ==========================================
// SETUP
// ==========================================
void setup() {
    Serial.begin(DEBUG_BAUD);
    delay(1000);
    Serial.println("\n=========================================");
    Serial.println(" SFFS -- 4-Room Zonal Suppression (9 servos)");
    Serial.println(" Classic ESP32 (38-pin)");
    Serial.println("=========================================\n");

    pinMode(BOOT_LED_PIN, OUTPUT);
    digitalWrite(BOOT_LED_PIN, HIGH);

    sensors.begin();
    actuators.begin();      // brings up PCA9685 + leaves hardware in SAFE state
    failsafe.begin();
    wifi.begin();
    mqttHandler.begin();

    digitalWrite(BOOT_LED_PIN, LOW);
    Serial.println("\n[System] Subsystems initialized. Entering main loop.\n");
}

// ==========================================
// MAIN LOOP
// ==========================================
void loop() {
    sensors.update();
    wifi.update();
    mqttHandler.update();

    // Connectivity FSM (telemetry only -- does not actuate hardware).
    failsafe.update(wifi.isConnected(), mqttHandler.isConnected());

    publishManualButtonEdges();

    // 1. Evaluate per-room fire status, 2. apply water gate, 3. derive pumps.
    ZoneCommand desired = evaluateZones();
    updateWaterGate();
    for (int r = 0; r < ROOM_COUNT; r++) {
        desired.pumpOn[r] = desired.roomFire[r] && pumpsAllowed;
    }

    // appliedState edge-tracking: write hardware only when the command changes.
    if (!appliedInit || memcmp(&desired, &appliedCmd, sizeof(ZoneCommand)) != 0) {
        applyZoneCommand(desired);
        appliedCmd  = desired;
        appliedInit = true;
    }

    unsigned long now = millis();
    if (now - lastSensorPublish >= SENSOR_PUBLISH_INTERVAL) {
        lastSensorPublish = now;
        publishSensorData();
    }
    if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
        lastHeartbeat = now;
        publishHeartbeat();
    }
    if (now - lastTelemetryPrint >= TELEMETRY_PRINT_INTERVAL) {
        lastTelemetryPrint = now;
        printTelemetry(desired);
    }
}

// ==========================================
// Zonal fire evaluation (level-driven)
// ==========================================
ZoneCommand evaluateZones() {
    ZoneCommand c;
    memset(&c, 0, sizeof(c));   // ensure padding is zero for reliable memcmp

    // Global overrides: local flame (active-LOW) or AI "FIRE" -> all rooms fire.
    bool globalFire = sensors.isFlameDetected() || mqttHandler.isFireCommanded();

    for (int r = 0; r < ROOM_COUNT; r++) {
        bool gas = sensors.isWarmedUp() && (sensors.getGas(r) > GAS_THRESHOLD);
        bool fire = globalFire || sensors.isButtonPressed(r) || gas;
        c.roomFire[r] = fire;
        if (fire) c.anyFire = true;
    }
    return c;
}

// ==========================================
// Water-level pump gate (hysteresis)
// ==========================================
void updateWaterGate() {
    float level = sensors.getWaterLevelPct();
    if (level < PUMP_DRY_CUTOFF_PCT)            pumpsAllowed = false;  // cut off
    else if (level >= WATER_REFILL_RESUME_PCT)  pumpsAllowed = true;   // re-enable
    // between thresholds: hold the current gate state
}

// ==========================================
// Drive actuators from the fused command
// ==========================================
void applyZoneCommand(const ZoneCommand& c) {
    // Global actions (any room in fire)
    actuators.setLeds(c.anyFire);        // red on / green off, else green on
    actuators.setGasValve(c.anyFire);    // CLOSE(180) on fire, else OPEN(0)
    actuators.setBuzzer(c.anyFire);      // continuous tone on fire
    actuators.setCorridors(c.anyFire);   // ch5 & ch6 -> 90 on any fire

    // Zonal actions
    for (int r = 0; r < ROOM_COUNT; r++) {
        actuators.setRoomDoor(r, c.roomFire[r]);  // open(90) on fire
        actuators.setPump(r, c.pumpOn[r]);
    }
    // Windows exist only for Room 3 (idx 2) and Room 4 (idx 3): close on local fire.
    actuators.setRoomWindow(2, c.roomFire[2]);
    actuators.setRoomWindow(3, c.roomFire[3]);

    Serial.print("[Control] anyFire=");
    Serial.print(c.anyFire ? "Y" : "N");
    Serial.print(" rooms=");
    for (int r = 0; r < ROOM_COUNT; r++) Serial.print(c.roomFire[r] ? "1" : "0");
    Serial.print(" pumps=");
    for (int r = 0; r < ROOM_COUNT; r++) Serial.print(c.pumpOn[r] ? "1" : "0");
    Serial.println();
}

// ==========================================
// Manual button edge publishing (dashboard)
// ==========================================
void publishManualButtonEdges() {
    for (int r = 0; r < ROOM_COUNT; r++) {
        bool now = sensors.isButtonPressed(r);
        if (now != btnPrev[r]) {
            mqttHandler.publishManualAlarm(ROOM_NAMES[r], now);
            btnPrev[r] = now;
        }
    }
}

bool anyGasDanger() {
    if (!sensors.isWarmedUp()) return false;
    for (int r = 0; r < ROOM_COUNT; r++) {
        if (sensors.getGas(r) > GAS_THRESHOLD) return true;
    }
    return false;
}

// ==========================================
// Telemetry publishing
// ==========================================
void publishSensorData() {
    if (!mqttHandler.isConnected()) return;
    const char* mode = anyGasDanger() ? "GAS_ALARM" : failsafe.getStateString();

    mqttHandler.publishSensorData(
        sensors.getGas(0), sensors.getGas(1), sensors.getGas(2), sensors.getGas(3),
        sensors.getTemperature(), sensors.getHumidity(), sensors.isFlameDetected(),
        sensors.getWaterLevelPct(), sensors.getWaterDistanceCm(),
        sensors.anyButtonPressed(), wifi.getRSSI(), mode
    );
    lastSensorPublish = millis();
}

void publishHeartbeat() {
    if (!mqttHandler.isConnected()) return;
    mqttHandler.publishHeartbeat();
}

// ==========================================
// Serial telemetry
// ==========================================
void printTelemetry(const ZoneCommand& c) {
    Serial.println("----------------------------------------");
    if (!sensors.isWarmedUp()) {
        Serial.print("MQ Warmup: ");
        Serial.print(sensors.getWarmupTimeRemaining());
        Serial.println("s remaining...");
    }
    Serial.print("Gas | R1:");  Serial.print(sensors.getGas(0), 0);
    Serial.print(" R2:");        Serial.print(sensors.getGas(1), 0);
    Serial.print(" R3:");        Serial.print(sensors.getGas(2), 0);
    Serial.print(" R4:");        Serial.println(sensors.getGas(3), 0);
    Serial.print("Env | Temp:"); Serial.print(sensors.getTemperature(), 1);
    Serial.print("C Hum:");      Serial.print(sensors.getHumidity(), 1); Serial.println("%");
    Serial.print("Flame: ");     Serial.println(sensors.isFlameDetected() ? "DETECTED" : "Clear");
    Serial.print("Water | Level:"); Serial.print(sensors.getWaterLevelPct(), 1);
    Serial.print("% Dist:");        Serial.print(sensors.getWaterDistanceCm(), 1);
    Serial.print("cm  PumpsAllowed:"); Serial.println(pumpsAllowed ? "Y" : "N");
    Serial.print("Buttons | ");
    for (int r = 0; r < ROOM_COUNT; r++) {
        Serial.print(ROOM_NAMES[r]); Serial.print(":");
        Serial.print(sensors.isButtonPressed(r) ? "P " : "- ");
    }
    Serial.println();
    Serial.print("Mode:"); Serial.print(failsafe.getStateString());
    Serial.print(" | FIRE:"); Serial.print(c.anyFire ? "Y" : "N");
    Serial.print(" rooms=");
    for (int r = 0; r < ROOM_COUNT; r++) Serial.print(c.roomFire[r] ? "1" : "0");
    Serial.println();
    Serial.println("----------------------------------------\n");
}
