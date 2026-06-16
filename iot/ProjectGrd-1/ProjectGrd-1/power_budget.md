# ⚡ SFFS Power Budget & Supply Calculation

## Three Independent Power Rails

Your system requires **3 separate power sources**. Never try to run everything from USB — it will brown-out and crash.

```
                    ┌─────────────────────────────────────────────────┐
                    │           MAIN 5V POWER SUPPLY (3A+)           │
                    │           (Switching adapter or PSU)            │
                    └──────┬──────────────────────────┬───────────────┘
                           │                          │
                    ┌──────▼──────┐            ┌──────▼──────┐
                    │  ESP32 VIN  │            │ Shared 5V   │
                    │  (5V input) │            │ Bus (direct)│
                    └──────┬──────┘            └──────┬──────┘
                           │                          │
                     ESP32 onboard              4× MQ Sensors
                     AMS1117 → 3.3V             2× SG90 Servos
                           │                    HC-SR04
                     ESP32 core                 Buzzer
                     DHT22                      Relay Module
                     MPU6050                    3× LEDs
                     Flame IR
                     
    ┌──────────────────────────────────────┐
    │    SEPARATE BUCK CONVERTER           │
    │    Input: 5V → Output: 3.7–4.2V     │
    │    (Must handle 2A bursts)           │
    │    + 1000µF electrolytic cap         │
    └──────────────┬───────────────────────┘
                   │
              SIM800L GSM Module

    ┌──────────────────────────────────────┐
    │    SEPARATE PUMP POWER SUPPLY        │
    │    (Matched to your pump specs)      │
    │    Switched through relay NO contacts│
    └──────────────┬───────────────────────┘
                   │
              Pump 1 + Pump 2
```

---

## Rail 1 — Main 5V Bus (Everything except SIM800L and Pumps)

| # | Component | Voltage | Typical Current | Peak Current | Notes |
|---|---|---|---|---|---|
| 1 | **ESP32 WROOM-32** | 3.3V (via onboard reg) | 80 mA | 240 mA | Peak during WiFi TX bursts |
| 2 | **MQ-2** (Smoke) | 5V | 150 mA | 160 mA | Heater coil ~33Ω (always on) |
| 3 | **MQ-5** (LPG) | 5V | 150 mA | 160 mA | Heater coil ~33Ω (always on) |
| 4 | **MQ-6** (Butane) | 5V | 150 mA | 160 mA | Heater coil ~33Ω (always on) |
| 5 | **MQ-7** (CO) | 5V | 150 mA | 160 mA | Heater coil ~33Ω (always on) |
| 6 | **DHT22** | 3.3V | 1.5 mA | 2.5 mA | Very low power |
| 7 | **MPU6050** | 3.3V | 3.9 mA | 5 mA | Accelerometer + gyro |
| 8 | **IR Flame Sensor** | 5V | 15 mA | 20 mA | Comparator module |
| 9 | **SG90 Servo 1** (Gas Valve) | 5V | 10 mA | 250 mA | Peak when moving under load |
| 10 | **SG90 Servo 2** (Doors) | 5V | 10 mA | 250 mA | Peak when moving under load |
| 11 | **Active Buzzer** | 5V | 25 mA | 30 mA | Only ON during alarm |
| 12 | **2-Ch Relay Module** (coils) | 5V | 70 mA | 150 mA | ~75mA per coil when active |
| 13 | **HC-SR04** Ultrasonic | 5V | 2 mA | 15 mA | 15mA during measurement pulse |
| 14 | **LED Green** | 3.3V | 0 mA | 20 mA | Only 1 LED is ON at a time |
| 15 | **LED Orange** | 3.3V | 0 mA | 20 mA | Only 1 LED is ON at a time |
| 16 | **LED Red** | 3.3V | 0 mA | 20 mA | Only 1 LED is ON at a time |
| | | | | | |
| | **TOTAL (Typical — SAFE state)** | | **≈ 820 mA** | | Gas heaters + ESP32 + sensors, no servos moving |
| | **TOTAL (Peak — FIRE state)** | | | **≈ 1,640 mA** | All actuators active + servos moving + WiFi TX |

> [!IMPORTANT]
> The **4 MQ gas sensor heaters alone** draw **600 mA continuously**. This is the largest power consumer in your system. They cannot be turned off — the heater must run 24/7 for accurate readings.

### ✅ Recommended Main Power Supply

| Spec | Value | Why |
|---|---|---|
| **Voltage** | **5V DC** | Standard for ESP32 dev boards via VIN pin |
| **Current Rating** | **3A minimum** | 1.64A peak × 1.8 safety margin ≈ 3A |
| **Recommended** | **5V 3A switching adapter** | Common phone charger size — cheap and reliable |
| **Connector** | Micro-USB or barrel jack → VIN | Depends on your ESP32 board's input |

