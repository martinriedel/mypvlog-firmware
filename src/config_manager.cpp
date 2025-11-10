/**
 * Configuration Manager - Centralized configuration storage
 */

#include "config_manager.h"
#include "config.h"
#include <Preferences.h>

Preferences configStorage;

ConfigManager::ConfigManager()
    : m_mode(OperationMode::NOT_CONFIGURED)
{
}

void ConfigManager::begin() {
    DEBUG_PRINTLN("Config Manager: Initializing...");

    configStorage.begin("config", true); // Read-only for initial load

    String modeStr = configStorage.getString("mode", "");

    if (modeStr == "generic") {
        m_mode = OperationMode::GENERIC_MQTT;
    } else if (modeStr == "mypvlog") {
        m_mode = OperationMode::MYPVLOG_DIRECT;
    } else {
        m_mode = OperationMode::NOT_CONFIGURED;
    }

    configStorage.end();

    DEBUG_PRINT("Config Manager: Mode = ");
    DEBUG_PRINTLN(modeStr);
}

OperationMode ConfigManager::getMode() {
    return m_mode;
}

void ConfigManager::setMode(OperationMode mode) {
    m_mode = mode;

    configStorage.begin("config", false); // Read-write

    switch (mode) {
        case OperationMode::GENERIC_MQTT:
            configStorage.putString("mode", "generic");
            break;
        case OperationMode::MYPVLOG_DIRECT:
            configStorage.putString("mode", "mypvlog");
            break;
        default:
            configStorage.putString("mode", "");
            break;
    }

    configStorage.end();

    DEBUG_PRINTLN("Config Manager: Mode updated");
}

MqttConfig ConfigManager::getMqttConfig() {
    MqttConfig config;

    configStorage.begin("config", true); // Read-only

    config.host = configStorage.getString("mqtt_host", "");
    config.port = configStorage.getUInt("mqtt_port", MQTT_DEFAULT_PORT);
    config.ssl = configStorage.getBool("mqtt_ssl", false);
    config.username = configStorage.getString("mqtt_user", "");
    config.password = configStorage.getString("mqtt_pass", "");
    config.topic_prefix = configStorage.getString("mqtt_topic", "opendtu");

    configStorage.end();

    return config;
}

void ConfigManager::setMqttConfig(const MqttConfig& config) {
    configStorage.begin("config", false); // Read-write

    configStorage.putString("mqtt_host", config.host);
    configStorage.putUInt("mqtt_port", config.port);
    configStorage.putBool("mqtt_ssl", config.ssl);
    configStorage.putString("mqtt_user", config.username);
    configStorage.putString("mqtt_pass", config.password);
    configStorage.putString("mqtt_topic", config.topic_prefix);

    configStorage.end();

    DEBUG_PRINTLN("Config Manager: MQTT config saved");
}

MyPVLogConfig ConfigManager::getMyPVLogConfig() {
    MyPVLogConfig config;

    configStorage.begin("config", true); // Read-only

    config.dtu_id = configStorage.getString("dtu_id", "");
    config.mqtt_username = configStorage.getString("pvlog_mqtt_user", "");
    config.mqtt_password = configStorage.getString("pvlog_mqtt_pass", "");
    config.api_token = configStorage.getString("pvlog_token", "");

    configStorage.end();

    return config;
}

void ConfigManager::setMyPVLogConfig(const MyPVLogConfig& config) {
    configStorage.begin("config", false); // Read-write

    configStorage.putString("dtu_id", config.dtu_id);
    configStorage.putString("pvlog_mqtt_user", config.mqtt_username);
    configStorage.putString("pvlog_mqtt_pass", config.mqtt_password);
    configStorage.putString("pvlog_token", config.api_token);

    configStorage.end();

    DEBUG_PRINTLN("Config Manager: MyPVLog config saved");
}

bool ConfigManager::isConfigured() {
    return m_mode != OperationMode::NOT_CONFIGURED;
}

void ConfigManager::factoryReset() {
    DEBUG_PRINTLN("Config Manager: Factory reset");

    configStorage.begin("config", false);
    configStorage.clear();
    configStorage.end();

    m_mode = OperationMode::NOT_CONFIGURED;
}
