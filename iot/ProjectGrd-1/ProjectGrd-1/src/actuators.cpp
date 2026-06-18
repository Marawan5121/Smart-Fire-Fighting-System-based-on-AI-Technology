#include "actuators.h"
#include "config.h"

ActuatorController::ActuatorController()
    : _gasValveOpen(true), _doorOpen(false), _buzzerActive(false),
      _pump1Active(false), _pump2Active(false),
      _buzzerMode(BUZZER_MODE_OFF), _buzzerLastToggle(0), _buzzerPinState(false) {}

void ActuatorController::begin(int buzzerPin,
                                int pump1Pin, int pump2Pin,
                                int ledGreenPin, int ledOrangePin, int ledRedPin) {
    _buzzerPin    = buzzerPin;
    _pump1Pin     = pump1Pin;
    _pump2Pin     = pump2Pin;
    _ledGreenPin  = ledGreenPin;
    _ledOrangePin = ledOrangePin;
    _ledRedPin    = ledRedPin;

    // ---- PCA9685 PWM servo driver (shared hardware I2C bus) — HARDENED INIT ----
    // Servo no-response fix: bring up the I2C master FIRST, PROBE the PCA9685 so a
    // wiring/power fault is reported (instead of silently failing to a dead bus), then
    // run the exact Adafruit bring-up sequence and let the prescaler settle BEFORE any
    // angle is commanded by setAllSafe().
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);                            // 400 kHz fast-mode I2C

    // I2C presence probe: if this NACKs, the chip is unpowered or miswired and NO
    // servo will ever move regardless of the PWM calls below — surface it loudly.
    Wire.beginTransmission(PCA9685_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[Actuators] ⚠️ PCA9685 NOT detected on I2C @0x" +
                       String(PCA9685_I2C_ADDR, HEX) +
                       " — check SDA=GPIO" + String(I2C_SDA) +
                       "/SCL=GPIO" + String(I2C_SCL) +
                       ", 3.3V logic VCC, AND the separate V+ servo supply.");
    }

    _pwm = Adafruit_PWMServoDriver(PCA9685_I2C_ADDR, Wire);
    _pwm.begin();
    _pwm.setOscillatorFrequency(PCA9685_OSC_FREQ);    // 27 MHz internal osc (Adafruit calibration)
    _pwm.setPWMFreq(SERVO_PWM_FREQ);                  // 50 Hz analog-servo refresh
    Wire.setClock(400000);                            // re-assert: _pwm.begin() can reset Wire to 100 kHz
    delay(10);                                        // let the prescaler/oscillator settle before first angle

    // Configure buzzer pin
    pinMode(_buzzerPin, OUTPUT);
    digitalWrite(_buzzerPin, LOW);

    // Pump relays (Active-LOW: HIGH = OFF). GHOST-TRIGGER FIX: preset the latch HIGH
    // *before* switching the pin to OUTPUT, so it never momentarily drives LOW (which
    // an active-LOW relay reads as ON) during init. Then drive HIGH again to confirm.
    // NOTE: for the pre-setup() reset window the GPIO still floats — add an external
    // 10k pull-UP on each relay IN line (especially GPIO 23) to fully kill boot noise.
    digitalWrite(_pump1Pin, HIGH);
    pinMode(_pump1Pin, OUTPUT);
    digitalWrite(_pump1Pin, HIGH);  // Pump 1 OFF at boot
    digitalWrite(_pump2Pin, HIGH);
    pinMode(_pump2Pin, OUTPUT);
    digitalWrite(_pump2Pin, HIGH);  // Pump 2 OFF at boot

    // Configure status LED pins. ALL THREE are ACTIVE-HIGH (Common-Cathode): HIGH = ON,
    // LOW = OFF. (Green is NOT inverted — that assumption caused the Green+Red co-lit bug.)
    pinMode(_ledGreenPin, OUTPUT);
    digitalWrite(_ledGreenPin, LOW);    // green OFF at boot (active-high)
    pinMode(_ledOrangePin, OUTPUT);
    digitalWrite(_ledOrangePin, LOW);
    pinMode(_ledRedPin, OUTPUT);
    digitalWrite(_ledRedPin, LOW);

    // Set safe defaults at boot (positions both servos; all pumps OFF).
    setAllSafe();

    Serial.println("[Actuators] Initialized: PCA9685@0x" + String(PCA9685_I2C_ADDR, HEX) +
                   " servos ch0-1" +
                   " | Buzzer=GPIO" + String(_buzzerPin) +
                   " Pump1=GPIO" + String(_pump1Pin) +
                   " Pump2=GPIO" + String(_pump2Pin) +
                   " | LEDs=GPIO" + String(_ledGreenPin) + "/" +
                   String(_ledOrangePin) + "/" + String(_ledRedPin));
}

