# 🔥 SFFS — ESP32 IoT Firmware (Edge Node)

Non-blocking, modular firmware for the **Smart Fire Fighting System (SFFS)** edge node.
The ESP32 reads gas/environmental/water sensors, fuses AI commands over MQTT, drives
actuators, and runs an **autonomous failsafe** that keeps protecting the site even when
WiFi, the broker, or the AI laptop all go down.

> [!IMPORTANT]
> **Target board: classic ESP32 WROOM-32 (38-pin) — NOT the ESP32-S3.**
> All analog (MQ) sensors are on **ADC1** because **ADC2 is unusable while WiFi is active**.
> The single source of pin truth is [`include/config.h`](include/config.h); this README mirrors it.

---

## 📁 Directory Structure

```
ProjectGrd-1/
├── include/
│   ├── config.h          # Pins, thresholds, intervals (mirrors Python constants)
│   ├── secrets.h         # 🔐 WiFi/MQTT/GSM credentials — GITIGNORED
│   ├── secrets.example.h # Template — copy to secrets.h
│   ├── filters.h         # O(1) EMA filter
│   ├── sensors.h         # MQ/DHT22/MPU6050/flame/HC-SR04/buttons
│   ├── gsm.h             # Non-blocking SIM800L FSM (verified delivery)
│   ├── wifi_manager.h    # WiFi auto-reconnect
│   ├── mqtt_handler.h    # JSON publish + AI command parse
│   ├── actuators.h       # Servos, pumps, buzzer, LEDs
│   └── failsafe.h        # Autonomous state machine
├── src/  (matching .cpp implementations)
└── platformio.ini        # board = esp32dev
```

---

## 🔐 First-Time Setup — Credentials (B7)

Credentials are **never** committed. Before building:

```bash
cp include/secrets.example.h include/secrets.h
# edit include/secrets.h with your real WiFi / broker IP / phone number
```

`secrets.h` is gitignored. `config.h` includes it and only ever references the macros —
no plaintext SSID, password, broker token, or phone number lives in tracked code.

---

## 🔌 Hardware Wiring — Classic ESP32 38-Pin (matches `config.h`)

> [!WARNING]
> - **MQ analog outputs can swing to 5V** → use a 1k/2k divider into the ADC1 pins.
> - **HC-SR04 ECHO is 5V** → mandatory 1k/2k divider to GPIO 36.
> - **GPIO 6–11** are flash — never use. **GPIO 34/35/36/39** are input-only (no pull-ups).

| Component | GPIO | Type | Notes |
|---|---|---|---|
| MQ-2 (smoke) | **32** | ADC1_CH4 | divider 5V→3.3V |
| MQ-5 (LPG) | **33** | ADC1_CH5 | divider 5V→3.3V |
| MQ-6 (butane) | **34** | ADC1_CH6 (in-only) | divider 5V→3.3V |
| MQ-7 (CO) | **35** | ADC1_CH7 (in-only) | divider 5V→3.3V |
| Flame IR | **27** | Digital IN | active LOW |
| DHT22 | **26** | Digital | 10k pull-up |
| MPU6050 SDA / SCL | **21 / 22** | I2C | 4.7k pull-ups |
| Boot LED | **2** | OUT | boot indicator only |
| Buzzer | **4** | OUT | active buzzer |
| Servo 1 — Gas valve | **18** | PWM | SG90 50Hz |
| Servo 2 — Doors | **19** | PWM | SG90 50Hz |
| Pump 1 relay | **5** | OUT | **active LOW** |
| Pump 2 relay | **23** | OUT | **active LOW** |
| 🟢 Green LED (SAFE) | **13** | OUT | **common-anode / inverted** (LOW=ON) — handled in `setStatusLeds()` |
| 🟠 Orange LED (SENSOR_ALERT) | **14** | OUT | active-high; intermittent-warning tier |
| 🔴 Red LED (FIRE/MANUAL) | **25** | OUT | confirmed emergency |
| HC-SR04 TRIG | **15** | OUT | |
| HC-SR04 ECHO | **36 (VP)** | in-only | ⚠️ divider required |
| Button 1 (Room 1) | **16** | IN_PULLDOWN | active HIGH (internal pulldown, wire to 3.3V) |
| Button 2 (Room 2) | **17** | IN_PULLDOWN | active HIGH (internal pulldown, wire to 3.3V) |
| ~~SIM800L~~ (REMOVED) | — | — | module removed; its old UART2 pins 16/17 are now the buttons |

> [!CAUTION]
> The previous revision of this document described an **ESP32-S3** with MQ sensors on GPIO 1–4
> and the SIM800L on GPIO 17/18. **That was wrong.** The firmware is a classic ESP32; wiring
> 5V analog into GPIO 1–4 (UART0/boot pins) will not work and can brick the upload. Use the
> table above.

