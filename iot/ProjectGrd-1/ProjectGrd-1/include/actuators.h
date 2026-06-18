#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>
#include "config.h"

/**
 * @brief Drives all physical actuators for the 4-room zonal suppression system:
 * 9 servos (gas valve, 4 room doors, 2 corridors, 2 windows) on a PCA9685, an
 * active buzzer, 4 relay-controlled water pumps, and 2 status LEDs.
 *
 * Every write is EDGE-TRACKED: setters cache the last value and only touch the
 * I2C bus / GPIO when the value actually changes. This makes it safe to call the
 * full state every loop and prevents PCA9685 chatter / relay brown-out.
 */
class ActuatorController {
private:
    Adafruit_PWMServoDriver _pwm;   // 9 servos on channels 0-8 (I2C @ 0x40)

    int _pumpPins[ROOM_COUNT];

    // Edge-tracking caches (-1 / impossible value forces the first write).
    int  _servoAngle[9];
    bool _pumpState[ROOM_COUNT];
    int  _ledGreenState;
    int  _ledRedState;
    int  _buzzerState;

    /** @brief Drive one PCA9685 channel to an angle (0-180 deg), write-on-change. */
    void setServoAngle(uint8_t channel, int angle);

public:
    ActuatorController();

    /** @brief Init PCA9685, buzzer, 4 pump relays, and 2 LEDs; leave hardware SAFE. */
    void begin();

    // ---- Servo groups (boolean, level-driven) ----
    void setGasValve(bool emergency);              // emergency -> CLOSED(180), else OPEN(0)
    void setRoomDoor(uint8_t roomIdx, bool open);  // roomIdx 0-3 -> ch 1-4
    void setCorridors(bool open);                  // ch 5 & 6 together
    void setRoomWindow(uint8_t roomIdx, bool fire);// roomIdx 2|3 -> ch 7|8; fire -> CLOSED(0)

    // ---- Pumps (Active-LOW relay) ----
    void setPump(uint8_t roomIdx, bool on);
    bool isPumpActive(uint8_t roomIdx) const;
    bool anyPumpActive() const;

    // ---- LEDs & Buzzer ----
    void setLeds(bool fire);    // fire -> RED on / GREEN off; else GREEN on / RED off
    void setBuzzer(bool on);    // continuous tone while true

    // ---- Safe defaults ----
    void setSafeDefaults();
};

#endif // ACTUATORS_H
