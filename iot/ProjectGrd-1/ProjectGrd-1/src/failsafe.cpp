#include "failsafe.h"

FailsafeManager::FailsafeManager(unsigned long criticalTimeoutMs, unsigned long recoveryHoldMs)
    : _currentState(STATE_NORMAL),
      _stateEntryTime(0),
      _dangerActive(false),
      _dangerStartTime(0),
      _criticalTimeoutMs(criticalTimeoutMs),
      _recoveryHoldMs(recoveryHoldMs),
      _eventCount(0) {}

void FailsafeManager::begin() {
    _currentState = STATE_NORMAL;
    _stateEntryTime = millis();
    _eventCount = 0;
    Serial.println("[Failsafe] Manager initialized. State: NORMAL");
}

bool FailsafeManager::update(bool wifiConnected, bool mqttConnected, bool sensorDanger) {
    bool connected = wifiConnected && mqttConnected;
    bool transitioned = false;

    switch (_currentState) {

        // ==========================================
        // STATE: NORMAL
        // WiFi + MQTT connected, AI is validating detections
        // ==========================================
        case STATE_NORMAL:
            if (!connected) {
                _changeState(STATE_DEGRADED);
                _logEvent("CONNECTIVITY_LOST");
                transitioned = true;
            }
            // Reset danger tracking while in normal mode
            _dangerActive = false;
            break;

        // ==========================================
        // STATE: DEGRADED
        // Lost WiFi or MQTT — monitoring sensors locally
        // ==========================================
        case STATE_DEGRADED:
            // Check if connectivity is restored
            if (connected) {
                _changeState(STATE_RECOVERY);
                _logEvent("CONNECTIVITY_RESTORED");
                transitioned = true;
                break;
            }

            // Track sustained danger duration
            if (sensorDanger) {
                if (!_dangerActive) {
                    // Danger just started
                    _dangerActive = true;
                    _dangerStartTime = millis();
                    Serial.println("[Failsafe] DEGRADED: Danger detected! Timer started...");
                } else {
                    // Danger is sustained — check if threshold exceeded
                    unsigned long dangerDuration = millis() - _dangerStartTime;
                    if (dangerDuration >= _criticalTimeoutMs) {
                        Serial.println("[Failsafe] CRITICAL: Danger sustained for " +
                                       String(dangerDuration / 1000) +
                                       "s without AI! Entering STANDALONE_ALERT.");
                        _changeState(STATE_STANDALONE_ALERT);
                        _logEvent("STANDALONE_ALERT_TRIGGERED:sustained_danger_" +
                                  String(dangerDuration / 1000) + "s");
                        transitioned = true;
                    }
                }
            } else {
                // Danger cleared — reset the timer
                if (_dangerActive) {
                    _dangerActive = false;
                    Serial.println("[Failsafe] DEGRADED: Danger cleared. Timer reset.");
                }
            }
            break;

        // ==========================================
        // STATE: STANDALONE_ALERT
        // ESP32 is acting autonomously (gas valve shut, buzzer on, SMS sent)
        // ==========================================
        case STATE_STANDALONE_ALERT:
            // Check if connectivity is restored
            if (connected) {
                _changeState(STATE_RECOVERY);
                _logEvent("CONNECTIVITY_RESTORED_FROM_STANDALONE");
                transitioned = true;
            }
            // Otherwise, remain in standalone mode — actuators are being
            // driven by main.cpp based on shouldActAutonomously()
            break;

        // ==========================================
        // STATE: RECOVERY
        // Connectivity restored — hold for stability before going NORMAL
        // ==========================================
        case STATE_RECOVERY:
            // If we lose connectivity again during recovery, go back to DEGRADED
            if (!connected) {
                _changeState(STATE_DEGRADED);
                _logEvent("RECOVERY_INTERRUPTED");
                transitioned = true;
                break;
            }

            // Wait for the recovery hold period to ensure stable connection
            if (millis() - _stateEntryTime >= _recoveryHoldMs) {
                Serial.println("[Failsafe] Recovery hold complete. Returning to NORMAL.");
                _changeState(STATE_NORMAL);
                _logEvent("NORMAL_RESTORED");
                transitioned = true;
            }
            break;
    }

    return transitioned;
}

void FailsafeManager::_changeState(FailsafeState newState) {
    Serial.print("[Failsafe] State Transition: ");
    Serial.print(getStateString());
    Serial.print(" → ");

    _currentState = newState;
    _stateEntryTime = millis();
    _dangerActive = false;  // Reset danger tracking on every transition

    Serial.println(getStateString());
}

void FailsafeManager::_logEvent(const String& event) {
    if (_eventCount < MAX_EVENTS) {
        // Format: "timestamp_seconds:event_description"
        _eventBuffer[_eventCount] = String(millis() / 1000) + ":" + event;
        _eventCount++;
        Serial.println("[Failsafe] Event logged [" + String(_eventCount) +
                       "/" + String(MAX_EVENTS) + "]: " + event);
    } else {
        Serial.println("[Failsafe] WARNING: Event buffer full! Oldest events may be lost.");
    }
}

const char* FailsafeManager::getStateString() const {
    switch (_currentState) {
        case STATE_NORMAL:           return "NORMAL";
        case STATE_DEGRADED:         return "DEGRADED";
        case STATE_STANDALONE_ALERT: return "STANDALONE_ALERT";
        case STATE_RECOVERY:         return "RECOVERY";
        default:                     return "UNKNOWN";
    }
}

String FailsafeManager::getBufferedEvent(int index) const {
    if (index >= 0 && index < _eventCount) {
        return _eventBuffer[index];
    }
    return "";
}

void FailsafeManager::clearEventBuffer() {
    _eventCount = 0;
    Serial.println("[Failsafe] Event buffer cleared after MQTT sync.");
}