---

## ⚡ Power Architecture — Three Independent Rails

See [`power_budget.md`](power_budget.md) for the full budget. Summary:

| Rail | Feeds | Why separate |
|---|---|---|
| **#1 — 5V ≥3A** | ESP32 VIN, 4× MQ heaters (≈600 mA continuous!), servos, buzzer, relay coils, HC-SR04, LEDs | ~1.6A peak |
| ~~**#2 — 4.0V buck + 1000µF cap**~~ | ~~SIM800L~~ (REMOVED) | rail no longer needed until the SIM800L is re-added |
| **#3 — separate PSU via relay** | both water pumps | matched to pump voltage; relay-switched |

**All grounds common.** USB power is *not* sufficient (MQ heaters alone exceed USB current).

> [!NOTE]
> **There is no programmatic solar / battery management in this firmware.** No MPPT, no
> charge-control, no fuel-gauge code exists or is planned in the ESP32 layer. If a solar
> panel is deployed, it is a **pure upstream hardware layer** feeding Rail #1's 5V adapter
> input and is completely invisible to the firmware. Document it as hardware-only; do not
> claim software solar management.

> [!CAUTION]
> The SG90 "gas valve" servo represents a **normally-closed solenoid valve** in a real
> deployment. It is a demo stand-in — state this to evaluators.

---

## 🧠 Firmware Behavior

### Non-blocking loop (B5)

`loop()` runs at full speed; every subsystem is `millis()`-gated and **nothing blocks**:

- **MQTT reconnect** is WiFi-gated with exponential backoff (5s → 30s cap) and the socket
  timeout is pinned to **2s** (`setSocketTimeout`). A down broker no longer freezes the loop;
  reconnects are never attempted while WiFi itself is down (the worst stall path).
- **HC-SR04** uses a bounded `pulseIn` timeout (`ULTRASONIC_TIMEOUT_US`) — an unplugged or
  silent sensor returns 0 instead of stalling.
- **GSM/SIM800L is currently REMOVED** — its UART2 pins (16/17) were reallocated to the manual
  buttons. The driver (`gsm.cpp`/`gsm.h`) is retained but not instantiated; all calls in
  `main.cpp` are commented out. The failsafe still trips local suppression when offline; it
  just no longer sends an SMS until the module is re-added.

### Sensor pipeline

EMA filter (α = 0.15) on all MQ channels and the ultrasonic distance. A **60s warm-up gate**
suppresses gas alarms at boot. Structured telemetry prints to the VS Code Serial Monitor
**every 2s** (gas, temp/humidity, flame, tilt, water %/distance, connectivity, mode, GSM/SMS
status, actuator states, heap, uptime).

### Decision → Actuator logic (3-tier control matrix)

Authoritative state comes from the AI over `sffs/system/status` (obeyed only in `NORMAL`).
The Python `DecisionEngine` decides the **state**; the firmware derives the **LED color and
buzzer pattern from that state** (`applyState`) and applies the mechanical actuators from the
`actions` block. Buzzer/LED are intentionally NOT raw MQTT tokens — that keeps a single
source of truth and lets the buzzer express patterns a bool cannot.

| State | Trigger | Gas Valve | Doors | Pump 1 | Pump 2 | Buzzer | LED |
|---|---|---|---|---|---|---|---|
| **SAFE** | all clear | OPEN | CLOSE | OFF | OFF | silent | 🟢 green |
| **SENSOR_ALERT** | high ambient **temperature only** | OPEN | CLOSE | OFF | OFF | **intermittent** | 🟠 orange |
| **FIRE** | **gas/smoke/flame sensors** · AI fire/smoke · manual button | CLOSE | OPEN | **ON** | **ON** | **continuous** | 🔴 red |

