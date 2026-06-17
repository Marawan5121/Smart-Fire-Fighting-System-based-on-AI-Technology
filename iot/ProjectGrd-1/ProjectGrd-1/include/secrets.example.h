/**
 * ============================================================
 * 🔐 SFFS — SECRETS TEMPLATE (safe to commit)
 * ============================================================
 * Copy this file to `secrets.h` (same folder) and fill in your
 * real values. `secrets.h` is gitignored and will NOT be tracked.
 *
 *     cp include/secrets.example.h include/secrets.h
 */
#ifndef SECRETS_H
#define SECRETS_H

// --- WiFi ---
#define WIFI_SSID          "YOUR_WIFI_SSID"
#define WIFI_PASSWORD      "YOUR_WIFI_PASSWORD"

// --- MQTT Broker (laptop running Mosquitto) ---
#define MQTT_BROKER        "192.168.1.100"   // your laptop's LAN IP
#define MQTT_USERNAME      ""                // leave empty for anonymous broker
#define MQTT_PASSWORD      ""

#endif // SECRETS_H
