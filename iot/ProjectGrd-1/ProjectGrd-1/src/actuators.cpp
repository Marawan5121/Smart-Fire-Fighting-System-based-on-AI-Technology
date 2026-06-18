#include "actuators.h"

ActuatorController::ActuatorController()
    : _ledGreenState(-1), _ledRedState(-1), _buzzerState(-1) {
    for (int i = 0; i < 9; i++)          _servoAngle[i] = -1;       // force first write
    for (int i = 0; i < ROOM_COUNT; i++) _pumpState[i]  = false;
}

void ActuatorController::begin() {
    _pumpPins[0] = PUMP1_PIN;
    _pumpPins[1] = PUMP2_PIN;
    _pumpPins[2] = PUMP3_PIN;
    _pumpPins[3] = PUMP4_PIN;

    // ---- PCA9685 bring-up (shared hardware I2C bus) ----
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);                            // 400 kHz fast-mode I2C

    // Presence probe: a NACK here means the chip is unpowered or miswired and no
    // servo will move regardless of the PWM calls below -- surface it loudly.
    Wire.beginTransmission(PCA9685_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.println("[Actuators] PCA9685 NOT detected on I2C @0x" +
                       String(PCA9685_I2C_ADDR, HEX) +
                       " -- check SDA=GPIO" + String(I2C_SDA) +
                       "/SCL=GPIO" + String(I2C_SCL) +
                       ", 3.3V logic VCC, and the separate V+ servo supply.");
    }

    _pwm = Adafruit_PWMServoDriver(PCA9685_I2C_ADDR, Wire);
    _pwm.begin();
    _pwm.setOscillatorFrequency(PCA9685_OSC_FREQ);    // 27 MHz internal osc
    _pwm.setPWMFreq(SERVO_PWM_FREQ);                  // 50 Hz analog-servo refresh
    Wire.setClock(400000);                            // _pwm.begin() can reset Wire to 100 kHz
    delay(10);                                        // let the prescaler settle

    // Buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);

    // Pump relays (Active-LOW: HIGH = OFF). Preset the latch HIGH *before* switching
    // the pin to OUTPUT so it never momentarily drives LOW (which a relay reads as ON).
    for (int i = 0; i < ROOM_COUNT; i++) {
        digitalWrite(_pumpPins[i], HIGH);
        pinMode(_pumpPins[i], OUTPUT);
        digitalWrite(_pumpPins[i], HIGH);   // OFF at boot
        _pumpState[i] = false;
    }

    // Status LEDs (active-HIGH)
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_RED_PIN,   OUTPUT);
    digitalWrite(LED_GREEN_PIN, LOW);
    digitalWrite(LED_RED_PIN,   LOW);

    setSafeDefaults();   // positions every servo + LEDs to the SAFE state

    Serial.println("[Actuators] Initialized: PCA9685@0x" + String(PCA9685_I2C_ADDR, HEX) +
                   " servos ch0-8 | 4 pumps | Green=GPIO" + String(LED_GREEN_PIN) +
                   " Red=GPIO" + String(LED_RED_PIN));
}

// ==========================================
// PCA9685 servo helper -- map degrees -> 12-bit pulse, write only on change
// ==========================================
void ActuatorController::setServoAngle(uint8_t channel, int angle) {
    angle = constrain(angle, 0, 180);
    if (channel < 9 && _servoAngle[channel] == angle) return;   // edge-tracked
    uint16_t count = map(angle, 0, 180, SERVO_PWM_MIN, SERVO_PWM_MAX);
    _pwm.setPWM(channel, 0, count);
    if (channel < 9) _servoAngle[channel] = angle;
}

// ==========================================
// Servo groups
// ==========================================
void ActuatorController::setGasValve(bool emergency) {
    setServoAngle(SERVO_GAS_VALVE_CH,
                  emergency ? SERVO_GAS_VALVE_CLOSED : SERVO_GAS_VALVE_OPEN);
}

void ActuatorController::setRoomDoor(uint8_t roomIdx, bool open) {
    if (roomIdx >= ROOM_COUNT) return;
    uint8_t ch = SERVO_R1_DOOR_CH + roomIdx;   // ch 1..4
    setServoAngle(ch, open ? SERVO_DOOR_OPEN : SERVO_DOOR_CLOSED);
}

void ActuatorController::setCorridors(bool open) {
    int angle = open ? SERVO_DOOR_OPEN : SERVO_DOOR_CLOSED;
    setServoAngle(SERVO_CORRIDOR_GF_CH, angle);
    setServoAngle(SERVO_CORRIDOR_1F_CH, angle);
}

void ActuatorController::setRoomWindow(uint8_t roomIdx, bool fire) {
    // Only Room 3 (idx 2) and Room 4 (idx 3) have controllable windows.
    uint8_t ch;
    if (roomIdx == 2)      ch = SERVO_R3_WINDOW_CH;
    else if (roomIdx == 3) ch = SERVO_R4_WINDOW_CH;
    else return;
    setServoAngle(ch, fire ? SERVO_WINDOW_CLOSED : SERVO_WINDOW_OPEN);
}

// ==========================================
// Pumps (Active-LOW relay), write only on change
// ==========================================
void ActuatorController::setPump(uint8_t roomIdx, bool on) {
    if (roomIdx >= ROOM_COUNT) return;
    if (_pumpState[roomIdx] == on) return;            // edge-tracked
    digitalWrite(_pumpPins[roomIdx], on ? LOW : HIGH);// LOW = ON
    _pumpState[roomIdx] = on;
}

bool ActuatorController::isPumpActive(uint8_t roomIdx) const {
    return (roomIdx < ROOM_COUNT) && _pumpState[roomIdx];
}

bool ActuatorController::anyPumpActive() const {
    for (int i = 0; i < ROOM_COUNT; i++) if (_pumpState[i]) return true;
    return false;
}

// ==========================================
// LEDs & Buzzer, write only on change
// ==========================================
void ActuatorController::setLeds(bool fire) {
    int green = fire ? LOW : HIGH;
    int red   = fire ? HIGH : LOW;
    if (_ledGreenState != green) { digitalWrite(LED_GREEN_PIN, green); _ledGreenState = green; }
    if (_ledRedState   != red)   { digitalWrite(LED_RED_PIN,   red);   _ledRedState   = red;   }
}

void ActuatorController::setBuzzer(bool on) {
    int level = on ? HIGH : LOW;
    if (_buzzerState == level) return;
    digitalWrite(BUZZER_PIN, level);
    _buzzerState = level;
}

// ==========================================
// Safe defaults (full SAFE state)
// ==========================================
void ActuatorController::setSafeDefaults() {
    setGasValve(false);                 // valve OPEN
    for (int r = 0; r < ROOM_COUNT; r++) {
        setRoomDoor(r, false);          // doors CLOSED
        setPump(r, false);              // pumps OFF
    }
    setCorridors(false);                // corridors CLOSED
    setRoomWindow(2, false);            // Room 3 window OPEN
    setRoomWindow(3, false);            // Room 4 window OPEN
    setLeds(false);                     // green ON
    setBuzzer(false);                   // silent
}
