/**
 * MQTT Client - Unified MQTT client for both modes
 *
 * Supports:
 * - Generic MQTT mode (user-configured broker)
 * - MyPVLog Direct mode (cloud broker with SSL)
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <PubSubClient.h>
#include "config_manager.h"

#ifdef ESP32
    #include <WiFi.h>
    #include <WiFiClientSecure.h>
#else
    #include <ESP8266WiFi.h>
    #include <WiFiClientSecure.h>
#endif

class MqttClient {
public:
    MqttClient();

    // Initialize with configuration
    void begin(const MqttConfig& config, bool useSSL = false);
    void beginMyPVLog(const MyPVLogConfig& config);

    // Connection management
    bool connect();
    void disconnect();
    bool isConnected();
    void loop();

    // Publishing
    bool publish(const String& topic, const String& payload, bool retained = false);
    bool publish(const String& topic, const char* payload, bool retained = false);

    // Subscribing
    bool subscribe(const String& topic);
    void setCallback(std::function<void(String topic, String payload)> callback);

    // Status
    String getLastError();
    unsigned long getLastReconnectAttempt();
    uint16_t getReconnectInterval() { return m_reconnectInterval; }

private:
    WiFiClient m_wifiClient;
    WiFiClientSecure m_wifiClientSecure;
    PubSubClient* m_mqttClient;

    // Configuration
    String m_broker;
    uint16_t m_port;
    String m_username;
    String m_password;
    String m_clientId;
    bool m_useSSL;

    // State
    bool m_initialized;
    unsigned long m_lastReconnectAttempt;
    uint16_t m_reconnectInterval;
    String m_lastError;

    // Callback
    std::function<void(String topic, String payload)> m_messageCallback;

    // Internal methods
    void reconnect();
    static void staticCallback(char* topic, byte* payload, unsigned int length);
    void handleMessage(char* topic, byte* payload, unsigned int length);
    String generateClientId();
};

#endif // MQTT_CLIENT_H
