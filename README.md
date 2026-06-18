# Smart Fire Fighting System (SFFS) — AI + IoT Zonal Suppression Platform

A distributed fire detection and suppression system that pairs a **YOLOv11 edge-vision pipeline** with a **real-time ESP32 controller** over an MQTT message bus. The vision tier provides probabilistic fire/smoke detection; the embedded tier provides deterministic, level-driven zonal suppression across four rooms with nine servo-actuated barriers and four independent water pumps.

## Engineering Abstract

The platform separates *perception* from *actuation*. A Python process runs YOLOv11 inference on a live camera feed and fuses the visual verdict with the ESP32's gas/flame telemetry in a priority-ordered decision engine, publishing a single fused command to the broker. The ESP32 executes a stateless, level-driven control loop: every iteration it recomputes the complete desired output state from current sensor levels and the latest AI command, then synchronizes the hardware only where it differs (edge-cached I/O). Local gas, flame, and manual-button paths actuate suppression directly inside the loop, so the controller remains fully autonomous if the network or the AI tier is lost.

### Dual-Core Architecture

- **Perception core (Python / YOLOv11):** camera inference → `decision_engine.py` evidence fusion → MQTT command publication. Runs on the host alongside the broker.
- **Control core (ESP32 / Arduino):** sensor acquisition (4× MQ on ADC1, IR flame, DHT22, HC-SR04, 4 buttons) → zonal evaluation → 9× PWM servos via PCA9685 + 4 pump relays + status LEDs/buzzer. A connectivity FSM tracks link health for telemetry without affecting suppression autonomy.

## Directory Layout

```
cv/
├── README.md                         # this file
├── requirements.txt                  # unified Python + PlatformIO manifest
│
├── Real-Time-Smoke-Fire-Detection-YOLO11/    # Perception core (Python)
│   ├── src/
│   │   ├── main_integrated.py        # pipeline entry point
│   │   ├── fire_detector.py          # YOLOv11 inference wrapper
│   │   ├── decision_engine.py        # gas/flame/vision fusion
│   │   ├── mqtt_client.py            # broker I/O
│   │   ├── occupancy_tracker.py      # people counting / occupancy
│   │   ├── notification_service.py   # alerting
│   │   └── config.py                 # thresholds, topics, broker
│   ├── models/                       # YOLOv11 weights
│   ├── requirements.txt              # module-local Python deps
│   └── .env.example                  # credentials template
│
└── iot/                              # Control core (ESP32 firmware)
    ├── nodered_dashboard_flow.json   # Node-RED dashboard import
    └── ProjectGrd-1/ProjectGrd-1/
        ├── platformio.ini            # board + library manifest
        ├── mosquitto.conf            # broker configuration
        ├── include/
        │   ├── config.h              # pin map, servo map, thresholds
        │   ├── sensors.h  actuators.h  mqtt_handler.h
        │   ├── failsafe.h  filters.h  wifi_manager.h
        │   └── secrets.example.h     # WiFi/MQTT credentials template
        ├── src/
        │   ├── main.cpp              # level-driven control loop
        │   ├── sensors.cpp  actuators.cpp
        │   ├── mqtt_handler.cpp  failsafe.cpp  wifi_manager.cpp
        └── docs/
            ├── architecture_documentation.md
            └── updated_power_budget.md
```

## Quick Start

### 1. MQTT Broker (Mosquitto)

```bash
# From the firmware directory; broker listens on 1883
cd iot/ProjectGrd-1/ProjectGrd-1
mosquitto -c mosquitto.conf -v
```

### 2. ESP32 Firmware (PlatformIO / VS Code)

```bash
cd iot/ProjectGrd-1/ProjectGrd-1

# Provide credentials (one-time)
cp include/secrets.example.h include/secrets.h
#   edit include/secrets.h:
#     WIFI_SSID, WIFI_PASSWORD
#     MQTT_BROKER  -> the LAN IP of the host running Mosquitto

# Build, flash, and open the serial monitor
pio run                       # compile
pio run --target upload       # flash over USB
pio device monitor -b 115200  # live telemetry
```

### 3. Edge AI — YOLOv11 Inference Pipeline (Python)

```bash
cd Real-Time-Smoke-Fire-Detection-YOLO11

python -m venv .venv
# Windows:
.venv\Scripts\activate
# Linux/macOS:
source .venv/bin/activate

pip install -r ../requirements.txt

cp .env.example .env          # set broker host + camera source
python src/main_integrated.py
```

### 4. Node-RED Dashboard

```
1. Open Node-RED  ->  http://127.0.0.1:1880
2. Menu  ->  Import  ->  select  iot/nodered_dashboard_flow.json
3. Set the MQTT broker node to the Mosquitto host:1883
4. Deploy  ->  open the dashboard UI tab
```

## Hardware Pinout — Quick Reference

| Domain | Signal | GPIO | Mode / Convention |
|--------|--------|------|-------------------|
| Gas | MQ-2 / MQ-5 / MQ-6 / MQ-7 | 32 / 33 / 34 / 35 | ADC1 analog |
| Flame | IR flame | 27 | `INPUT_PULLUP`, active-LOW |
| Env | DHT22 | 26 | 1-wire |
| Water | HC-SR04 TRIG / ECHO | 15 / 36 | ECHO via 5 V→3.3 V divider |
| Buttons | Room 1–4 | 16 / 17 / 14 / 19 | `INPUT_PULLDOWN`, active-HIGH |
| Pumps | Relay 1–4 | 5 / 23 / 12 / 13 | active-LOW (GPIO 12 needs ext. pull-down) |
| LEDs | Green / Red | 18 / 25 | active-HIGH |
| Buzzer | Active buzzer | 4 | continuous tone |
| I2C | SDA / SCL (PCA9685 @0x40) | 21 / 22 | 9 servos, 50 Hz |

PCA9685 channels: `0` gas valve · `1–4` room doors · `5–6` corridors (GF/1F) · `7–8` windows (R3/R4). See `iot/ProjectGrd-1/ProjectGrd-1/docs/architecture_documentation.md` for full specifications and `docs/updated_power_budget.md` for supply sizing.