> [!IMPORTANT]
> **Gas/smoke/flame need no AI validation (#3).** The camera can't see a gas leak or ambient
> smoke, so any MQ-gas or IR-flame threshold crossing escalates **straight to FIRE** (gas valve
> CLOSED, both pumps ON). The ESP32 trips this **locally and immediately** via
> `processSensorEmergency()` — suppression fires even if the AI laptop is slow or offline, no
> 2s MQTT round-trip and no 30s failsafe wait. Precedence: **manual button → local sensor
> emergency → AI command → autonomous failsafe.** `SENSOR_ALERT` is now the temp-only soft tier.

> [!IMPORTANT]
> **HYBRID alarm latching** (`updateAlarmState()`):
> - **Manual buttons → fixed 5-second latch** (`ALARM_LATCH_MS`). A brief click sets
>   `manualAlarmActive` for exactly 5s then returns to SAFE (`updateManualLatch()`); pressing
>   again restarts the 5s. The manual→AI echo (`source="MANUAL_OVERRIDE"`) is ignored by the
>   continuous path, so manual is *exactly* 5s.
> - **Sensor + AI → CONTINUOUS suppression** (`autoFireActive`, no timer). Held for as long as
>   local gas/smoke/flame **or** a genuine AI camera FIRE is present; clears **only when BOTH**
>   are clear (AI back to SAFE/SENSOR_ALERT **and** sensors below threshold).
>
> Emergency = manual latch **OR** continuous danger. `setEmergency()`/`setAllSafe()` are
> edge-driven (once per transition). The offline `STANDALONE_ALERT` failsafe owns actuators
> when not in `NORMAL`.

**Manual alarm buttons** bypass the AI: a brief click (no hold needed) arms the 5s latch —
full suppression on every node — and the ESP32 sets `manual_trigger: 1` in its sensor JSON
(published immediately). The Python layer intercepts that flag and forces the whole system into
CONFIRMED FIRE. After 5s the latch clears and `manual_trigger` returns to 0.

### Autonomous failsafe state machine

```
            WiFi/MQTT lost                 sustained danger >30s (no AI)
 ┌────────┐ ───────────────► ┌──────────┐ ──────────────────────────► ┌──────────────────┐
 │ NORMAL │                  │ DEGRADED │                              │ STANDALONE_ALERT │
 │ obey AI│ ◄─────────────── │ monitor  │ ◄──────────────────────────  │ act locally      │
 └────────┘   hold 10s       └──────────┘     WiFi/MQTT restored        └──────────────────┘
      ▲           │                                                            │
      │      ┌──────────┐                                                      │
      └───── │ RECOVERY │ ◄────────────────────────────────────────────────────┘
             └──────────┘
```

**Edge-triggered actuators (B5):** `setEmergency()` is applied **once** on entering
`STANDALONE_ALERT`, and `setAllSafe()` **once** on entering `RECOVERY`. They are *not*
re-driven every loop iteration — that previously hammered the servos/relays thousands of
times per second and risked brown-outs.

**Pump dry-run protection (B6):** while standalone with pumps running, if
`waterLevelPct < PUMP_DRY_CUTOFF_PCT` (10%) the pumps are cut and a **refill alert** is
raised; they re-enable only above `WATER_REFILL_RESUME_PCT` (20%, hysteresis) — preventing
the pumps from burning out dry.

**Verified GSM SMS with retries (B6) — currently DISABLED:** the SIM800L has been removed, so
the standalone SMS path is commented out in `main.cpp`. The verified-delivery FSM
(`gsm.cpp`: reports `SUCCESS` only on the modem's final `OK` after `+CMGS:`, `FAILED` on
timeout, bounded retries) is retained intact for when the module is re-added.

---

## 📡 MQTT Interface

| Topic | Dir | QoS | Payload |
|---|---|---|---|
| `sffs/sensors/data` | ESP32 → | 0 | gas, env, motion, **water**, **`manual_trigger`**, meta (rssi/uptime/heap/mode/flame) |
| `sffs/system/status` | → ESP32 | 1 | `state` + `actions{gas_valve,doors,pump1,pump2}` (buzzer/LED derived from `state`; AI retains this) |
| `sffs/system/heartbeat` | ESP32 → | 0 | `{"status":"alive"}` (LWT: retained `offline`) |
| `sffs/manual/alarm` | ESP32 → | 0 | manual button events (room + active flag) |

> The AI side publishes `sffs/system/status` **retained + every 5s**, so a rebooting ESP32
> immediately re-receives the last authoritative command. The AI's own liveness/LWT lives on
> `sffs/ai/status` and is **never** parsed here as an actuator command.

---

## 🚀 Build & Run

```bash
cp include/secrets.example.h include/secrets.h   # then edit it
pio run                # compile
pio run --target upload
pio device monitor -b 115200
```

In VS Code/PlatformIO: **Ctrl+Alt+U** to upload. Watch the 2s telemetry dashboard, wait
out the 60s MQ warm-up, then verify WiFi/MQTT connect.

---

## 🔗 Single Source of Truth (drift prevention)

`GAS_THRESHOLD` (2000) and `TEMP_THRESHOLD` (50.0) in [`config.h`](include/config.h) **must**
match the mirror in `Real-Time-Smoke-Fire-Detection-YOLO11/src/decision_engine.py`. Change one,
update the other. The intended long-term fix is runtime config push over `sffs/system/config`.
