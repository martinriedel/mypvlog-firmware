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
    bool isConnected();
    String getSSID();
    int8_t getRSSI();
    String getIPAddress();
    bool connect(const String& ssid, const String& password);
    void startAP();
    void stopAP();

private:
    bool m_connected;
    unsigned long m_lastReconnectAttempt;
    void loadCredentials();
    void saveCredentials(const String& ssid, const String& password);
    void reconnect();
};

#endif // WIFI_MANAGER_H