> [!WARNING]
> **USB from laptop is NOT enough!** USB 2.0 provides only 500mA, USB 3.0 provides 900mA. Your MQ sensors alone need 600mA. You **must** use an external 5V adapter rated at ≥3A.

---

## Rail 2 — SIM800L GSM Module (Separate!)

| Spec | Value | Notes |
|---|---|---|
| **Voltage** | **3.7 – 4.2V** | NOT 5V, NOT 3.3V! |
| **Idle current** | 15 mA | Registered on network, idle |
| **Average current** | 250–350 mA | During SMS or call |
| **Peak burst** | **2,000 mA (2A)** | During GSM TX bursts (577µs pulses) |

> [!CAUTION]
> The SIM800L **2A TX bursts** will crash your entire system if powered from the same 5V rail without proper isolation. It **must** have its own power source.

### ✅ Recommended SIM800L Power

| Spec | Value |
|---|---|
| **Method** | Buck converter (5V → 4.0V) OR single 18650 Li-ion cell (3.7V nominal) |
| **Current Rating** | **2A minimum** (to handle TX bursts) |
| **Decoupling** | **1000µF electrolytic capacitor** directly across SIM800L VCC/GND |
| **Example module** | LM2596 buck converter set to 4.0V output |

```
  Main 5V ──► LM2596 Buck Converter (set to 4.0V) ──┬──► SIM800L VCC
                                                      │
                                            1000µF cap ┤
                                                      │
                                                    GND ◄── SIM800L GND
```

**Alternative**: Use a single **18650 Li-ion battery** (3.7V, 2000mAh+) with a TP4056 charger module. This gives clean 3.7V and handles the 2A bursts naturally.

---

## Rail 3 — Water Pumps (Separate via Relay)

The pumps are switched ON/OFF by the relay module. Their power comes from a **completely separate supply** — not from the ESP32.

| Pump Type | Voltage | Current Each | Total (2 pumps) |
|---|---|---|---|
| Small DC mini pump (3-6V) | 5V | 150–300 mA | 300–600 mA |
| Standard DC pump (12V) | 12V | 250–500 mA | 500–1000 mA |

### ✅ Recommended Pump Power

| Spec | Value |
|---|---|
| **Voltage** | Match your pump's rated voltage (5V or 12V) |
| **Current** | 2× single pump rating (for both pumps running simultaneously) |
| **Example** | 12V 2A adapter for two 12V pumps |

```
  External Pump PSU (+) ──► Relay COM ──► Relay NO ──► Pump (+)
  External Pump PSU (-) ──────────────────────────────► Pump (-)
  
  (Relay switches the positive wire; negative goes direct)
```

> [!NOTE]
> **All GND rails must be connected together!** The ESP32 GND, relay GND, SIM800L GND, and pump PSU GND must share a common ground. Only the positive voltage rails are separated.

---

## Complete Shopping List — Power Components

| # | Item | Spec | Est. Price | Purpose |
|---|---|---|---|---|
| 1 | **5V 3A Switching Adapter** | 5V DC, barrel jack or Micro-USB | ~$3–5 | Main system power |
| 2 | **LM2596 Buck Converter** | Adjustable, set to 4.0V | ~$1–2 | SIM800L power |
| 3 | **1000µF Electrolytic Cap** | 6.3V or 10V rating | ~$0.20 | SIM800L decoupling |
| 4 | **Pump Power Supply** | Match pump voltage, 2A | ~$3–5 | Water pump power |
| 5 | **3× 220Ω Resistors** | For LEDs (current limiting) | ~$0.10 | LED protection |
| 6 | **1kΩ + 2kΩ Resistors** | For HC-SR04 ECHO voltage divider | ~$0.10 | ESP32 GPIO protection |
| 7 | **Breadboard Jumper Wires** | Male-to-male, male-to-female | ~$2 | Connections |

---

## Summary — What to Buy

```
┌────────────────────────────────────────────────────────────┐
│                    POWER SUPPLY SUMMARY                    │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  🔌 PSU #1: 5V 3A adapter ──► ESP32 VIN + all sensors    │
│             (≈ 1.6A peak load)                            │
│                                                            │
│  🔋 PSU #2: 5V → 4.0V buck converter (2A capable)        │
│             + 1000µF cap ──► SIM800L only                 │
│             (≈ 2A peak burst)                             │
│                                                            │
│  🔌 PSU #3: Separate adapter ──► Relay ──► 2× Pumps      │
│             (voltage matches your pump specs)              │
│                                                            │
│  ⏚  ALL GROUNDS CONNECTED TOGETHER (common ground)       │
│                                                            │
│  Total system power consumption:                           │
│    Typical (SAFE):  ≈ 1.1A @ 5V  (5.5W)                  │
│    Peak (FIRE):     ≈ 1.6A @ 5V  (8.0W) + pumps + GSM   │
│                                                            │
└────────────────────────────────────────────────────────────┘
```
