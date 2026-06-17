#include "mqtt_handler.h"
#include "config.h"
#include <string.h>

// Static instance pointer for the C-style callback bridge
MqttHandler* MqttHandler::_instance = nullptr;

MqttHandler::MqttHandler(const char* broker, int port, const char* clientId)
    : _mqttClient(_wifiClient), _broker(broker), _port(port), _clientId(clientId),
      _lastReconnectAttempt(0), _reconnectBackoffMs(MQTT_RECONNECT_MIN_MS),
      _newCommandAvailable(false) {
    _instance = this;
    _latestCommand.isValid = false;
}

void MqttHandler::begin() {
    // Configure the client ONCE (no duplicate setServer).
    _mqttClient.setServer(_broker, _port);
    _mqttClient.setCallback(_staticCallback);
    _mqttClient.setBufferSize(JSON_BUFFER_SIZE);
    _mqttClient.setKeepAlive(MQTT_KEEPALIVE);
    _mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);  // B5: cap blocking at 2s

    Serial.print("[MQTT] Target broker: ");
    Serial.print(_broker);
    Serial.print(":");
    Serial.println(_port);

    _attemptConnect();
}

bool MqttHandler::_attemptConnect() {
    const char* user = (strlen(MQTT_USERNAME) > 0) ? MQTT_USERNAME : nullptr;
    const char* pass = (strlen(MQTT_PASSWORD) > 0) ? MQTT_PASSWORD : nullptr;

    // LWT on the heartbeat topic: broker flags us offline if the ESP32 drops.
    bool ok = _mqttClient.connect(
        _clientId, user, pass,
        TOPIC_HEARTBEAT, 1, true, "{\"status\":\"offline\"}"
    );

    if (ok) {
        Serial.println("[MQTT] Connected to broker.");
        _mqttClient.subscribe(TOPIC_SYSTEM_STATUS, 1);  // QoS 1 for AI commands
        Serial.print("[MQTT] Subscribed to: ");
        Serial.println(TOPIC_SYSTEM_STATUS);
        _reconnectBackoffMs = MQTT_RECONNECT_MIN_MS;     // reset backoff on success
    } else {
        Serial.print("[MQTT] Connect failed, rc=");
        Serial.println(_mqttClient.state());
    }
    return ok;
}

void MqttHandler::update() {
    if (_mqttClient.connected()) {
        _mqttClient.loop();
        return;
    }

    // B5: fully non-blocking reconnect.
    // Never attempt while WiFi is down (that path is where the long stalls live),
    // and gate attempts behind an exponential backoff so the main loop keeps
    // reading sensors, driving servos, and scanning buttons uninterrupted.
    if (WiFi.status() != WL_CONNECTED) {
        return;
    }

    unsigned long now = millis();
    if (now - _lastReconnectAttempt < _reconnectBackoffMs) {
        return;
    }
    _lastReconnectAttempt = now;

    Serial.print("[MQTT] Reconnect attempt (backoff ");
    Serial.print(_reconnectBackoffMs);
    Serial.println("ms)...");

    if (!_attemptConnect()) {
        // Exponential backoff, capped.
        _reconnectBackoffMs = min(_reconnectBackoffMs * 2UL,
                                  (unsigned long)MQTT_RECONNECT_MAX_MS);
    }
}

bool MqttHandler::isConnected() {
    return _mqttClient.connected();
}

// ==========================================
// Static Callback Bridge
// ==========================================
void MqttHandler::_staticCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance) {
        _instance->_handleMessage(topic, payload, length);
    }
}