// ==========================================
// PCA9685 servo helper — map degrees → 12-bit pulse count
// ==========================================
void ActuatorController::setServoAngle(uint8_t channel, int angle) {
    angle = constrain(angle, 0, 180);
    uint16_t count = map(angle, 0, 180, SERVO_PWM_MIN, SERVO_PWM_MAX);
    _pwm.setPWM(channel, 0, count);
}

// ==========================================
// Gas Valve Servo (PCA9685 ch 0)
// ==========================================

void ActuatorController::openGasValve() {
    setServoAngle(SERVO_GAS_VALVE_CH, SERVO_GAS_VALVE_OPEN);
    _gasValveOpen = true;
}

void ActuatorController::closeGasValve() {
    setServoAngle(SERVO_GAS_VALVE_CH, SERVO_GAS_VALVE_CLOSED);
    _gasValveOpen = false;
}

void ActuatorController::setGasValve(const String& command) {
    if (command == "OPEN") {
        openGasValve();
    } else if (command == "CLOSE") {
        closeGasValve();
    }
}

// ==========================================
// Door / Window Servo (PCA9685 ch 1)
// ==========================================

void ActuatorController::openDoors() {
    setServoAngle(SERVO_DOORS_CH, SERVO_DOOR_OPEN);
    _doorOpen = true;
}

void ActuatorController::closeDoors() {
    setServoAngle(SERVO_DOORS_CH, SERVO_DOOR_CLOSED);
    _doorOpen = false;
}

void ActuatorController::setDoors(const String& command) {
    if (command == "OPEN") {
        openDoors();
    } else if (command == "CLOSE") {
        closeDoors();
    }
}

// ==========================================
// Buzzer (non-blocking pattern engine)
// ==========================================

void ActuatorController::buzzerOn()  { setBuzzerMode(BUZZER_MODE_CONTINUOUS); }
void ActuatorController::buzzerOff() { setBuzzerMode(BUZZER_MODE_OFF); }

void ActuatorController::setBuzzerMode(BuzzerMode mode) {
    _buzzerMode = mode;
    _buzzerActive = (mode != BUZZER_MODE_OFF);

    // Apply the immediate, steady-state pin level. INTERMITTENT then toggles
    // itself forward in updateBuzzer(); CONTINUOUS/OFF need no further ticking.
    switch (mode) {
        case BUZZER_MODE_OFF:
            digitalWrite(_buzzerPin, LOW);
            _buzzerPinState = false;
            break;
        case BUZZER_MODE_CONTINUOUS:
            digitalWrite(_buzzerPin, HIGH);
            _buzzerPinState = true;
            break;
        case BUZZER_MODE_INTERMITTENT:
            digitalWrite(_buzzerPin, HIGH);   // start the first beep immediately
            _buzzerPinState  = true;
            _buzzerLastToggle = millis();
            break;
    }
}

void ActuatorController::updateBuzzer() {
    // Only the intermittent pattern needs periodic servicing.
    if (_buzzerMode != BUZZER_MODE_INTERMITTENT) return;

    if (millis() - _buzzerLastToggle >= BUZZER_BEEP_INTERVAL_MS) {
        _buzzerLastToggle = millis();
        _buzzerPinState = !_buzzerPinState;
        digitalWrite(_buzzerPin, _buzzerPinState ? HIGH : LOW);
    }
}

