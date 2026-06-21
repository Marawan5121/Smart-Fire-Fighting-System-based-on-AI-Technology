# Project Context — SFFS Graduation Thesis Book

**Title:** Smart Fire Fighting System based on AI Technology (SFFS)
**Institution:** Modern Academy for Engineering and Technology — Department of Electronics and Communication Technology
**Supervisor:** Dr. Maha Gaber
**Authors:** Ahmed Ali Abd El-Baky (4220170), Ahmed Mohamed Khodary (4190978), Eman Mohamed Ibrahim (4220494), Marawan Mohamed Ragab (4220258), Manar Hamed Abd El-Wahab (4220540), Nesrin Gamal Mahmoud Elsayed (4220164), Rawnaa Elsherbiny Khalel Elsherbiny (4220536)
**Document type:** Final graduation thesis book (academic)
**Authoring model:** Independent Markdown chapters → Pandoc → Microsoft Word (.docx)
**Status:** Initialization phase (no chapter content yet)

---

## 0. SOURCE-OF-TRUTH PRECEDENCE (non-negotiable)

The **active production codebase on disk** (`iot/.../src/`, `iot/.../include/`, `platformio.ini`, and the Python `Real-Time-Smoke-Fire-Detection-YOLO11/src/`) is the absolute source of truth. Where the converted `term1_draft.md`, the BOM PDF, or any earlier verbal description contradicts the code, **the code wins** and the contradicting text is discarded. All hardware, parameters, and logic below were re-extracted from the live code this session.

## 1. Verified Hardware Architecture (from `config.h` — authoritative)

**Main controller:** ESP32-WROOM-32 (Classic, 38-pin).

**Perception layer**
- MQ gas/smoke array on ADC1: MQ-2 (GPIO 32), MQ-5 (GPIO 33), MQ-6 (GPIO 34), MQ-7 (GPIO 35), EMA-filtered.
- IR optical flame sensor: GPIO 27, `INPUT_PULLUP`, active-LOW (LOW = fire).
- DHT22 temperature/humidity: GPIO 26.
- HC-SR04 ultrasonic water-level sensor: TRIG GPIO 15, ECHO GPIO 36 (5 V→3.3 V divider).

**Actuation & suppression layer**
- PCA9685 16-ch PWM driver (I2C @ 0x40, SDA 21 / SCL 22), 50 Hz, **9 servos** (counts 102→491 for 0°→180°):
  - ch0 gas main valve (open 0° / closed 180°)
  - ch1–4 room doors 1–4 (closed 0° / open 90°)
  - ch5 ground-floor corridor, ch6 first-floor corridor (closed 0° / open 90°)
  - ch7 Room 3 window, ch8 Room 4 window (open 90° / closed 0°)
- **4 water-pump relays**, active-LOW: Room 1 GPIO 5, Room 2 GPIO 23, Room 3 GPIO 12 (boot-strapping pin — external pull-down required), Room 4 GPIO 13.
- 2 status LEDs (active-HIGH): green/SAFE GPIO 18, red/FIRE GPIO 25.
- Active buzzer: GPIO 4 (continuous tone on fire).
- 4 manual alarm buttons (`INPUT_PULLDOWN`, active-HIGH): Room 1 GPIO 16, Room 2 GPIO 17, Room 3 GPIO 14, Room 4 GPIO 19; 300 ms debounce.

**Excluded — must NOT appear anywhere in the thesis** (removed from architecture; absent from code): GSM SIM800L, MPU6050 accelerometer/gyro, ACS712 current sensor. Also absent from the active design (legacy draft artifacts only): PIR motion sensors, thermal cameras, YOLOv5, the 360°-rotating single-servo concept, and the BAMB escape system.

**Key parameters (config.h):** `GAS_THRESHOLD = 2000`; MQ warm-up 60 s; pump dry-run cutoff < 10 %, resume ≥ 20 %; EMA α = 0.15; tank height 20 cm; MQTT keepalive 60 s, reconnect backoff 5–30 s.

## 2. Software & AI Stack (from code)

