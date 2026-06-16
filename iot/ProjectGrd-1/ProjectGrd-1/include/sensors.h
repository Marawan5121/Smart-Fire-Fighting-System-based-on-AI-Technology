#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <DHT.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include "filters.h"

/**
 * @brief Manages all physical sensor inputs: 4x MQ gas sensors (with EMA filtering),
 * IR flame sensor, DHT22 (temp/humidity), and MPU6050 (accelerometer/tilt).
 */
class SensorsManager {
private:
    // EMA Filter Instances for each MQ Gas Sensor
    EmaFilter _mq2Filter;
    EmaFilter _mq5Filter;
    EmaFilter _mq6Filter;
    EmaFilter _mq7Filter;

    // Filtered gas values
    float _filteredGas1;
    float _filteredGas2;
    float _filteredGas3;
    float _filteredGas4;

    // DHT22 Environmental readings
    DHT _dht;
    float _ambientTemp;
    float _ambientHum;
    unsigned long _lastDhtRead;

    // MPU6050 Motion/Tilt
    Adafruit_MPU6050 _mpu;
    float _accelX, _accelY, _accelZ;
    float _tiltAngle;
    bool _tiltDetected;
    bool _mpuAvailable;

    // Flame sensor
    bool _flameDetected;

    // Warm-up management
    bool _isWarmedUp;
    unsigned long _bootTime;

    // Ultrasonic Water Level (HC-SR04)
    EmaFilter _waterFilter;
    float _waterDistanceCm;
    float _waterLevelPct;
    unsigned long _lastUltrasonicRead;

    // Manual Alarm Push Buttons
    bool _button1Pressed;
    bool _button2Pressed;
    unsigned long _btn1LastDebounce;
    unsigned long _btn2LastDebounce;
    bool _btn1LastState;
    bool _btn2LastState;

public:
    SensorsManager();

    /** @brief Configure all sensor pins, I2C, and peripherals */
    void begin();

    /** @brief Read and filter all sensors. Call every loop iteration. */
    void update();

    // ---- Filtered Gas Getters ----
    float getGas1() const { return _filteredGas1; }
    float getGas2() const { return _filteredGas2; }
    float getGas3() const { return _filteredGas3; }
    float getGas4() const { return _filteredGas4; }
    bool isGasDanger() const;

    // ---- Environmental Getters ----
    float getTemperature() const { return _ambientTemp; }
    float getHumidity() const { return _ambientHum; }
    float getInternalTemp();

    // ---- Flame Getter ----
    bool isFlameDetected() const { return _flameDetected; }

    // ---- Motion / Tilt Getters ----
    float getAccelX() const { return _accelX; }
    float getAccelY() const { return _accelY; }
    float getAccelZ() const { return _accelZ; }
    float getTiltAngle() const { return _tiltAngle; }
    bool isTiltDetected() const { return _tiltDetected; }

    // ---- System Status ----
    bool isWarmedUp() const { return _isWarmedUp; }
    unsigned long getWarmupTimeRemaining() const;

    // ---- Water Level Getters ----
    float getWaterLevelPct() const { return _waterLevelPct; }
    float getWaterDistanceCm() const { return _waterDistanceCm; }

    // ---- Manual Alarm Button Getters ----
    bool isButton1Pressed() const { return _button1Pressed; }
    bool isButton2Pressed() const { return _button2Pressed; }
};

#endif // SENSORS_H
