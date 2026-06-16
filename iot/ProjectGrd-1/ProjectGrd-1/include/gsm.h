#ifndef GSM_H
#define GSM_H

#include <Arduino.h>

/**
 * @brief Asynchronous, non-blocking GSM controller for SIM800L.
 *
 * Drives AT commands through a millis()-timed FSM (never blocks the loop).
 * B6: exposes a verified-delivery result so the caller can retry on failure
 * instead of blindly assuming success on queue.
 *
 * Storage uses fixed char buffers (no Arduino String) to avoid long-uptime
 * heap fragmentation.
 */
class GsmController {
public:
    enum GsmState {
        GSM_STATE_IDLE,
        GSM_STATE_SEND_AT,
        GSM_STATE_WAIT_AT_RESPONSE,
        GSM_STATE_SEND_CMGF,
        GSM_STATE_WAIT_CMGF_RESPONSE,
        GSM_STATE_SEND_CMGS,
        GSM_STATE_WAIT_PROMPT,
        GSM_STATE_SEND_BODY,
        GSM_STATE_WAIT_SEND_RESPONSE,
        GSM_STATE_ERROR_COOLDOWN
    };

    // Result of the most recent SMS attempt (polled by the caller).
    enum GsmResult {
        GSM_RESULT_NONE,      // nothing attempted yet
        GSM_RESULT_PENDING,   // an attempt is in progress
        GSM_RESULT_SUCCESS,   // SIM800L acknowledged ("OK" after +CMGS:)
        GSM_RESULT_FAILED     // timed out / error — safe to retry
    };

private:
    static const size_t PHONE_BUF = 20;
    static const size_t MSG_BUF   = 161;   // 160 GSM chars + NUL
    static const size_t RX_BUF    = 128;

    HardwareSerial& _serial;
    GsmState        _state;
    GsmResult       _result;
    unsigned long   _lastTime;
    unsigned long   _timeout;

    char _phoneNumber[PHONE_BUF];
    char _pendingMessage[MSG_BUF];
    char _rxBuffer[RX_BUF];
    int  _rxLen;

    void changeState(GsmState newState, unsigned long timeoutMs = 0);
    bool checkResponse(const char* expected);
    void flushInput();

public:
    explicit GsmController(HardwareSerial& serialRef);

    /** @brief Initialize the SIM800L hardware serial port. */
    void begin();

    /**
     * @brief Start a non-blocking SMS transmission.
     * @return false if the module is busy with another send.
     */
    bool sendSMSAsync(const char* phone, const char* text);

    /** @brief FSM engine — call every loop() iteration. */
    void update();

    /** @brief True while a send sequence (or its cooldown) is in progress. */
    bool isBusy() const { return _state != GSM_STATE_IDLE; }

    /** @brief Result of the most recent SMS attempt (for retry logic). */
    GsmResult getResult() const { return _result; }

    /** @brief Current FSM state (debugging). */
    GsmState getState() const { return _state; }
};

#endif // GSM_H
