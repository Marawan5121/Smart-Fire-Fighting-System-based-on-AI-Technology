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
// 📶 WiFi Configuration  (SSID/PASSWORD → secrets.h)
// ==========================================
#define WIFI_RETRY_DELAY   5000        // ms between reconnect attempts

// ==========================================
// 📡 MQTT Broker Configuration  (BROKER/AUTH → secrets.h)
// ==========================================
#define MQTT_PORT             1883
#define MQTT_CLIENT_ID        "sffs_esp32"
#define MQTT_KEEPALIVE        60        // seconds
#define MQTT_SOCKET_TIMEOUT_S 2         // B5: bound PubSubClient blocking to 2s
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
// 📌 GPIO Pin Configuration — Classic ESP32 WROOM-32 (38-pin)
// ==========================================
//
// ⚠️ This is a CLASSIC ESP32 (NOT an ESP32-S3).
//    ADC2 pins (GPIO 0,2,4,12,13,14,15,25,26,27) are unusable for
//    analog input while WiFi is active → all MQ sensors use ADC1.
//    Input-only GPIOs: 34, 35, 36 (VP), 39 (VN) — no internal pull-ups.
//    Flash SPI: GPIO 6-11 (DO NOT USE).
// ==========================================

// --- Analog Inputs (MQ Gas Sensors) — ADC1 only ---
#define GAS1_PIN         32    // MQ-2 (Combustible gas & smoke) - ADC1_CH4
#define GAS2_PIN         33    // MQ-5 (Natural gas & LPG)       - ADC1_CH5
#define GAS3_PIN         34    // MQ-6 (LPG, butane & propane)   - ADC1_CH6 (input-only)
#define GAS4_PIN         35    // MQ-7 (Carbon Monoxide)         - ADC1_CH7 (input-only)

// --- Digital Inputs ---
#define FLAME_PIN        27    // IR Flame sensor (Active LOW)
#define DHT_PIN          26    // DHT22 data pin

// --- I2C bus — DEDICATED to the PCA9685 PWM driver @0x40 ---
// The MPU6050 has been removed from the design, so this hardware I2C bus
// (GPIO 21 SDA / 22 SCL) now carries the PCA9685 exclusively.
#define I2C_SDA          21
#define I2C_SCL          22

// --- Digital Outputs ---
#define BOOT_LED_PIN     2     // Built-in LED (boot indicator only)
#define BUZZER_PIN       4     // Active buzzer
// NOTE: Both servos are driven by the PCA9685 over I2C (SERVO_*_CH below), NOT by
//       ESP32 GPIO. The old SERVO1/SERVO2 pins (18/19) are freed; GPIO 18 hosts the
//       green LED.
#define PUMP1_PIN        5     // Water pump 1 relay (Active LOW)
#define PUMP2_PIN        23    // Water pump 2 relay (Active LOW)

// --- Status Indicator LEDs (ALL active-HIGH / Common-Cathode: HIGH = ON, LOW = OFF) ---
#define LED_GREEN_PIN    18    // Green LED (SAFE) — active-HIGH, on a freed servo pin (servos now on PCA9685)
#define LED_ORANGE_PIN   14    // Orange LED (SENSOR_ALERT — sensor early warning)
#define LED_RED_PIN      25    // Red LED    (FIRE / MANUAL — confirmed emergency)

// --- Ultrasonic Water Level Sensor (HC-SR04) ---
#define ULTRASONIC_TRIG_PIN  15    // Trigger pulse output
#define ULTRASONIC_ECHO_PIN  36    // Echo input (VP, input-only, divider 5V→3.3V)

// --- Manual Alarm Push Buttons ---
// Reclaimed from the former SIM800L UART2 pins (SIM removed). GPIO 16/17 are
// regular GPIOs WITH internal pulldowns → both are INPUT_PULLDOWN, active-HIGH
// (wire each button between the pin and 3.3V). This also fixes the old GPIO39
// floating-input problem (39 had no internal pull).
#define BUTTON1_PIN      16    // Room 1 (INPUT_PULLDOWN, active HIGH)
#define BUTTON2_PIN      17    // Room 2 (INPUT_PULLDOWN, active HIGH)
#define BUTTON_DEBOUNCE_MS   300

// (GSM/SIM800L hardware removed from the design — no UART2 pins reserved.)

