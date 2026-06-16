#include "gsm.h"
#include "config.h"
#include <string.h>

GsmController::GsmController(HardwareSerial& serialRef)
    : _serial(serialRef), _state(GSM_STATE_IDLE), _result(GSM_RESULT_NONE),
      _lastTime(0), _timeout(0), _rxLen(0) {
    _phoneNumber[0] = '\0';
    _pendingMessage[0] = '\0';
    _rxBuffer[0] = '\0';
}

void GsmController::begin() {
    _serial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
    Serial.print("[GSM] HardwareSerial up on RX:");
    Serial.print(GSM_RX_PIN);
    Serial.print(" TX:");
    Serial.println(GSM_TX_PIN);
}

bool GsmController::sendSMSAsync(const char* phone, const char* text) {
    if (_state != GSM_STATE_IDLE) {
        Serial.println("[GSM] Warn: module busy, send rejected.");
        return false;
    }

    strlcpy(_phoneNumber, phone, sizeof(_phoneNumber));
    strlcpy(_pendingMessage, text, sizeof(_pendingMessage));
    _result = GSM_RESULT_PENDING;

    changeState(GSM_STATE_SEND_AT, 2000);
    Serial.println("[GSM] Starting non-blocking SMS sequence...");
    return true;
}

void GsmController::changeState(GsmState newState, unsigned long timeoutMs) {
    _state = newState;
    _lastTime = millis();
    _timeout = timeoutMs;
    flushInput();
    _rxLen = 0;
    _rxBuffer[0] = '\0';
}

void GsmController::flushInput() {
    while (_serial.available()) {
        _serial.read();
    }
}

bool GsmController::checkResponse(const char* expected) {
    while (_serial.available()) {
        char c = _serial.read();
        Serial.write(c);  // mirror raw modem output to the debug console

        if (_rxLen >= (int)sizeof(_rxBuffer) - 1) {
            // Overflow guard: reset and keep scanning new data.
            _rxLen = 0;
            _rxBuffer[0] = '\0';
        }
        _rxBuffer[_rxLen++] = c;
        _rxBuffer[_rxLen] = '\0';

        if (strstr(_rxBuffer, expected) != nullptr) {
            _rxLen = 0;
            _rxBuffer[0] = '\0';
            return true;
        }
    }
    return false;
}

void GsmController::update() {
    if (_state == GSM_STATE_IDLE) {
        return;
    }

    // ---- Unified timeout handler (cooldown is reachable here, not a dead case) ----
    if (millis() - _lastTime >= _timeout) {
        if (_state == GSM_STATE_ERROR_COOLDOWN) {
            Serial.println("[GSM] Error recovery complete. Ready for next command.");
            changeState(GSM_STATE_IDLE);   // result stays FAILED → caller may retry
        } else {
            Serial.print("[GSM] ERROR: timeout in state ");
            Serial.println((int)_state);
            _result = GSM_RESULT_FAILED;
            changeState(GSM_STATE_ERROR_COOLDOWN, GSM_COOLDOWN_MS);
        }
        return;
    }

    // ---- Active send sequence ----
    switch (_state) {
        case GSM_STATE_SEND_AT:
            Serial.println("[GSM] AT ping...");
            _serial.println("AT");
            changeState(GSM_STATE_WAIT_AT_RESPONSE, 2000);
            break;

        case GSM_STATE_WAIT_AT_RESPONSE:
            if (checkResponse("OK")) changeState(GSM_STATE_SEND_CMGF, 2000);
            break;

        case GSM_STATE_SEND_CMGF:
            Serial.println("[GSM] Text mode (CMGF=1)...");
            _serial.println("AT+CMGF=1");
            changeState(GSM_STATE_WAIT_CMGF_RESPONSE, 2000);
            break;

        case GSM_STATE_WAIT_CMGF_RESPONSE:
            if (checkResponse("OK")) changeState(GSM_STATE_SEND_CMGS, 2000);
            break;

        case GSM_STATE_SEND_CMGS:
            Serial.println("[GSM] Addressing recipient (CMGS)...");
            _serial.print("AT+CMGS=\"");
            _serial.print(_phoneNumber);
            _serial.println("\"");
            changeState(GSM_STATE_WAIT_PROMPT, 3000);
            break;

        case GSM_STATE_WAIT_PROMPT:
            if (checkResponse(">")) changeState(GSM_STATE_SEND_BODY, 5000);
            break;

        case GSM_STATE_SEND_BODY:
            Serial.println("[GSM] Sending message body...");
            _serial.print(_pendingMessage);
            _serial.write(26);  // Ctrl+Z terminates the SMS
            changeState(GSM_STATE_WAIT_SEND_RESPONSE, 10000);
            break;

        case GSM_STATE_WAIT_SEND_RESPONSE:
            // Success = final "OK" that follows the "+CMGS: <ref>" line.
            if (checkResponse("OK")) {
                Serial.println("\n[GSM] SUCCESS: SMS delivery acknowledged.");
                _result = GSM_RESULT_SUCCESS;
                changeState(GSM_STATE_IDLE);
            }
            break;

        default:
            changeState(GSM_STATE_IDLE);
            break;
    }
}
