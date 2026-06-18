#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

/**
 * @brief MQTT client for the SFFS ESP32 node.
 * Publishes sensor telemetry, subscribes to the AI command topic, and exposes the
 * latest AI "state". Control is level-driven on the ESP32, so only `state` (the
 * global FIRE override) and `confidence` are extracted from the command payload.
 */
class MqttHandler {
private:
    WiFiClient   _wifiClient;
    PubSubClient _mqttClient;
    const char*  _broker;
    int          _port;
    const char*  _clientId;

    // Non-blocking reconnect backoff
    unsigned long _lastReconnectAttempt;
    unsigned long _reconnectBackoffMs;

    // Latest AI command state
    bool  _fireCommanded;     // true when the last parsed state == "FIRE"
    float _confidence;

    static MqttHandler* _instance;
    static void _staticCallback(char* topic, byte* payload, unsigned int length);
    void _handleMessage(char* topic, byte* payload, unsigned int length);
    bool _attemptConnect();

public:
    MqttHandler(const char* broker, int port, const char* clientId);

    void begin();
    void update();
    bool isConnected();

    /** @brief Publish sensor telemetry as JSON (schema consumed by the Python AI). */
    void publishSensorData(float gas1, float gas2, float gas3, float gas4,
                           float temp, float hum, bool flame,
                           float waterLevelPct, float waterDistanceCm,
                           bool manualTrigger,
                           int rssi, const char* mode);

    void publishManualAlarm(const char* room, bool active);
    void publishHeartbeat();

    /** @brief True only while connected AND the latest AI state is "FIRE". */
    bool isFireCommanded() const { return _fireCommanded; }
    float getConfidence() const { return _confidence; }
};

#endif // MQTT_HANDLER_H
