#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <DHT.h>
#include "filters.h"

/**
 * @brief Manages all physical sensor inputs: 4x MQ gas sensors (one per room,
 * EMA-filtered), an IR flame sensor (active-LOW), a DHT22 (temp/humidity),
 * an HC-SR04 water-level sensor, and 4 manual alarm push buttons.
 */
class SensorsManager {
private:
    // Per-room MQ gas sensors + EMA filters
    EmaFilter _mq2Filter;
    EmaFilter _mq5Filter;
    EmaFilter _mq6Filter;
    EmaFilter _mq7Filter;
    float _filteredGas[4];      // [0]=Room1 MQ2 .. [3]=Room4 MQ7

    // DHT22 environmental readings
    DHT _dht;
    float _ambientTemp;
    float _ambientHum;
    unsigned long _lastDhtRead;

    // Flame sensor
    bool _flameDetected;

    // Warm-up management
    bool _isWarmedUp;
    unsigned long _bootTime;

    // Ultrasonic water level (HC-SR04)
    EmaFilter _waterFilter;
    float _waterDistanceCm;
    float _waterLevelPct;
    unsigned long _lastUltrasonicRead;

    // Manual alarm buttons (one per room)
    bool _buttonPressed[4];
    unsigned long _btnLastDebounce[4];
    bool _btnLastState[4];

public:
    SensorsManager();

    void begin();
    void update();

    // ---- Per-room gas getters (index 0-3) ----
    float getGas(uint8_t roomIdx) const { return (roomIdx < 4) ? _filteredGas[roomIdx] : 0.0f; }

    // ---- Environmental getters ----
    float getTemperature() const { return _ambientTemp; }
    float getHumidity() const { return _ambientHum; }

    // ---- Flame getter (true while IR sensor reads LOW) ----
    bool isFlameDetected() const { return _flameDetected; }

    // ---- System status ----
    bool isWarmedUp() const { return _isWarmedUp; }
    unsigned long getWarmupTimeRemaining() const;

    // ---- Water level getters ----
    float getWaterLevelPct() const { return _waterLevelPct; }
    float getWaterDistanceCm() const { return _waterDistanceCm; }

    // ---- Manual alarm button getters (index 0-3) ----
    bool isButtonPressed(uint8_t roomIdx) const { return (roomIdx < 4) && _buttonPressed[roomIdx]; }
    bool anyButtonPressed() const {
        for (int i = 0; i < 4; i++) if (_buttonPressed[i]) return true;
        return false;
    }
};

#endif // SENSORS_H
