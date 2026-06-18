#include "failsafe.h"

FailsafeManager::FailsafeManager(unsigned long recoveryHoldMs)
    : _currentState(STATE_NORMAL),
      _stateEntryTime(0),
      _recoveryHoldMs(recoveryHoldMs) {}

void FailsafeManager::begin() {
    _currentState   = STATE_NORMAL;
    _stateEntryTime = millis();
    Serial.println("[Failsafe] Connectivity monitor initialized. State: NORMAL");
}

bool FailsafeManager::update(bool wifiConnected, bool mqttConnected) {
    bool connected = wifiConnected && mqttConnected;
    bool transitioned = false;

    switch (_currentState) {
        case STATE_NORMAL:
            if (!connected) {
                _changeState(STATE_DEGRADED);
                transitioned = true;
            }
            break;

        case STATE_DEGRADED:
            if (connected) {
                _changeState(STATE_RECOVERY);
                transitioned = true;
            }
            break;

        case STATE_RECOVERY:
            if (!connected) {
                _changeState(STATE_DEGRADED);
                transitioned = true;
            } else if (millis() - _stateEntryTime >= _recoveryHoldMs) {
                _changeState(STATE_NORMAL);
                transitioned = true;
            }
            break;
    }

    return transitioned;
}

void FailsafeManager::_changeState(FailsafeState newState) {
    Serial.print("[Failsafe] State: ");
    Serial.print(getStateString());
    _currentState   = newState;
    _stateEntryTime = millis();
    Serial.print(" -> ");
    Serial.println(getStateString());
}

const char* FailsafeManager::getStateString() const {
    switch (_currentState) {
        case STATE_NORMAL:   return "NORMAL";
        case STATE_DEGRADED: return "DEGRADED";
        case STATE_RECOVERY: return "RECOVERY";
        default:             return "UNKNOWN";
    }
}
