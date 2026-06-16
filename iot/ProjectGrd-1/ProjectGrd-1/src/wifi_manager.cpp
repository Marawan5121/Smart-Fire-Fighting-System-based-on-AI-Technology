#include "wifi_manager.h"
#include "config.h"

WifiManager::WifiManager(const char* ssid, const char* password, unsigned long retryDelay)
    : _ssid(ssid), _password(password), _lastAttemptTime(0),
      _retryDelay(retryDelay), _wasConnected(false) {}

void WifiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);   // ESP32 hardware-level auto-reconnect
    WiFi.begin(_ssid, _password);

    Serial.print("[WiFi] Connecting to: ");
    Serial.print(_ssid);

    // Blocking initial connection (max 10 seconds at boot)
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime < 10000)) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        _wasConnected = true;
        Serial.println("\n[WiFi] Connected!");
        Serial.print("[WiFi] IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("[WiFi] RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
    } else {
        Serial.println("\n[WiFi] Initial connection FAILED. Will retry in background.");
    }
}

void WifiManager::update() {
    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);

    // Detect disconnection transition
    if (_wasConnected && !currentlyConnected) {
        Serial.println("[WiFi] CONNECTION LOST! Attempting reconnect...");
        _wasConnected = false;
        _lastAttemptTime = millis();
    }

    // Non-blocking reconnection attempts
    if (!currentlyConnected) {
        if (millis() - _lastAttemptTime >= _retryDelay) {
            Serial.println("[WiFi] Reconnecting...");
            WiFi.disconnect();
            WiFi.begin(_ssid, _password);
            _lastAttemptTime = millis();
        }
    }

    // Detect reconnection transition
    if (!_wasConnected && currentlyConnected) {
        _wasConnected = true;
        Serial.println("[WiFi] RECONNECTED!");
        Serial.print("[WiFi] IP: ");
        Serial.println(WiFi.localIP());
    }
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

int WifiManager::getRSSI() const {
    return WiFi.RSSI();
}

String WifiManager::getIP() const {
    return WiFi.localIP().toString();
}
