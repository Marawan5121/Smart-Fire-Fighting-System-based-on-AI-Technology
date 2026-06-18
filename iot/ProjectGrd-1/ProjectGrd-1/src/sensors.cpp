#include "sensors.h"
#include "config.h"

SensorsManager::SensorsManager()
    : _mq2Filter(EMA_ALPHA),
      _mq5Filter(EMA_ALPHA),
      _mq6Filter(EMA_ALPHA),
      _mq7Filter(EMA_ALPHA),
      _dht(DHT_PIN, DHT22),
      _ambientTemp(0), _ambientHum(0), _lastDhtRead(0),
      _flameDetected(false), _isWarmedUp(false), _bootTime(0),
      _waterFilter(EMA_ALPHA), _waterDistanceCm(0), _waterLevelPct(0), _lastUltrasonicRead(0) {
    for (int i = 0; i < 4; i++) {
        _filteredGas[i]    = 0;
        _buttonPressed[i]  = false;
        _btnLastDebounce[i] = 0;
        _btnLastState[i]   = false;
    }
}

void SensorsManager::begin() {
    _bootTime = millis();

    // MQ gas sensors (analog inputs)
    pinMode(GAS1_PIN, INPUT);
    pinMode(GAS2_PIN, INPUT);
    pinMode(GAS3_PIN, INPUT);
    pinMode(GAS4_PIN, INPUT);

    // IR flame sensor: INPUT_PULLUP, active-LOW (digitalRead == LOW means fire).
    pinMode(FLAME_PIN, INPUT_PULLUP);

    _dht.begin();
    Serial.println("[Sensors] DHT22 initialized on GPIO" + String(DHT_PIN));

    // Ultrasonic water level
    pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
    pinMode(ULTRASONIC_ECHO_PIN, INPUT);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    Serial.println("[Sensors] HC-SR04 initialized (TRIG:GPIO" + String(ULTRASONIC_TRIG_PIN) +
                   " ECHO:GPIO" + String(ULTRASONIC_ECHO_PIN) + ")");

    // Manual alarm buttons (INPUT_PULLDOWN, active-HIGH)
    pinMode(BUTTON1_PIN, INPUT_PULLDOWN);
    pinMode(BUTTON2_PIN, INPUT_PULLDOWN);
    pinMode(BUTTON3_PIN, INPUT_PULLDOWN);
    pinMode(BUTTON4_PIN, INPUT_PULLDOWN);
    Serial.println("[Sensors] 4 manual alarm buttons initialized (GPIO" +
                   String(BUTTON1_PIN) + "/" + String(BUTTON2_PIN) + "/" +
                   String(BUTTON3_PIN) + "/" + String(BUTTON4_PIN) + ")");

    Serial.println("[Sensors] Configured. Warm-up started (" +
                   String(SENSOR_WARMUP_MS / 1000) + "s)...");
}

void SensorsManager::update() {
    // 1. Read + filter the four MQ gas sensors (one per room)
    _filteredGas[0] = _mq2Filter.filter((float)analogRead(GAS1_PIN));
    _filteredGas[1] = _mq5Filter.filter((float)analogRead(GAS2_PIN));
    _filteredGas[2] = _mq6Filter.filter((float)analogRead(GAS3_PIN));
    _filteredGas[3] = _mq7Filter.filter((float)analogRead(GAS4_PIN));

    // 2. Flame sensor (active-LOW)
    _flameDetected = (digitalRead(FLAME_PIN) == HIGH);

    // 3. DHT22 (rate-limited to 2s; can return NaN)
    if (millis() - _lastDhtRead >= 2000) {
        float t = _dht.readTemperature();
        float h = _dht.readHumidity();
        if (!isnan(t)) _ambientTemp = t;
        if (!isnan(h)) _ambientHum  = h;
        _lastDhtRead = millis();
    }

    // 4. Warm-up gate
    if (!_isWarmedUp && (millis() - _bootTime >= SENSOR_WARMUP_MS)) {
        _isWarmedUp = true;
        Serial.println("[Sensors] Warm-up complete. Gas readings stabilized.");
    }

    // 5. HC-SR04 water level
    if (millis() - _lastUltrasonicRead >= ULTRASONIC_READ_INTERVAL_MS) {
        _lastUltrasonicRead = millis();

        digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

        long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_US);
        if (duration > 0) {
            float rawDistance = (duration * 0.0343f) / 2.0f;   // 343 m/s
            _waterDistanceCm = _waterFilter.filter(rawDistance);
            float level = ((TANK_HEIGHT_CM - _waterDistanceCm) / TANK_HEIGHT_CM) * 100.0f;
            _waterLevelPct = constrain(level, 0.0f, 100.0f);
        }
        // timeout (duration == 0): keep last valid reading
    }

    // 6. Manual alarm buttons (debounced)
    const int btnPins[4] = { BUTTON1_PIN, BUTTON2_PIN, BUTTON3_PIN, BUTTON4_PIN };
    for (int i = 0; i < 4; i++) {
        bool raw = (digitalRead(btnPins[i]) == HIGH);
        if (raw != _btnLastState[i]) {
            _btnLastDebounce[i] = millis();
            _btnLastState[i] = raw;
        }
        if (millis() - _btnLastDebounce[i] >= BUTTON_DEBOUNCE_MS) {
            _buttonPressed[i] = _btnLastState[i];
        }
    }
}

unsigned long SensorsManager::getWarmupTimeRemaining() const {
    if (_isWarmedUp) return 0;
    unsigned long elapsed = millis() - _bootTime;
    if (elapsed >= SENSOR_WARMUP_MS) return 0;
    return (SENSOR_WARMUP_MS - elapsed) / 1000;
}
