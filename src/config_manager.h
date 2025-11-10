/**
 * Configuration Manager - Centralized configuration storage
 *
 * Handles all persistent configuration using ESP32 Preferences (NVS)
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>

// Operation modes
enum class OperationMode {
    NOT_CONFIGURED,
    GENERIC_MQTT,
    MYPVLOG_DIRECT
};

// MQTT Configuration
struct MqttConfig {
    String host;
    uint16_t port;
    bool ssl;
    String username;
    String password;
    String topic_prefix;
};

// MyPVLog Configuration
struct MyPVLogConfig {
    String dtu_id;
    String mqtt_username;
    String mqtt_password;
    String api_token;
};

class ConfigManager {
public:
    ConfigManager();

    // Initialize configuration
    void begin();

    // Operation mode
    OperationMode getMode();
    void setMode(OperationMode mode);

    // Generic MQTT configuration
    MqttConfig getMqttConfig();
    void setMqttConfig(const MqttConfig& config);

    // MyPVLog Direct configuration
    MyPVLogConfig getMyPVLogConfig();
    void setMyPVLogConfig(const MyPVLogConfig& config);

    // Configuration status
    bool isConfigured();

    // Factory reset
    void factoryReset();

private:
    OperationMode m_mode;
};

#endif // CONFIG_MANAGER_H