// ==========================================
// 🌡️ Sensor Thresholds & Calibration
// ==========================================
// ┌─────────────────────────────────────────────────────────────┐
// │ ⚠️ SINGLE SOURCE OF TRUTH — DRIFT WARNING (Defect 4)         │
// │ GAS_THRESHOLD and TEMP_THRESHOLD below MUST stay identical   │
// │ to the Python mirror in:                                    │
// │   Real-Time-Smoke-Fire-Detection-YOLO11/src/decision_engine.py │
// │     GAS_THRESHOLD = 2000   TEMP_THRESHOLD = 50.0            │
// │ Change them in ONE place, then update the other. The runtime │
// │ override path is the (future) `sffs/system/config` topic.   │
// └─────────────────────────────────────────────────────────────┘
#define GAS_THRESHOLD         2000    // ADC threshold for MQ sensors (0-4095)
#define TEMP_THRESHOLD        50.0    // °C ambient threshold
#define SENSOR_WARMUP_MS      60000   // 60s MQ warm-up gate

// --- Ultrasonic water tank ---
#define TANK_HEIGHT_CM               20.0   // sensor-to-tank-bottom distance
#define WATER_LEVEL_LOW_PCT          15.0   // dashboard low-water warning
#define ULTRASONIC_READ_INTERVAL_MS  500
#define ULTRASONIC_TIMEOUT_US        25000  // B5: bounded echo wait (~4.3m max)

// --- B6: pump dry-run protection (water safety) ---
#define PUMP_DRY_CUTOFF_PCT     10.0   // shut pumps off below this level
#define WATER_REFILL_RESUME_PCT 20.0   // hysteresis: re-enable pumps above this

// ==========================================
// 📈 Filter Settings (EMA)
// ==========================================
#define EMA_ALPHA            0.15f

// ==========================================
// 🎛️ PCA9685 16-ch PWM Servo Driver (I2C @ 0x40, dedicated bus 21/22)
// ==========================================
#define PCA9685_I2C_ADDR     0x40        // default address (A0-A5 unbridged)
#define PCA9685_OSC_FREQ     27000000UL  // on-board oscillator (Adafruit calibration)
#define SERVO_PWM_FREQ       50          // Hz — standard analog servo refresh
// 12-bit counts (0-4095) for the pulse range at 50Hz (~0.5ms..2.4ms), matching the
// old ESP32Servo attach(pin, 500, 2400) endpoints.
#define SERVO_PWM_MIN        102         // ~0.5ms → 0°
#define SERVO_PWM_MAX        491         // ~2.4ms → 180°

// 2 servos → PCA9685 channels 0-1 (3 extra servos to be added in a later step)
#define SERVO_GAS_VALVE_CH   0           // Gas valve servo
#define SERVO_DOORS_CH       1           // Door/window servo

// ==========================================
// 🔧 Servo Positions (degrees)
// ==========================================
#define SERVO_GAS_VALVE_OPEN    0
#define SERVO_GAS_VALVE_CLOSED  90
#define SERVO_DOOR_CLOSED       0
#define SERVO_DOOR_OPEN         90

// ==========================================
// 🔊 Buzzer Patterns (non-blocking, millis()-driven)
// ==========================================
// The buzzer mode is derived from the SYSTEM STATE, not a raw ON/OFF token:
//   SAFE          → silent
//   SENSOR_ALERT  → intermittent beep (early warning)
//   FIRE / MANUAL → continuous tone (confirmed emergency)
// Assumes an ACTIVE buzzer (built-in oscillator: sounds on DC HIGH). If yours
// is a PASSIVE buzzer, swap digitalWrite() for tone()/noTone() in actuators.cpp.
#define BUZZER_BEEP_INTERVAL_MS  250    // intermittent half-period (on/off toggle)

// ==========================================
// ⏱️ FIRE Alarm Auto-Timeout (non-blocking latch)
// ==========================================
// Every FIRE trigger (manual button, local gas/smoke/flame, or AI FIRE command)
// arms a fixed-duration emergency burst, then auto-returns to SAFE. Caps pump
// runtime (anti-flood / anti-burnout) and frees the alarm from button hold-time.
#define ALARM_LATCH_MS        5000    // FIRE suppression runs exactly 5s per trigger

// ==========================================
// 🛡️ Failsafe Configuration
// ==========================================
#define FAILSAFE_CRITICAL_TIMEOUT_MS  30000   // sustained danger before standalone
#define FAILSAFE_RECOVERY_HOLD_MS     10000   // hold in recovery before NORMAL

// ==========================================
// 📡 Serial & JSON
// ==========================================
#define DEBUG_BAUD           115200
#define JSON_BUFFER_SIZE     512

#endif // CONFIG_H
