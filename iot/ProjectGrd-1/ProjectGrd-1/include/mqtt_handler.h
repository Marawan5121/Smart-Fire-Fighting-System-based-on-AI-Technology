#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Latest actuator command from the AI module.
// Fixed char buffers (no Arduino String) to avoid heap fragmentation on the
// command hot path during long uptimes.
struct ActuatorCommand {
    char  gasValve[8];   // "OPEN" / "CLOSE"
    char  doors[8];      // "OPEN" / "CLOSE"
    char  pump1[8];      // "ON" / "OFF"
    char  pump2[8];      // "ON" / "OFF"
    char  state[16];     // "SAFE" / "SENSOR_ALERT" / "FIRE"  (drives LED + buzzer)
    float confidence;
    bool  isValid;       // true once a valid command has been parsed
};

/**
 * @brief MQTT client handler for the SFFS ESP32 node.
 * Publishes sensor telemetry as JSON, subscribes to AI commands,
 * and manages the broker connection lifecycle.
 */
class MqttHandler {
private:
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    const char* _broker;
    int _port;
    const char* _clientId;

    // Non-blocking reconnect backoff (B5)
    unsigned long _lastReconnectAttempt;
    unsigned long _reconnectBackoffMs;

    // Latest received command from the AI module
    ActuatorCommand _latestCommand;
    bool _newCommandAvailable;

    // Internal MQTT message callback dispatcher
    static MqttHandler* _instance;
    static void _staticCallback(char* topic, byte* payload, unsigned int length);
    void _handleMessage(char* topic, byte* payload, unsigned int length);

    // Single bounded (non-looping) connect attempt. Returns true on success.
    bool _attemptConnect();

public:
    MqttHandler(const char* broker, int port, const char* clientId);

    /** @brief Initialize MQTT client with LWT and connect */
    void begin();

    /** @brief Non-blocking connection maintenance. Call in loop(). */
    void update();

    /** @brief Check if MQTT is connected to the broker */
    bool isConnected();

    /**
     * @brief Publish sensor telemetry as structured JSON.
     * @param gas1..gas4   Filtered MQ sensor values
     * @param temp         Ambient temperature (°C)
     * @param hum          Humidity (%)
     * @param flame        Flame detection flag
     * @param manualTrigger Manual alarm button currently latched (→ AI forces FIRE)
     * @param rssi         WiFi RSSI (dBm)
     * @param mode         Current operating mode string
     */
    void publishSensorData(float gas1, float gas2, float gas3, float gas4,
                           float temp, float hum, bool flame,
                           float waterLevelPct, float waterDistanceCm,
                           bool manualTrigger,
                           int rssi, const char* mode);

    /** @brief Publish a manual alarm event from a push button */
    void publishManualAlarm(const char* room, bool active);

    /** @brief Publish a heartbeat signal to the broker */
    void publishHeartbeat();

    /** @brief Check if a new actuator command has arrived from AI */
    bool hasNewCommand();

    /** @brief Get the latest actuator command (resets the new flag) */
    ActuatorCommand getLatestCommand();
};

#endif // MQTT_HANDLER_H
