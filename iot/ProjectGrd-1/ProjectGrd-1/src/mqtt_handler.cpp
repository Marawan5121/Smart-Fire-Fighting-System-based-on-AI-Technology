#include "mqtt_handler.h"
#include "config.h"
#include <string.h>

MqttHandler* MqttHandler::_instance = nullptr;

MqttHandler::MqttHandler(const char* broker, int port, const char* clientId)
    : _mqttClient(_wifiClient), _broker(broker), _port(port), _clientId(clientId),
      _lastReconnectAttempt(0), _reconnectBackoffMs(MQTT_RECONNECT_MIN_MS),
      _fireCommanded(false), _confidence(0.0f) {
    _instance = this;
}

void MqttHandler::begin() {
    _mqttClient.setServer(_broker, _port);
    _mqttClient.setCallback(_staticCallback);
    _mqttClient.setBufferSize(JSON_BUFFER_SIZE);
    _mqttClient.setKeepAlive(MQTT_KEEPALIVE);
    _mqttClient.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);

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
        _mqttClient.subscribe(TOPIC_SYSTEM_STATUS, 1);   // QoS 1 for AI commands
        Serial.print("[MQTT] Subscribed to: ");
        Serial.println(TOPIC_SYSTEM_STATUS);
        _reconnectBackoffMs = MQTT_RECONNECT_MIN_MS;
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

    // Drop the AI override the moment the link is down; local sensors still act.
    _fireCommanded = false;

    if (WiFi.status() != WL_CONNECTED) return;

    unsigned long now = millis();
    if (now - _lastReconnectAttempt < _reconnectBackoffMs) return;
    _lastReconnectAttempt = now;

    Serial.print("[MQTT] Reconnect attempt (backoff ");
    Serial.print(_reconnectBackoffMs);
    Serial.println("ms)...");

    if (!_attemptConnect()) {
        _reconnectBackoffMs = min(_reconnectBackoffMs * 2UL,
                                  (unsigned long)MQTT_RECONNECT_MAX_MS);
    }
}

bool MqttHandler::isConnected() {
    return _mqttClient.connected();
}

// ==========================================
// Callback bridge
// ==========================================
void MqttHandler::_staticCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance) _instance->_handleMessage(topic, payload, length);
}

void MqttHandler::_handleMessage(char* topic, byte* payload, unsigned int length) {
    if (strcmp(topic, TOPIC_SYSTEM_STATUS) != 0) return;

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

    // Only `state` and `confidence` are used: the ESP32 runs level-driven zonal
    // control locally, so the per-actuator `actions` block is not consumed here.
    const char* state = doc["state"] | "";
    _confidence    = doc["confidence"] | 0.0f;
    _fireCommanded = (strcmp(state, "FIRE") == 0);

    Serial.print("[MQTT] CMD state=");
    Serial.print(state);
    Serial.print(" conf=");
    Serial.println(_confidence, 2);
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

    // gas1..gas4 are Room1..Room4 readings; key each by the sensor physically in
    // that room (bench-locked): R1=MQ-7(CO) R2=MQ-6 R3=MQ-5 R4=MQ-2(smoke). The
    // key set is unchanged, so downstream parsers (Node-RED / AI) are unaffected.
    JsonObject gas = doc["gas"].to<JsonObject>();
    gas["mq7"] = (int)gas1;   // Room 1 (CO)
    gas["mq6"] = (int)gas2;   // Room 2
    gas["mq5"] = (int)gas3;   // Room 3
    gas["mq2"] = (int)gas4;   // Room 4 (smoke)

    JsonObject env = doc["env"].to<JsonObject>();
    env["temp"] = temp;
    env["hum"]  = hum;

    JsonObject water = doc["water"].to<JsonObject>();
    water["level_pct"]   = waterLevelPct;
    water["distance_cm"] = waterDistanceCm;

    // Top-level so the AI can intercept it instantly and force CONFIRMED FIRE.
    doc["manual_trigger"] = manualTrigger ? 1 : 0;

    JsonObject meta = doc["meta"].to<JsonObject>();
    meta["wifi_rssi"] = rssi;
    meta["uptime_s"]  = (unsigned long)(millis() / 1000);
    meta["heap_free"] = (unsigned long)ESP.getFreeHeap();
    meta["mode"]      = mode;
    meta["flame"]     = flame;

    char buffer[JSON_BUFFER_SIZE];
    serializeJson(doc, buffer, sizeof(buffer));
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
