#include "sensors.h"
#include "config.h"
#include <Wire.h>

SensorsManager::SensorsManager()
    : _mq2Filter(EMA_ALPHA),
      _mq5Filter(EMA_ALPHA),
      _mq6Filter(EMA_ALPHA),
      _mq7Filter(EMA_ALPHA),
      _dht(DHT_PIN, DHT22),
      _filteredGas1(0), _filteredGas2(0), _filteredGas3(0), _filteredGas4(0),
      _ambientTemp(0), _ambientHum(0), _lastDhtRead(0),
      _accelX(0), _accelY(0), _accelZ(0),
      _tiltAngle(0), _tiltDetected(false), _mpuAvailable(false),
      _flameDetected(false), _isWarmedUp(false), _bootTime(0),
      _waterFilter(EMA_ALPHA), _waterDistanceCm(0), _waterLevelPct(0), _lastUltrasonicRead(0),
      _button1Pressed(false), _button2Pressed(false),
      _btn1LastDebounce(0), _btn2LastDebounce(0),
      _btn1LastState(false), _btn2LastState(false) {}

void SensorsManager::begin() {
    _bootTime = millis();

    // Configure analog input pins (MQ sensors)
    pinMode(GAS1_PIN, INPUT);
    pinMode(GAS2_PIN, INPUT);
    pinMode(GAS3_PIN, INPUT);
    pinMode(GAS4_PIN, INPUT);

    // Configure digital input pin (flame sensor)
    pinMode(FLAME_PIN, INPUT);

    // Initialize DHT22
    _dht.begin();
    Serial.println("[Sensors] DHT22 initialized on GPIO" + String(DHT_PIN));

    // Initialize I2C and MPU6050
    Wire.begin(I2C_SDA, I2C_SCL);
    if (_mpu.begin(0x68, &Wire)) {
        _mpuAvailable = true;
        _mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
        _mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        Serial.println("[Sensors] MPU6050 initialized on I2C (SDA:" +
                       String(I2C_SDA) + " SCL:" + String(I2C_SCL) + ")");
    } else {
        _mpuAvailable = false;
        Serial.println("[Sensors] WARNING: MPU6050 not found! Tilt detection disabled.");
    }

    // Configure ultrasonic pins
    pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
    pinMode(ULTRASONIC_ECHO_PIN, INPUT);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    Serial.println("[Sensors] HC-SR04 ultrasonic initialized (TRIG:GPIO" + String(ULTRASONIC_TRIG_PIN) + " ECHO:GPIO" + String(ULTRASONIC_ECHO_PIN) + ")");

    // Configure button pins (GPIO 16/17, reclaimed from the removed SIM800L UART2).
    // Both use the internal pulldown → active-HIGH, no external resistor needed.
    pinMode(BUTTON1_PIN, INPUT_PULLDOWN);  // GPIO 16
    pinMode(BUTTON2_PIN, INPUT_PULLDOWN);  // GPIO 17
    Serial.println("[Sensors] Manual alarm buttons initialized (BTN1:GPIO" + String(BUTTON1_PIN) + " BTN2:GPIO" + String(BUTTON2_PIN) + ")");

    Serial.println("[Sensors] All sensors configured. Warm-up period started ("
                   + String(SENSOR_WARMUP_MS / 1000) + "s)...");
}

