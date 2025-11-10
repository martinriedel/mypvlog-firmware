/**
 * MQTT Client - Unified MQTT client implementation
 */

#include "mqtt_client.h"
#include "config.h"

// Static instance pointer for callback
static MqttClient* instance = nullptr;

MqttClient::MqttClient()
    : m_mqttClient(nullptr)
    , m_port(MQTT_DEFAULT_PORT)
    , m_useSSL(false)
    , m_initialized(false)
    , m_lastReconnectAttempt(0)
    , m_reconnectInterval(MQTT_RECONNECT_INTERVAL)
{
    instance = this;
}

void MqttClient::begin(const MqttConfig& config, bool useSSL) {
    DEBUG_PRINTLN("MQTT Client: Initializing Generic MQTT mode");

    m_broker = config.host;
    m_port = config.port;
    m_username = config.username;
    m_password = config.password;
    m_useSSL = useSSL || config.ssl;
    m_clientId = generateClientId();

    // Create MQTT client with appropriate WiFi client
    if (m_useSSL) {
        DEBUG_PRINTLN("MQTT Client: Using SSL/TLS");
        m_wifiClientSecure.setInsecure(); // For now, don't validate certificates
        m_mqttClient = new PubSubClient(m_wifiClientSecure);
    } else {
        m_mqttClient = new PubSubClient(m_wifiClient);
    }

    m_mqttClient->setServer(m_broker.c_str(), m_port);
    m_mqttClient->setCallback(staticCallback);
    m_mqttClient->setKeepAlive(MQTT_DEFAULT_KEEPALIVE);

    m_initialized = true;

    DEBUG_PRINT("MQTT Client: Broker: ");
    DEBUG_PRINT(m_broker);
    DEBUG_PRINT(":");
    DEBUG_PRINTLN(m_port);
    DEBUG_PRINT("MQTT Client: Client ID: ");
    DEBUG_PRINTLN(m_clientId);
}

void MqttClient::beginMyPVLog(const MyPVLogConfig& config) {
    DEBUG_PRINTLN("MQTT Client: Initializing MyPVLog Direct mode");

    m_broker = MYPVLOG_MQTT_BROKER;
    m_port = MYPVLOG_MQTT_PORT;
    m_username = config.mqtt_username;
    m_password = config.mqtt_password;
    m_useSSL = true;
    m_clientId = config.dtu_id;

    // Always use SSL for MyPVLog
    DEBUG_PRINTLN("MQTT Client: Using SSL/TLS (MyPVLog)");
    m_wifiClientSecure.setInsecure(); // TODO: Add certificate pinning
    m_mqttClient = new PubSubClient(m_wifiClientSecure);

    m_mqttClient->setServer(m_broker.c_str(), m_port);
    m_mqttClient->setCallback(staticCallback);
    m_mqttClient->setKeepAlive(MQTT_DEFAULT_KEEPALIVE);

    m_initialized = true;

    DEBUG_PRINT("MQTT Client: Broker: ");
    DEBUG_PRINT(m_broker);
    DEBUG_PRINT(":");
    DEBUG_PRINTLN(m_port);
    DEBUG_PRINT("MQTT Client: DTU ID: ");
    DEBUG_PRINTLN(m_clientId);
}

bool MqttClient::connect() {
    if (!m_initialized) {
        m_lastError = "Not initialized";
        DEBUG_PRINTLN("MQTT Client: Error - Not initialized");
        return false;
    }

    DEBUG_PRINT("MQTT Client: Connecting to ");
    DEBUG_PRINT(m_broker);
    DEBUG_PRINT(":");
    DEBUG_PRINT(m_port);
    DEBUG_PRINTLN("...");

    bool connected = false;

    if (m_username.length() > 0) {
        connected = m_mqttClient->connect(
            m_clientId.c_str(),
            m_username.c_str(),
            m_password.c_str()
        );
    } else {
        connected = m_mqttClient->connect(m_clientId.c_str());
    }

    if (connected) {
        DEBUG_PRINTLN("MQTT Client: Connected!");
        m_lastError = "";
        return true;
    } else {
        int state = m_mqttClient->state();
        m_lastError = "Connection failed, state: " + String(state);

        DEBUG_PRINT("MQTT Client: Connection failed, state: ");
        DEBUG_PRINTLN(state);

        return false;
    }
}

void MqttClient::disconnect() {
    if (m_mqttClient && m_mqttClient->connected()) {
        m_mqttClient->disconnect();
        DEBUG_PRINTLN("MQTT Client: Disconnected");
    }
}

bool MqttClient::isConnected() {
    return m_mqttClient && m_mqttClient->connected();
}

void MqttClient::loop() {
    if (!m_initialized) {
        return;
    }

    // Handle MQTT messages
    if (m_mqttClient->connected()) {
        m_mqttClient->loop();
    } else {
        // Try to reconnect
        unsigned long now = millis();
        if (now - m_lastReconnectAttempt > m_reconnectInterval) {
            m_lastReconnectAttempt = now;
            reconnect();
        }
    }
}

void MqttClient::reconnect() {
    DEBUG_PRINTLN("MQTT Client: Attempting to reconnect...");

    if (connect()) {
        DEBUG_PRINTLN("MQTT Client: Reconnected successfully");
    } else {
        DEBUG_PRINTLN("MQTT Client: Reconnection failed, will retry");
    }
}

bool MqttClient::publish(const String& topic, const String& payload, bool retained) {
    return publish(topic, payload.c_str(), retained);
}

bool MqttClient::publish(const String& topic, const char* payload, bool retained) {
    if (!isConnected()) {
        DEBUG_PRINTLN("MQTT Client: Cannot publish, not connected");
        return false;
    }

    bool success = m_mqttClient->publish(topic.c_str(), payload, retained);

    if (success) {
        DEBUG_PRINT("MQTT Client: Published to ");
        DEBUG_PRINT(topic);
        DEBUG_PRINT(": ");
        DEBUG_PRINTLN(payload);
    } else {
        DEBUG_PRINT("MQTT Client: Publish failed to ");
        DEBUG_PRINTLN(topic);
    }

    return success;
}

bool MqttClient::subscribe(const String& topic) {
    if (!isConnected()) {
        DEBUG_PRINTLN("MQTT Client: Cannot subscribe, not connected");
        return false;
    }

    bool success = m_mqttClient->subscribe(topic.c_str());

    if (success) {
        DEBUG_PRINT("MQTT Client: Subscribed to ");
        DEBUG_PRINTLN(topic);
    } else {
        DEBUG_PRINT("MQTT Client: Subscribe failed to ");
        DEBUG_PRINTLN(topic);
    }

    return success;
}

void MqttClient::setCallback(std::function<void(String topic, String payload)> callback) {
    m_messageCallback = callback;
}

String MqttClient::getLastError() {
    return m_lastError;
}

unsigned long MqttClient::getLastReconnectAttempt() {
    return m_lastReconnectAttempt;
}

void MqttClient::staticCallback(char* topic, byte* payload, unsigned int length) {
    if (instance) {
        instance->handleMessage(topic, payload, length);
    }
}

void MqttClient::handleMessage(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
    String payloadStr;
    payloadStr.reserve(length + 1);
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }

    DEBUG_PRINT("MQTT Client: Received message on ");
    DEBUG_PRINT(topic);
    DEBUG_PRINT(": ");
    DEBUG_PRINTLN(payloadStr);

    // Call user callback if set
    if (m_messageCallback) {
        m_messageCallback(String(topic), payloadStr);
    }
}

String MqttClient::generateClientId() {
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    return "mypvlog-" + mac;
}
