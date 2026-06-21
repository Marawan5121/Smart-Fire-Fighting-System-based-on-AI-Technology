#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "secrets.h"   // WiFi/MQTT credentials live here (gitignored)

// Defensive defaults so the firmware still compiles if a secret is omitted.
#ifndef MQTT_USERNAME
#define MQTT_USERNAME ""
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif

// ==========================================
// WiFi Configuration  (SSID/PASSWORD -> secrets.h)
// ==========================================
#define WIFI_RETRY_DELAY   5000        // ms between reconnect attempts

// ==========================================
// MQTT Broker Configuration  (BROKER/AUTH -> secrets.h)
// ==========================================
#define MQTT_PORT             1883
#define MQTT_CLIENT_ID        "sffs_esp32"
#define MQTT_KEEPALIVE        60        // seconds
#define MQTT_SOCKET_TIMEOUT_S 2         // bound PubSubClient blocking to 2s
#define MQTT_RECONNECT_MIN_MS 5000      // base reconnect backoff
#define MQTT_RECONNECT_MAX_MS 30000     // capped reconnect backoff

// MQTT Topics
#define TOPIC_SENSOR_DATA     "sffs/sensors/data"
#define TOPIC_SYSTEM_STATUS   "sffs/system/status"
#define TOPIC_HEARTBEAT       "sffs/system/heartbeat"
#define TOPIC_MANUAL_ALARM    "sffs/manual/alarm"

// Publishing intervals (ms)
#define SENSOR_PUBLISH_INTERVAL   2000   // sensor data every 2s
#define HEARTBEAT_INTERVAL        5000   // heartbeat every 5s
#define TELEMETRY_PRINT_INTERVAL  2000   // serial dashboard every 2s

// ==========================================
// GPIO Pin Configuration -- Classic ESP32 WROOM-32 (38-pin)
// ==========================================
// ADC2 pins are unusable for analog input while WiFi is active -> all MQ
// sensors are on ADC1. Input-only GPIOs: 34, 35, 36 (VP), 39 (VN). Flash
// SPI: GPIO 6-11 (DO NOT USE).
// ==========================================

// --- Analog Inputs (MQ Gas Sensors), one per room -- ADC1 only ---
// Physical deployment (bench-locked): each room's gas index points to the GPIO
// of the MQ sensor installed in that room; each MQ keeps its canonical ADC pin.
#define GAS1_PIN         35    // Room 1: MQ-7  (CO)                - ADC1_CH7 (input-only)
#define GAS2_PIN         34    // Room 2: MQ-6  (isobutane/propane) - ADC1_CH6 (input-only)
#define GAS3_PIN         33    // Room 3: MQ-5  (natural gas / CH4) - ADC1_CH5
#define GAS4_PIN         32    // Room 4: MQ-2  (smoke / LPG / H2)  - ADC1_CH4

// --- Digital Inputs ---
#define FLAME_PIN        27    // IR flame sensor: INPUT_PULLUP, active-LOW (LOW = fire)
#define DHT_PIN          26    // DHT22 data pin

// --- I2C bus -- DEDICATED to the PCA9685 PWM driver @0x40 ---
#define I2C_SDA          21
#define I2C_SCL          22

// --- Digital Outputs ---
#define BOOT_LED_PIN     2     // Built-in LED (boot indicator only)
#define BUZZER_PIN       4     // Active buzzer (continuous tone on DC HIGH)

// --- Zonal Water Pump Relays (Active-LOW: HIGH = OFF, LOW = ON) ---
#define PUMP1_PIN        5     // Room 1 pump
#define PUMP2_PIN        23    // Room 2 pump
#define PUMP3_PIN        12    // Room 3 pump  (NOTE: GPIO12 is a boot strapping pin --
                               //               keep an external pull-DOWN on this relay
                               //               IN line so it reads LOW at reset, else the
                               //               board may fail to boot.)
#define PUMP4_PIN        13    // Room 4 pump

// --- Status Indicator LEDs (active-HIGH / Common-Cathode: HIGH = ON, LOW = OFF) ---
#define LED_GREEN_PIN    18    // Green LED (SAFE)
#define LED_RED_PIN      25    // Red LED   (FIRE)