void MqttHandler::_handleMessage(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, TOPIC_SYSTEM_STATUS) != 0) {
        return;
    }

    // Null-terminate the payload into a bounded stack buffer.
    char message[JSON_BUFFER_SIZE];
    unsigned int copyLen = min(length, (unsigned int)(JSON_BUFFER_SIZE - 1));
    memcpy(message, payload, copyLen);
    message[copyLen] = '\0';

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.print("[MQTT] JSON parse error: ");
        Serial.println(error.c_str());
        return;
    }

    // Copy fields into fixed buffers (no Arduino String). Buzzer + LED are
    // derived from `state` on the ESP32 (applyState), so they are not parsed here.
    strlcpy(_latestCommand.state,    doc["state"]                 | "", sizeof(_latestCommand.state));
    strlcpy(_latestCommand.gasValve, doc["actions"]["gas_valve"]  | "", sizeof(_latestCommand.gasValve));
    strlcpy(_latestCommand.doors,    doc["actions"]["doors"]      | "", sizeof(_latestCommand.doors));
    strlcpy(_latestCommand.pump1,    doc["actions"]["pump1"]      | "", sizeof(_latestCommand.pump1));
    strlcpy(_latestCommand.pump2,    doc["actions"]["pump2"]      | "", sizeof(_latestCommand.pump2));
    _latestCommand.confidence = doc["confidence"] | 0.0f;
    _latestCommand.isValid    = (strlen(_latestCommand.state) > 0);
    _newCommandAvailable      = _latestCommand.isValid;

    Serial.print("[MQTT] CMD state=");
    Serial.print(_latestCommand.state);
    Serial.print(" valve=");
    Serial.print(_latestCommand.gasValve);
    Serial.print(" doors=");
    Serial.print(_latestCommand.doors);
    Serial.print(" p1=");
    Serial.print(_latestCommand.pump1);
    Serial.print(" p2=");
    Serial.println(_latestCommand.pump2);
}

// ==========================================
// Publishing
// ==========================================
void MqttHandler::publishSensorData(float gas1, float gas2, float gas3, float gas4,
                                     float temp, float hum, bool flame,
                                     float waterLevelPct, float waterDistanceCm,
                                     bool manualTrigger,
                                     int rssi, const char* mode) {
    if (!_mqttClient.connected()) return;

    JsonDocument doc;
    doc["ts"] = (unsigned long)(millis() / 1000);

    JsonObject gas = doc["gas"].to<JsonObject>();
    gas["mq2"] = (int)gas1;
    gas["mq5"] = (int)gas2;
    gas["mq6"] = (int)gas3;
    gas["mq7"] = (int)gas4;

    // ArduinoJson serializes floats with minimal round-trip digits — no String temporaries.
    JsonObject env = doc["env"].to<JsonObject>();
    env["temp"] = temp;
    env["hum"]  = hum;

    // (MPU6050 removed — no "motion" object in the payload.)

    JsonObject water = doc["water"].to<JsonObject>();
    water["level_pct"]   = waterLevelPct;
    water["distance_cm"] = waterDistanceCm;

    // Manual fire button — top-level so the AI can intercept it instantly and
    // force the CONFIRMED FIRE state, bypassing camera inference.
    doc["manual_trigger"] = manualTrigger ? 1 : 0;

    JsonObject meta = doc["meta"].to<JsonObject>();
    meta["wifi_rssi"] = rssi;
    meta["uptime_s"]  = (unsigned long)(millis() / 1000);
    meta["heap_free"] = (unsigned long)ESP.getFreeHeap();
    meta["mode"]      = mode;
    meta["flame"]     = flame;

    char buffer[JSON_BUFFER_SIZE];
    serializeJson(doc, buffer, sizeof(buffer));   // null-terminates buffer
    if (!_mqttClient.publish(TOPIC_SENSOR_DATA, buffer)) {
        Serial.println("[MQTT] Failed to publish sensor data!");
    }
}

void MqttHandler::publishHeartbeat() {
    if (!_mqttClient.connected()) return;

    JsonDocument doc;
    doc["ts"]     = (unsigned long)(millis() / 1000);
    doc["status"] = "alive";

    char buffer[96];
    serializeJson(doc, buffer, sizeof(buffer));
    _mqttClient.publish(TOPIC_HEARTBEAT, buffer);
}

void MqttHandler::publishManualAlarm(const char* room, bool active) {
    if (!_mqttClient.connected()) return;

    JsonDocument doc;
    doc["ts"]     = (unsigned long)(millis() / 1000);
    doc["room"]   = room;
    doc["active"] = active;

    char buffer[96];
    serializeJson(doc, buffer, sizeof(buffer));
    _mqttClient.publish(TOPIC_MANUAL_ALARM, buffer);

    Serial.print("[MQTT] Manual alarm: ");
    Serial.print(room);
    Serial.print(" active=");
    Serial.println(active ? "true" : "false");
}

// ==========================================
// Command Access
// ==========================================
bool MqttHandler::hasNewCommand() {
    return _newCommandAvailable;
}

ActuatorCommand MqttHandler::getLatestCommand() {
    _newCommandAvailable = false;  // consume
    return _latestCommand;
}
