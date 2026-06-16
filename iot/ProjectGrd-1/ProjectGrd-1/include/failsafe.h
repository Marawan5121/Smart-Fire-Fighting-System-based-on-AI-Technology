#ifndef FAILSAFE_H
#define FAILSAFE_H

#include <Arduino.h>

/**
 * @brief Autonomous Failsafe State Machine for the ESP32.
 *
 * State Transitions:
 *   NORMAL ─(WiFi/MQTT lost)────────────────────────► DEGRADED
 *   DEGRADED ─(critical sensors for >30s, no AI)────► STANDALONE_ALERT
 *   STANDALONE_ALERT ─(WiFi/MQTT restored)──────────► RECOVERY
 *   RECOVERY ─(hold 10s stable)─────────────────────► NORMAL
 *
 * In STANDALONE_ALERT mode, the ESP32 autonomously:
 *   - Shuts the gas valve
 *   - Activates the buzzer
 *   - Sends SMS via SIM800L
 *   - Logs the event for later MQTT sync
 */
class FailsafeManager {
public:
    enum FailsafeState {
        STATE_NORMAL,           // WiFi + MQTT connected, AI is validating
        STATE_DEGRADED,         // WiFi or MQTT lost, sensors still monitoring
        STATE_STANDALONE_ALERT, // Critical threshold breached without AI — act locally
        STATE_RECOVERY          // Connectivity restored, syncing missed events
    };

private:
    FailsafeState _currentState;
    unsigned long _stateEntryTime;         // When we entered the current state

    // Danger tracking (for DEGRADED → STANDALONE_ALERT transition)
    bool _dangerActive;                    // Are critical sensors currently in danger?
    unsigned long _dangerStartTime;        // When did the sustained danger begin?
    unsigned long _criticalTimeoutMs;      // How long before autonomous action
    unsigned long _recoveryHoldMs;         // How long to hold in RECOVERY before NORMAL

    // Event buffer for missed events during standalone operation
    static const int MAX_EVENTS = 10;
    String _eventBuffer[MAX_EVENTS];
    int _eventCount;

    // Transition helpers
    void _changeState(FailsafeState newState);
    void _logEvent(const String& event);

public:
    FailsafeManager(unsigned long criticalTimeoutMs, unsigned long recoveryHoldMs);

    /** @brief Initialize the failsafe manager */
    void begin();

    /**
     * @brief Main FSM update method. Call every loop iteration.
     * @param wifiConnected    Is WiFi currently connected?
     * @param mqttConnected    Is MQTT broker currently connected?
     * @param sensorDanger     Are critical sensors above threshold?
     * @return true if a state transition occurred this cycle
     */
    bool update(bool wifiConnected, bool mqttConnected, bool sensorDanger);

    /** @brief Get the current failsafe state */
    FailsafeState getState() const { return _currentState; }

    /** @brief Get the current state as a human-readable string */
    const char* getStateString() const;

    /** @brief Check if the system should act autonomously (standalone alert) */
    bool shouldActAutonomously() const {
        return _currentState == STATE_STANDALONE_ALERT;
    }

    /** @brief Check if there are buffered events to sync after recovery */
    bool hasBufferedEvents() const { return _eventCount > 0; }

    /** @brief Get the number of buffered events */
    int getBufferedEventCount() const { return _eventCount; }

    /** @brief Get a buffered event by index */
    String getBufferedEvent(int index) const;

    /** @brief Clear the event buffer (after successful MQTT sync) */
    void clearEventBuffer();
};

#endif // FAILSAFE_H