// --- Ultrasonic Water Level Sensor (HC-SR04) ---
#define ULTRASONIC_TRIG_PIN  15    // Trigger pulse output
#define ULTRASONIC_ECHO_PIN  36    // Echo input (VP, input-only, divider 5V->3.3V)

// --- Manual Alarm Push Buttons (INPUT_PULLDOWN, active-HIGH: wire pin -> 3.3V) ---
#define BUTTON1_PIN      16    // Room 1
#define BUTTON2_PIN      17    // Room 2
#define BUTTON3_PIN      14    // Room 3
#define BUTTON4_PIN      19    // Room 4
#define BUTTON_DEBOUNCE_MS   300

// ==========================================
// Sensor Thresholds & Calibration
// ==========================================
// GAS_THRESHOLD mirrors the Python decision engine:
//   Real-Time-Smoke-Fire-Detection-YOLO11/src/decision_engine.py (GAS_THRESHOLD = 2000)
// Change it in one place, then update the other.
#define GAS_THRESHOLD         2000    // ADC threshold for MQ sensors (0-4095)
#define SENSOR_WARMUP_MS      60000   // 60s MQ warm-up gate (suppresses cold-start noise)

// --- Ultrasonic water tank ---
#define TANK_HEIGHT_CM               20.0   // sensor-to-tank-bottom distance
#define WATER_LEVEL_LOW_PCT          15.0   // dashboard low-water warning
#define ULTRASONIC_READ_INTERVAL_MS  500
#define ULTRASONIC_TIMEOUT_US        25000  // bounded echo wait (~4.3m max)

// --- Pump dry-run protection (water safety, hysteresis) ---
#define PUMP_DRY_CUTOFF_PCT     10.0   // cut pumps off below this level
#define WATER_REFILL_RESUME_PCT 20.0   // re-enable pumps above this level

// ==========================================
// Filter Settings (EMA)
// ==========================================
#define EMA_ALPHA            0.15f

// ==========================================
// PCA9685 16-ch PWM Servo Driver (I2C @ 0x40, dedicated bus 21/22)
// ==========================================
#define PCA9685_I2C_ADDR     0x40        // default address (A0-A5 unbridged)
#define PCA9685_OSC_FREQ     27000000UL  // on-board oscillator (Adafruit calibration)
#define SERVO_PWM_FREQ       50          // Hz -- standard analog servo refresh
#define SERVO_PWM_MIN        102         // ~0.5ms  -> 0 deg
#define SERVO_PWM_MAX        491         // ~2.4ms  -> 180 deg

// --- 9-servo zonal channel map (PCA9685 ch 0-8) ---
#define SERVO_GAS_VALVE_CH     0   // Gas main valve
#define SERVO_R1_DOOR_CH       1   // Room 1 door
#define SERVO_R2_DOOR_CH       2   // Room 2 door
#define SERVO_R3_DOOR_CH       3   // Room 3 door
#define SERVO_R4_DOOR_CH       4   // Room 4 door
#define SERVO_CORRIDOR_GF_CH   5   // Ground-floor corridor
#define SERVO_CORRIDOR_1F_CH   6   // First-floor corridor
#define SERVO_R3_WINDOW_CH     7   // Room 3 window
#define SERVO_R4_WINDOW_CH     8   // Room 4 window

// ==========================================
// Servo Positions (degrees)
// ==========================================
#define SERVO_GAS_VALVE_OPEN    0     // SAFE: gas flowing
#define SERVO_GAS_VALVE_CLOSED  180   // EMERGENCY: gas shut off
#define SERVO_DOOR_CLOSED       0     // SAFE
#define SERVO_DOOR_OPEN         90    // FIRE (door + corridor open angle)
#define SERVO_WINDOW_OPEN       90    // SAFE: room ventilated
#define SERVO_WINDOW_CLOSED     0     // LOCAL FIRE: starve the fire of oxygen

// ==========================================
// Failsafe (connectivity monitor) Configuration
// ==========================================
#define FAILSAFE_RECOVERY_HOLD_MS     10000   // hold in RECOVERY before NORMAL

// ==========================================
// Serial & JSON
// ==========================================
#define DEBUG_BAUD           115200
#define JSON_BUFFER_SIZE     512

// Number of independently-suppressed rooms.
#define ROOM_COUNT           4

#endif // CONFIG_H