// ==========================================
// Water Pumps (Active LOW Relay)
// ==========================================

void ActuatorController::pump1On() {
    digitalWrite(_pump1Pin, LOW);   // Active LOW: LOW = relay ON
    _pump1Active = true;
}

void ActuatorController::pump1Off() {
    digitalWrite(_pump1Pin, HIGH);  // Active LOW: HIGH = relay OFF
    _pump1Active = false;
}

void ActuatorController::setPump1(const String& command) {
    if (command == "ON") {
        pump1On();
    } else if (command == "OFF") {
        pump1Off();
    }
}

void ActuatorController::pump2On() {
    digitalWrite(_pump2Pin, LOW);   // Active LOW: LOW = relay ON
    _pump2Active = true;
}

void ActuatorController::pump2Off() {
    digitalWrite(_pump2Pin, HIGH);  // Active LOW: HIGH = relay OFF
    _pump2Active = false;
}

void ActuatorController::setPump2(const String& command) {
    if (command == "ON") {
        pump2On();
    } else if (command == "OFF") {
        pump2Off();
    }
}

// ==========================================
// Status LEDs (only one color ON at a time)
// ==========================================

void ActuatorController::setStatusLeds(const String& state) {
    // LEDs are confirmed Common-Cathode (ACTIVE-HIGH): HIGH = ON, LOW = OFF.
    //
    // STEP 1 — BLANK ALL THREE to LOW first. Wiping every colour before driving the new
    //          one guarantees ABSOLUTE EXCLUSIVITY: no pin can bleed across a state
    //          change (this is what kills the Green+Red co-activation).
    digitalWrite(_ledGreenPin,  LOW);
    digitalWrite(_ledOrangePin, LOW);
    digitalWrite(_ledRedPin,    LOW);

    // STEP 2 — drive EXACTLY ONE pin HIGH for the active state.
    if (state == "SAFE") {
        digitalWrite(_ledGreenPin,  HIGH);   // 🟢 SAFE
    } else if (state == "SENSOR_ALERT") {
        digitalWrite(_ledOrangePin, HIGH);   // 🟠 high-temp early warning (from Python AI)
    } else if (state == "FIRE" || state == "MANUAL") {
        digitalWrite(_ledRedPin,    HIGH);   // 🔴 confirmed emergency
    }
    // else (unknown/empty): all three stay LOW → fail-dark.
}

// ==========================================
// State → LED + Buzzer policy (single source of truth)
// ==========================================
void ActuatorController::applyState(const String& state) {
    setStatusLeds(state);

    if (state == "FIRE" || state == "MANUAL") {
        setBuzzerMode(BUZZER_MODE_CONTINUOUS);
    } else if (state == "SENSOR_ALERT") {
        setBuzzerMode(BUZZER_MODE_INTERMITTENT);
    } else {
        setBuzzerMode(BUZZER_MODE_OFF);
    }
}

// ==========================================
// Batch Commands (mechanical actuators only)
// ==========================================

void ActuatorController::applyCommands(const String& gasValve,
                                        const String& doors,
                                        const String& pump1,
                                        const String& pump2) {
    setGasValve(gasValve);
    setDoors(doors);
    setPump1(pump1);
    setPump2(pump2);

    Serial.println("[Actuators] Applied -> Valve:" + gasValve +
                   " Doors:" + doors +
                   " Pump1:" + pump1 + " Pump2:" + pump2);
}

void ActuatorController::setAllSafe() {
    openGasValve();
    closeDoors();
    pump1Off();
    pump2Off();           // both suppression pumps OFF in SAFE
    applyState("SAFE");   // green LED + buzzer OFF
    Serial.println("[Actuators] All set to SAFE defaults.");
}

void ActuatorController::setEmergency() {
    closeGasValve();
    openDoors();
    pump1On();
    pump2On();            // both suppression pumps ON (dry-run cutoff still applies)
    applyState("FIRE");   // red LED + continuous buzzer
    Serial.println("[Actuators] EMERGENCY mode activated!");
}
