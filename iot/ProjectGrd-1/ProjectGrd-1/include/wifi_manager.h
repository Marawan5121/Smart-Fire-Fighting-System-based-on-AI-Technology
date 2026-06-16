#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>

/**
 * @brief Non-blocking WiFi connection manager for ESP32.
 * Handles initial connection, automatic reconnection, and status reporting.
 */
class WifiManager {
private:
    const char* _ssid;
    const char* _password;
    unsigned long _lastAttemptTime;
    unsigned long _retryDelay;
    bool _wasConnected;   // Track previous state for transition logging

public:
    WifiManager(const char* ssid, const char* password, unsigned long retryDelay);

    /** @brief Initialize WiFi in station mode and begin connection */
    void begin();

    /** @brief Non-blocking connection check and reconnect. Call in loop(). */
    void update();

    /** @brief Check if WiFi is currently connected */
    bool isConnected() const;

    /** @brief Get current WiFi signal strength in dBm */
    int getRSSI() const;

    /** @brief Get the assigned local IP address */
    String getIP() const;
};

#endif // WIFI_MANAGER_H
