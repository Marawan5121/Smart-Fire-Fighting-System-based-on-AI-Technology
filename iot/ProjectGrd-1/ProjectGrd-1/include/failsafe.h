#ifndef FAILSAFE_H
#define FAILSAFE_H

#include <Arduino.h>

/**
 * @brief Connectivity-state monitor for the ESP32.
 *
 * Local sensors (gas/flame/buttons) drive zonal suppression directly and do NOT
 * depend on connectivity, so this FSM no longer actuates hardware. It only tracks
 * the WiFi/MQTT link to expose a "mode" string for telemetry and dashboards.
 *
 *   NORMAL ─(WiFi/MQTT lost)──────────► DEGRADED
 *   DEGRADED ─(connectivity restored)─► RECOVERY
 *   RECOVERY ─(hold stable)───────────► NORMAL
 */
class FailsafeManager {
public:
    enum FailsafeState {
        STATE_NORMAL,     // WiFi + MQTT connected
        STATE_DEGRADED,   // WiFi or MQTT lost
        STATE_RECOVERY    // connectivity restored, holding before NORMAL
    };

private:
    FailsafeState _currentState;
    unsigned long _stateEntryTime;
    unsigned long _recoveryHoldMs;

    void _changeState(FailsafeState newState);

public:
    explicit FailsafeManager(unsigned long recoveryHoldMs);

    void begin();

    /** @brief Update the connectivity FSM. Returns true on a state transition. */
    bool update(bool wifiConnected, bool mqttConnected);

    FailsafeState getState() const { return _currentState; }
    const char* getStateString() const;
};

#endif // FAILSAFE_H
