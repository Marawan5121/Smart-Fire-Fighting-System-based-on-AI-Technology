#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>
#include <Adafruit_PWMServoDriver.h>

/** @brief Buzzer drive patterns, selected by system state (non-blocking). */
enum BuzzerMode {
    BUZZER_MODE_OFF,          // silent (SAFE)
    BUZZER_MODE_INTERMITTENT, // periodic beep (SENSOR_ALERT)
    BUZZER_MODE_CONTINUOUS    // solid tone (FIRE / MANUAL)
};

/**
 * @brief Controls all physical actuators: 2x SG90 servos (gas valve, doors),
 * active buzzer, 2x relay-controlled water pumps, and 3x status indicator LEDs.
 * Provides a clean command interface matching the MQTT command schema.
 */
class ActuatorController {
private:
    Adafruit_PWMServoDriver _pwm;   // 5 servos on channels 0-4 (I2C @ 0x40)

    int _buzzerPin;
    int _pump1Pin;
    int _pump2Pin;
    int _pump3Pin;
    int _ledGreenPin;
    int _ledOrangePin;
    int _ledRedPin;

    // Current actuator states (for telemetry reporting)
    bool _gasValveOpen;
    bool _doorOpen;
    bool _buzzerActive;
    bool _pump1Active;
    bool _pump2Active;
    bool _pump3Active;

    // Non-blocking buzzer pattern state
    BuzzerMode    _buzzerMode;
    unsigned long _buzzerLastToggle;
    bool          _buzzerPinState;

    /** @brief Drive one PCA9685 servo channel to an angle (0-180°). */
    void setServoAngle(uint8_t channel, int angle);

public:
    ActuatorController();

    /** @brief Init PCA9685 (servos ch 0-4) + configure buzzer/pump/LED pins.
     *  Servos are on I2C now, so no servo GPIOs are passed. Pump 3 is driven
     *  ALWAYS-ON here and is intentionally not touched by setEmergency/setAllSafe. */
    void begin(int buzzerPin,
               int pump1Pin, int pump2Pin, int pump3Pin,
               int ledGreenPin, int ledOrangePin, int ledRedPin);

    // ---- Gas Valve Servo ----
    void openGasValve();
    void closeGasValve();
    void setGasValve(const String& command);

    // ---- Door/Window Servo ----
    void openDoors();
    void closeDoors();
    void setDoors(const String& command);

    // ---- Buzzer ----
    void buzzerOn();                       // → continuous mode
    void buzzerOff();                      // → off mode
    void setBuzzerMode(BuzzerMode mode);   // select OFF / INTERMITTENT / CONTINUOUS
    void updateBuzzer();                   // tick the pattern; call every loop()

    // ---- Water Pumps (Active LOW relay) ----
    void pump1On();
    void pump1Off();
    void setPump1(const String& command);
    void pump2On();
    void pump2Off();
    void setPump2(const String& command);
    void pump3On();
    void pump3Off();

    // ---- Status LEDs ----
    /** @brief Set LEDs by system state: "SAFE"(🟢) "SENSOR_ALERT"(🟠) "FIRE"/"MANUAL"(🔴) */
    void setStatusLeds(const String& state);

    // ---- Batch Command (mechanical actuators only) ----
    /** @brief Apply valve/door/pump commands. LED + buzzer come from applyState(). */
    void applyCommands(const String& gasValve, const String& doors,
                       const String& pump1, const String& pump2);

    /** @brief Drive LED color + buzzer pattern from the fused system state. */
    void applyState(const String& state);

    // ---- Safe Defaults ----
    void setAllSafe();
    void setEmergency();

    // ---- Status Getters ----
    bool isGasValveOpen() const { return _gasValveOpen; }
    bool isDoorOpen() const { return _doorOpen; }
    bool isBuzzerActive() const { return _buzzerActive; }
    bool isPump1Active() const { return _pump1Active; }
    bool isPump2Active() const { return _pump2Active; }
    bool isPump3Active() const { return _pump3Active; }
};

#endif // ACTUATORS_H