void SensorsManager::update() {
    // ==========================================
    // 1. Read and Filter MQ Gas Sensors
    // ==========================================
    float rawGas1 = (float)analogRead(GAS1_PIN);
    float rawGas2 = (float)analogRead(GAS2_PIN);
    float rawGas3 = (float)analogRead(GAS3_PIN);
    float rawGas4 = (float)analogRead(GAS4_PIN);

    _filteredGas1 = _mq2Filter.filter(rawGas1);
    _filteredGas2 = _mq5Filter.filter(rawGas2);
    _filteredGas3 = _mq6Filter.filter(rawGas3);
    _filteredGas4 = _mq7Filter.filter(rawGas4);

    // ==========================================
    // 2. Read Flame Sensor (Active LOW)
    // ==========================================
    _flameDetected = (digitalRead(FLAME_PIN) == LOW);

    // ==========================================
    // 3. Read DHT22 (rate-limited to every 2 seconds by hardware)
    // ==========================================
    if (millis() - _lastDhtRead >= 2000) {
        float tempReading = _dht.readTemperature();
        float humReading  = _dht.readHumidity();

        // Only update if readings are valid (DHT22 can return NaN)
        if (!isnan(tempReading)) {
            _ambientTemp = tempReading;
        }
        if (!isnan(humReading)) {
            _ambientHum = humReading;
        }
        _lastDhtRead = millis();
    }

    // ==========================================
    // 4. Read MPU6050 Accelerometer
    // ==========================================
    if (_mpuAvailable) {
        sensors_event_t accel, gyro, temp;
        _mpu.getEvent(&accel, &gyro, &temp);

        _accelX = accel.acceleration.x;
        _accelY = accel.acceleration.y;
        _accelZ = accel.acceleration.z;

        // Calculate tilt angle from gravity vector
        // In a level position: ax≈0, ay≈0, az≈9.8
        // tiltAngle = atan2(sqrt(ax² + ay²), az) × 180/π
        float horizontalAccel = sqrt(_accelX * _accelX + _accelY * _accelY);
        _tiltAngle = atan2(horizontalAccel, abs(_accelZ)) * 180.0f / PI;
        _tiltDetected = (_tiltAngle > TILT_ANGLE_THRESHOLD);
    }

    // ==========================================
    // 5. Warm-up Monitoring
    // ==========================================
    if (!_isWarmedUp) {
        if (millis() - _bootTime >= SENSOR_WARMUP_MS) {
            _isWarmedUp = true;
            Serial.println("[Sensors] Warm-up complete! All sensor readings stabilized.");
        }
    }

    // ==========================================
    // 6. Read HC-SR04 Ultrasonic Water Level
    // ==========================================
    if (millis() - _lastUltrasonicRead >= ULTRASONIC_READ_INTERVAL_MS) {
        _lastUltrasonicRead = millis();

        // Send 10µs trigger pulse
        digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

        // Measure echo pulse duration. B5: bounded timeout so an unplugged or
        // silent HC-SR04 cannot stall the loop — pulseIn returns 0 after the cap.
        long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_US);

        if (duration > 0) {
            // Speed of sound: 343 m/s = 0.0343 cm/µs
            // Distance = (duration × 0.0343) / 2
            float rawDistance = (duration * 0.0343f) / 2.0f;
            float filteredDistance = _waterFilter.filter(rawDistance);
            _waterDistanceCm = filteredDistance;

            // Calculate water level percentage
            float level = ((TANK_HEIGHT_CM - filteredDistance) / TANK_HEIGHT_CM) * 100.0f;
            _waterLevelPct = constrain(level, 0.0f, 100.0f);
        }
        // If duration == 0 (timeout), keep last valid reading
    }

    // ==========================================
    // 7. Read Manual Alarm Buttons (Debounced)
    // ==========================================
    bool btn1Raw = (digitalRead(BUTTON1_PIN) == HIGH);
    bool btn2Raw = (digitalRead(BUTTON2_PIN) == HIGH);

    if (btn1Raw != _btn1LastState) {
        _btn1LastDebounce = millis();
        _btn1LastState = btn1Raw;
    }
    if (millis() - _btn1LastDebounce >= BUTTON_DEBOUNCE_MS) {
        _button1Pressed = _btn1LastState;
    }

    if (btn2Raw != _btn2LastState) {
        _btn2LastDebounce = millis();
        _btn2LastState = btn2Raw;
    }
    if (millis() - _btn2LastDebounce >= BUTTON_DEBOUNCE_MS) {
        _button2Pressed = _btn2LastState;
    }
}

bool SensorsManager::isGasDanger() const {
    if (!_isWarmedUp) return false;
    return (_filteredGas1 > GAS_THRESHOLD ||
            _filteredGas2 > GAS_THRESHOLD ||
            _filteredGas3 > GAS_THRESHOLD ||
            _filteredGas4 > GAS_THRESHOLD);
}

float SensorsManager::getInternalTemp() {
    return temperatureRead();  // ESP32 internal chip temperature
}

unsigned long SensorsManager::getWarmupTimeRemaining() const {
    if (_isWarmedUp) return 0;
    unsigned long elapsed = millis() - _bootTime;
    if (elapsed >= SENSOR_WARMUP_MS) return 0;
    return (SENSOR_WARMUP_MS - elapsed) / 1000;
}
