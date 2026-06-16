# Implementation Plan — Pumps, Status LEDs, Ultrasonic Water Level & AI Detection

## Overview

Four integrated changes:
1. **Two relay-controlled water pumps** (GPIO 5 & GPIO 23)
2. **Three status LEDs** — Red (danger confirmed), Orange (uncertain), Green (safe) — (GPIO 25, 14, 13)
3. **HC-SR04 ultrasonic sensor** for water tank level measurement (GPIO 15 TRIG, GPIO 36 ECHO)
4. **Direct AI detection publishing** to Node-RED via `sffs/ai/detection`

---

## Hardware Wiring — Complete Updated Table

> [!WARNING]
> **HC-SR04 ECHO pin outputs 5V!** You MUST use a voltage divider (1kΩ + 2kΩ) on the ECHO line to bring it down to 3.3V for the ESP32. The TRIG input accepts 3.3V just fine.

| Component | GPIO | Pin Type | Notes |
|---|---|---|---|
| **MQ-2** (Smoke) | GPIO 32 | ADC1_CH4 | Voltage divider 5V→3.3V |
| **MQ-5** (LPG) | GPIO 33 | ADC1_CH5 | Voltage divider 5V→3.3V |
| **MQ-6** (Butane) | GPIO 34 | ADC1_CH6 | Input-only — fine for analog |
| **MQ-7** (CO) | GPIO 35 | ADC1_CH7 | Input-only — fine for analog |
| **Flame IR** | GPIO 27 | Digital IN | Active LOW |
| **DHT22** | GPIO 26 | Digital | 10kΩ pull-up to 3.3V |
| **MPU6050 SDA** | GPIO 21 | I2C | 4.7kΩ pull-up |
| **MPU6050 SCL** | GPIO 22 | I2C | 4.7kΩ pull-up |
| **Built-in LED** | GPIO 2 | Digital OUT | Boot indicator only |
| **Buzzer** | GPIO 4 | Digital OUT | Active buzzer |
| **Servo 1** (Gas Valve) | GPIO 18 | PWM | SG90, 50Hz |
| **Servo 2** (Doors) | GPIO 19 | PWM | SG90, 50Hz |
| **Pump 1** (Water) | **GPIO 5** | **Digital OUT** | **Relay IN1 (Active LOW)** |
| **Pump 2** (Water) | **GPIO 23** | **Digital OUT** | **Relay IN2 (Active LOW)** |
| **🟢 Green LED** | **GPIO 13** | **Digital OUT** | **SAFE state** |
| **🟠 Orange LED** | **GPIO 14** | **Digital OUT** | **SMOKE (uncertain) state** |
| **🔴 Red LED** | **GPIO 25** | **Digital OUT** | **FIRE / GAS_LEAK (confirmed danger)** |
| **HC-SR04 TRIG** | **GPIO 15** | **Digital OUT** | **Ultrasonic trigger pulse** |
| **HC-SR04 ECHO** | **GPIO 36 (VP)** | **Input-only** | **⚠️ Voltage divider 5V→3.3V required** |
| **SIM800L TX→** | GPIO 16 | UART2 RX | ESP32 receives |
| **SIM800L RX←** | GPIO 17 | UART2 TX | ESP32 sends |

> [!NOTE]
> After these additions, **all 21 usable GPIOs** on the ESP32 38-pin board are allocated. No pins remain for future expansion without removing existing components.

### Updated Pinout Diagram

```
              ┌────────────────────┐
        3.3V ─┤                    ├─ VIN (5V) → Relay VCC + HC-SR04 VCC
         GND ─┤                    ├─ GND
  GPIO 15 TRG─┤                    ├─ GPIO 13 🟢LED
  GPIO  2 LED─┤                    ├─ GPIO 12
  GPIO  4 BUZ─┤                    ├─ GPIO 14 🟠LED
  GPIO 16 RX2─┤                    ├─ GPIO 27 FLAME
  GPIO 17 TX2─┤                    ├─ GPIO 26 DHT22
  GPIO  5 PMP1┤     ESP32 38-Pin  ├─ GPIO 25 🔴LED
  GPIO 18 SV1─┤                    ├─ GPIO 33 MQ5
  GPIO 19 SV2─┤                    ├─ GPIO 32 MQ2
  GPIO 21 SDA─┤                    ├─ GPIO 35 MQ7
   GPIO 3 RX0─┤                    ├─ GPIO 34 MQ6
   GPIO 1 TX0─┤                    ├─ GPIO 39 (VN)
  GPIO 22 SCL─┤                    ├─ GPIO 36 ECHO (VP)
  GPIO 23 PMP2┤                    ├─ EN
              └────────────────────┘
```

