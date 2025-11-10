/**
 * WiFi Manager - Handle WiFi connection and AP mode
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#ifdef ESP32
    #include <WiFi.h>
#else
    #include <ESP8266WiFi.h>
#endif

class WiFiManager {
public:
    WiFiManager();
    void begin();
    void loop();

    // Connection status
    bool isConnected();
    bool isAPMode();
    String getSSID();
    int8_t getRSSI();
    String getIPAddress();
    String getMacAddress();

    // WiFi operations
    bool connect(const String& ssid, const String& password);
    void startAP();
    void stopAP();
    String scanNetworks();

    // Credentials management
    void clearCredentials();
    void reset();

private:
    bool m_connected;
    bool m_apMode;
    unsigned long m_lastReconnectAttempt;
    uint8_t m_reconnectAttempts;
    String m_savedSSID;
    String m_savedPassword;

    void loadCredentials();
    void saveCredentials(const String& ssid, const String& password);
    void reconnect();
};

#endif // WIFI_MANAGER_H