- **Firmware:** modular OOP PlatformIO C++ across `src/` + `include/`. Modules: `wifi_manager`, `mqtt_handler`, `sensors`, `actuators`, `failsafe`, with `main.cpp` orchestrating a pure level-driven control loop (edge-cached `ZoneCommand` via `memcmp`; per-room fire = button OR MQ > 2000; global override = flame active-LOW OR MQTT `state == "FIRE"`; water-level hysteresis; connectivity FSM NORMAL/DEGRADED/RECOVERY for telemetry).
- **Middleware:** Mosquitto MQTT broker `localhost:1883`, MQTT v3.1.1. Topics: `sffs/sensors/data`, `sffs/system/status`, `sffs/system/heartbeat`, `sffs/manual/alarm`. Structured JSON payloads.
- **Dashboard:** Node-RED "SFFS Control Room" — real-time telemetry (WiFi RSSI, free heap, uptime), water-tank level, per-room state, manual override injection.
- **Vision core:** GPU-accelerated **YOLOv11** via CUDA FP16, weights `best_nano_111.pt` (resolved from `models/`). Python modules: `main_integrated`, `fire_detector`, `decision_engine`, `mqtt_client`, `occupancy_tracker`, `people_counter`, `notification_service`, `config`. Line-crossing occupancy tracker (`OCCUPANCY_LINE_X = 300`). Decision engine fuses gas/flame/vision; `GAS_THRESHOLD = 2000` mirrors the firmware.
- **Alerts:** automated Telegram bot (`sffs_bot`) via `notification_service.py`, 45 s alert cooldown.

## 3. Deliverable & File Workflow Standards

- Each chapter is an independent Markdown file (`chapter1.md`, …) in `docs/thesis/`, compiled to one `.docx` via Pandoc.
- Source documents converted to Markdown before analysis: Word → Pandoc; PDF → `pdftotext`/`pdfplumber`/`PyPDF2` (never raw PDF binary).

## 3a. Thesis Structure (4-CHAPTER MANDATE — authoritative)

The book consists strictly of **four** comprehensive chapters (revised from an earlier 7-chapter plan):

1. **Chapter 1 — Introduction & Project Overview** (motivation, problem statement, objectives & scope, proposed system overview, thesis organization).
2. **Chapter 2 — Hardware Architecture & Component Specifications** (ESP32, MQ-2/5/6/7 matrix, active-LOW IR flame, DHT22, HC-SR04, PCA9685, relays; full pin map + electrical specs).
3. **Chapter 3 — Full System Integration & Implementation** (wiring, PlatformIO C++ modules `wifi_manager`/`mqtt_handler`/`sensors`/`actuators`/`failsafe`, Mosquitto MQTT protocol + JSON contract, Node-RED backend, YOLOv11 CUDA integration).
4. **Chapter 4 — Empirical Testing, Results, Validation & Conclusion** (YOLOv11 performance, telemetry logs, Telegram notifications, end-to-end validation, conclusion + future outlook).

Section 1.5 of Chapter 1 ("Thesis Book Organization") outlines exactly these four chapters. Section numbers are embedded in heading text (e.g., `## 1.5 ...`); the final Pandoc compile therefore must **not** also pass `--number-sections` (avoids double numbering), consistent with the manual `Figure X.Y` caption convention.

## 4. Dynamic Index Compliance (TOC / TOF / TOT)

- No manually typed TOC strings, leader dots, or static page numbers.
- TOC: nested semantic Markdown headings (`#`, `##`, `###`) only.
- TOF/TOT: standardized caption syntax `![Figure X.Y: Caption Text](path)` for native Word Figure/Table registration.

## 5. Compilation & docx-js / OpenXML Rendering Standards (apply only at final render)

A4 (210 × 297 mm) in exact DXA; landscape via portrait dims + `PageOrientation.LANDSCAPE`; no `\n` in runs (discrete `Paragraph`s); bullets via `LevelFormat.BULLET` (no unicode glyphs); `PageBreak` inside a `Paragraph`; `ImageRun` with explicit `type`; table widths absolute DXA (never `PERCENTAGE`); dual-width tables (`columnWidths[]` = sum, cells match); cell `margins {top:80,bottom:80,left:120,right:120}`; `ShadingType.CLEAR` only; dividers via paragraph bottom border `{SINGLE, size:6, color:"2E75B6", space:1}` (no single-cell-table rules; footers use tab stops); headings via `HeadingLevel` + built-in style overrides (`"Heading1"`…) with explicit `outlineLevel` (0=H1).

## 6. Document Protection Policy

Output `.docx` remains fully unprotected — no read-only, locking, restricted editing, or encryption. User retains 100 % manual edit capability.