### HC-SR04 Wiring Detail

```
  ESP32 VIN (5V) ──────► HC-SR04 VCC
  ESP32 GND      ──────► HC-SR04 GND
  ESP32 GPIO 15  ──────► HC-SR04 TRIG

  HC-SR04 ECHO ───┬── 1kΩ ──┬── ESP32 GPIO 36 (VP)
                   │         │
                   └── 2kΩ ──┘── GND
                   (Voltage divider: 5V × 2k/(1k+2k) = 3.3V)
```

### Relay Module Wiring

```
  ESP32 VIN (5V) ──────► Relay VCC
  ESP32 GND      ──────► Relay GND
  ESP32 GPIO  5  ──────► Relay IN1  ──── NO1 ──── Pump 1 (+) ──┐
  ESP32 GPIO 23  ──────► Relay IN2  ──── NO2 ──── Pump 2 (+) ──┤
                                         COM ─────────────────── External PSU (+)
                                         Pump (-) ──────────── External PSU (-)
```

---

## LED State Mapping

| System State | 🟢 Green | 🟠 Orange | 🔴 Red | Meaning |
|---|---|---|---|---|
| **SAFE** | ✅ ON | OFF | OFF | All clear |
| **SMOKE** | OFF | ✅ ON | OFF | Uncertain — smoke detected, not confirmed fire |
| **GAS_LEAK** | OFF | OFF | ✅ ON | Confirmed danger — gas leak |
| **FIRE** | OFF | OFF | ✅ ON | Confirmed danger — fire detected |

---

## Actuator Logic per State

| State | Gas Valve | Doors | Buzzer | Pump 1 | Pump 2 | LEDs |
|---|---|---|---|---|---|---|
| **SAFE** | OPEN | CLOSE | OFF | OFF | OFF | 🟢 |
| **GAS_LEAK** | CLOSE | OPEN | ON | OFF | OFF | 🔴 |
| **SMOKE** | CLOSE | CLOSE | ON | OFF | OFF | 🟠 |
| **FIRE** | CLOSE | OPEN | ON | **ON** | **ON** | 🔴 |

---

## Water Level Measurement

The HC-SR04 is mounted at the **top** of the water tank, pointing down at the water surface.

```
  ┌──────────────┐ ← HC-SR04 sensor (mounted here)
  │              │
  │   AIR        │ ← measured distance (distance_cm)
  │              │
  │ ~~~~~~~~~~~~ │ ← water surface
  │ ▓▓▓▓▓▓▓▓▓▓▓ │
  │ ▓▓ WATER ▓▓ │
  │ ▓▓▓▓▓▓▓▓▓▓▓ │
  └──────────────┘ ← tank bottom (TANK_HEIGHT_CM away from sensor)
```

**Formula**: `water_level_pct = ((TANK_HEIGHT_CM - distance_cm) / TANK_HEIGHT_CM) × 100`

- `TANK_HEIGHT_CM = 20` (configurable in `config.h`)
- Clamped to 0–100% range
- Readings are EMA-filtered for stability
- Published every 2 seconds in the sensor JSON payload under `"water": {"level_pct": 75, "distance_cm": 5.0}`

---

## Proposed Changes

### ESP32 Firmware

---

#### [MODIFY] [config.h](file:///D:/Graduation%20Project/iot/ProjectGrd-1/ProjectGrd-1/include/config.h)
- Add pump pins: `PUMP1_PIN 5`, `PUMP2_PIN 23`
- Add LED pins: `LED_GREEN_PIN 13`, `LED_ORANGE_PIN 14`, `LED_RED_PIN 25`
- Add ultrasonic pins: `ULTRASONIC_TRIG_PIN 15`, `ULTRASONIC_ECHO_PIN 36`
- Add tank config: `TANK_HEIGHT_CM 20.0`, `WATER_LEVEL_LOW_PCT 15.0`
- Remove old `LED_PIN 2` (replaced by 3-LED system, GPIO 2 still available for boot blink)

---

#### [MODIFY] [actuators.h](file:///D:/Graduation%20Project/iot/ProjectGrd-1/ProjectGrd-1/include/actuators.h)
- Add pump members: `_pump1Pin`, `_pump2Pin`, `_pump1Active`, `_pump2Active`
- Add LED members: `_ledGreenPin`, `_ledOrangePin`, `_ledRedPin`
- Add pump methods: `pump1On/Off()`, `pump2On/Off()`, `setPump1/2(command)`
- Add LED method: `setStatusLeds(state)` — takes state string, lights the correct LED
- Update `begin()` to accept pump and LED pins
- Update `applyCommands()` to accept pump commands
- Add getters: `isPump1Active()`, `isPump2Active()`

---

#### [MODIFY] [actuators.cpp](file:///D:/Graduation%20Project/iot/ProjectGrd-1/ProjectGrd-1/src/actuators.cpp)
- Implement pump relay control (active LOW)
- Implement 3-LED state indicator logic
- Update `setAllSafe()` → pumps OFF, green LED ON
- Update `setEmergency()` → pumps ON, red LED ON

---

#### [MODIFY] [sensors.h](file:///D:/Graduation%20Project/iot/ProjectGrd-1/ProjectGrd-1/include/sensors.h) + [sensors.cpp](file:///D:/Graduation%20Project/iot/ProjectGrd-1/ProjectGrd-1/src/sensors.cpp)
- Add HC-SR04 ultrasonic distance measurement
- Add `getWaterLevelPct()` and `getWaterDistanceCm()` methods
- EMA filter on distance readings for stability
- Non-blocking measurement using `pulseIn()` with timeout

---

#### [MODIFY] [mqtt_handler.h](file:///D:/Graduation%20Project/iot/ProjectGrd-1/ProjectGrd-1/include/mqtt_handler.h)
- Add `pump1`, `pump2` fields to `ActuatorCommand` struct

---

#### [MODIFY] [mqtt_handler.cpp](file:///D:/Graduation%20Project/iot/ProjectGrd-1/ProjectGrd-1/src/mqtt_handler.cpp)
- Parse `actions.pump1` and `actions.pump2` from incoming JSON
- Add `"water"` object to `publishSensorData()` JSON payload

---

#### [MODIFY] [main.cpp](file:///D:/Graduation%20Project/iot/ProjectGrd-1/ProjectGrd-1/src/main.cpp)
- Update `actuators.begin()` to pass pump + LED pins
- Update `processAiCommands()` to pass pump commands and call `setStatusLeds()`
- Update `processFailsafe()` to set red LED in emergency
- Update `printTelemetry()` to show pump status and water level
- Update `publishSensorData()` to include water level data

---

### Python AI Module

---

#### [MODIFY] [decision_engine.py](file:///D:/Graduation%20Project/cv/Real-Time-Smoke-Fire-Detection-YOLO11/src/decision_engine.py)
- Add `"pump1"` and `"pump2"` to `_state_actions`:
  - SAFE/GAS_LEAK/SMOKE: `"OFF"`
  - FIRE: `"ON"`

---

#### [MODIFY] [fire_detector.py](file:///D:/Graduation%20Project/cv/Real-Time-Smoke-Fire-Detection-YOLO11/src/fire_detector.py)
- Add `self.last_confidence` attribute to expose detection confidence

---

#### [MODIFY] [main_integrated.py](file:///D:/Graduation%20Project/cv/Real-Time-Smoke-Fire-Detection-YOLO11/src/main_integrated.py)
- Publish raw detection directly to `sffs/ai/detection` after `fire_detector.process_frame()`
- Track `last_published_detection` to avoid redundant publishes

---

### Node-RED Dashboard

---

#### [MODIFY] [nodered_dashboard_flow.json](file:///D:/Graduation%20Project/iot/nodered_dashboard_flow.json)
- Add **Pump 1** and **Pump 2** text widgets in Actuator Status group
- Add **Water Level** gauge (0–100%) in a new "Water Tank" group
- Add **MQTT subscriber** for `sffs/ai/detection`
- Add **AI Camera Detection** text widget in System Status group
- Add **ui_toast** notification for fire/smoke alerts
- Update `parse_status` function to extract pump1/pump2
- Update `parse_sensors` to extract water level
- Update `log_event` to include pump states

---

### Documentation

---

#### [MODIFY] [walkthrough.md](file:///C:/Users/Marawan%20Mohamed/.gemini/antigravity/brain/deb91dfe-cc2f-4a2c-b128-dedde3e9ca7a/walkthrough.md)
- Add all new components to the hardware wiring table and pinout diagram
- Add pump, LED, and water level to the architecture description
- Update actuator status telemetry section

---

## Verification Plan

### Compile Test
```bash
cd "D:\Graduation Project\iot\ProjectGrd-1\ProjectGrd-1"
pio run
```

### Manual Verification
1. **LEDs**: Upload firmware → Green LED should light up (SAFE state)
2. **Pumps**: Send FIRE command via `mosquitto_pub` → hear relay click, both pumps ON, Red LED ON
3. **Water Level**: Cover/uncover HC-SR04 → Serial Monitor shows distance + percentage
4. **Dashboard**: Open Node-RED → verify water gauge, pump indicators, AI detection widgets
5. **AI Detection Path**: Show fire to camera → toast notification + status text on dashboard
