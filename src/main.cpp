/**
 * MyPVLog Firmware - Main Entry Point
 *
 * Dual-mode firmware for ESP32/ESP8266:
 * - Generic MQTT Mode: OpenDTU/AhoyDTU compatible
 * - MyPVLog Direct Mode: Cloud integration with mypvlog.net
 *
 * Copyright (c) 2025 MyPVLog.net
 * Licensed under MIT License
 */

#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "config_manager.h"
#include "mqtt_client.h"

#ifdef RADIO_NRF24
#include "hoymiles_hm.h"
#endif

#ifdef RADIO_CMT2300A
#include "hoymiles_hms.h"
#endif

// ============================================
// Global Instances
// ============================================

WiFiManager wifiManager;
WebServer webServer;
ConfigManager configManager;
MqttClient mqttClient;

#ifdef RADIO_NRF24
HoymilesHM hoymilesHM;
#endif

#ifdef RADIO_CMT2300A
HoymilesHMS hoymilesHMS;
#endif

// ============================================
// Inverter Data Callback
// ============================================

void onInverterData(uint64_t serial, float power, float voltage, float current) {
    DEBUG_PRINT("Inverter ");
    DEBUG_PRINT((unsigned long)serial);
    DEBUG_PRINT(": Power=");
    DEBUG_PRINT(power);
    DEBUG_PRINT("W, Voltage=");
    DEBUG_PRINT(voltage);
    DEBUG_PRINT("V, Current=");
    DEBUG_PRINT(current);
    DEBUG_PRINTLN("A");

    // Publish to MQTT if connected
    if (mqttClient.isConnected()) {
        String topic;
        OperationMode mode = configManager.getMode();

        if (mode == OperationMode::GENERIC_MQTT) {
            MqttConfig config = configManager.getMqttConfig();
            topic = config.topic_prefix + "/" + wifiManager.getMacAddress() + "/" + String((unsigned long)serial);
        } else if (mode == OperationMode::MYPVLOG_DIRECT) {
            MyPVLogConfig config = configManager.getMyPVLogConfig();
            topic = "opendtu/" + config.dtu_id + "/" + String((unsigned long)serial);
        }

        if (topic.length() > 0) {
            String payload = "{\"power\":" + String(power) +
                           ",\"voltage\":" + String(voltage) +
                           ",\"current\":" + String(current) + "}";

            mqttClient.publish(topic + "/data", payload);
        }
    }
}

// ============================================
// Setup Function
// ============================================

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Wait up to 3s for serial

    Serial.println();
    Serial.println("========================================");
    Serial.println("  MyPVLog Firmware v" VERSION);
    Serial.println("  Build: " __DATE__ " " __TIME__);
    Serial.println("========================================");

    // Display platform information
    #ifdef ESP32
    Serial.println("Platform: ESP32");
    #elif defined(ESP32S3)
    Serial.println("Platform: ESP32-S3");
    #elif defined(ESP8266)
    Serial.println("Platform: ESP8266");
    #endif

    // Display radio configuration
    #ifdef RADIO_NRF24
    Serial.println("Radio: NRF24L01+ (Hoymiles HM)");
    #endif

    #ifdef RADIO_CMT2300A
    Serial.println("Radio: CMT2300A (Hoymiles HMS/HMT)");
    #endif

    Serial.println();

    // Step 1: Initialize Configuration Manager
    configManager.begin();

    OperationMode mode = configManager.getMode();
    Serial.print("Operation Mode: ");
    switch (mode) {
        case OperationMode::GENERIC_MQTT:
            Serial.println("Generic MQTT");
            break;
        case OperationMode::MYPVLOG_DIRECT:
            Serial.println("MyPVLog Direct");
            break;
        default:
            Serial.println("Not Configured (Setup Required)");
            break;
    }
    Serial.println();

    // Step 2: Initialize WiFi Manager
    // Will create AP if no credentials saved, or connect to saved WiFi
    wifiManager.begin();

    // Step 3: Initialize Web Server
    // Serves setup wizard in AP mode, or local UI in client mode
    webServer.begin();

    if (wifiManager.isAPMode()) {
        Serial.println("========================================");
        Serial.println("  SETUP MODE");
        Serial.println("========================================");
        Serial.print("  Connect to WiFi: MyPVLog-");
        String mac = wifiManager.getMacAddress();
        mac.replace(":", "");
        Serial.println(mac.substring(mac.length() - 4));
        Serial.print("  Password: ");
        Serial.println(WIFI_AP_PASSWORD);
        Serial.print("  Then open: http://");
        Serial.println(wifiManager.getIPAddress());
        Serial.println("========================================");
    } else {
        Serial.println("========================================");
        Serial.println("  CONNECTED");
        Serial.println("========================================");
        Serial.print("  Network: ");
        Serial.println(wifiManager.getSSID());
        Serial.print("  IP Address: http://");
        Serial.println(wifiManager.getIPAddress());
        Serial.print("  Signal: ");
        Serial.print(wifiManager.getRSSI());
        Serial.println(" dBm");
        Serial.println("========================================");
    }

    // Step 4: Initialize MQTT (if configured and WiFi connected)
    if (configManager.isConfigured() && wifiManager.isConnected()) {
        Serial.println();
        Serial.println("Initializing MQTT...");

        if (mode == OperationMode::GENERIC_MQTT) {
            MqttConfig mqttConfig = configManager.getMqttConfig();
            mqttClient.begin(mqttConfig, mqttConfig.ssl);

            Serial.print("  Broker: ");
            Serial.print(mqttConfig.host);
            Serial.print(":");
            Serial.println(mqttConfig.port);
            Serial.print("  SSL: ");
            Serial.println(mqttConfig.ssl ? "Yes" : "No");
        } else if (mode == OperationMode::MYPVLOG_DIRECT) {
            MyPVLogConfig pvlogConfig = configManager.getMyPVLogConfig();
            mqttClient.beginMyPVLog(pvlogConfig);

            Serial.println("  Broker: mqtt.mypvlog.net:8883 (SSL)");
            Serial.print("  DTU ID: ");
            Serial.println(pvlogConfig.dtu_id);
        }

        // Try to connect
        if (mqttClient.connect()) {
            Serial.println("  Status: Connected!");
        } else {
            Serial.print("  Status: Connection failed - ");
            Serial.println(mqttClient.getLastError());
        }
    }

    // Step 5: Initialize Hoymiles Protocol (if configured)
    #ifdef RADIO_NRF24
    if (configManager.isConfigured()) {
        Serial.println();
        hoymilesHM.begin();
        hoymilesHM.setDataCallback(onInverterData);

        // Set poll interval based on mode
        if (mode == OperationMode::MYPVLOG_DIRECT) {
            hoymilesHM.setPollInterval(HOYMILES_POLL_INTERVAL_FAST);
        }

        Serial.println("Hoymiles HM: Ready");
    }
    #endif

    #ifdef RADIO_CMT2300A
    if (configManager.isConfigured()) {
        Serial.println();
        hoymilesHMS.begin();
        hoymilesHMS.setDataCallback(onInverterData);

        // Set poll interval based on mode
        if (mode == OperationMode::MYPVLOG_DIRECT) {
            hoymilesHMS.setPollInterval(HOYMILES_POLL_INTERVAL_FAST);
        }

        Serial.println("Hoymiles HMS/HMT: Ready");
    }
    #endif

    Serial.println();
    Serial.println("Initialization complete!");
    Serial.println();
}

// ============================================
// Main Loop
// ============================================

void loop() {
    // Handle WiFi (reconnection, AP mode)
    wifiManager.loop();

    // Handle web server (HTTP requests, captive portal DNS)
    webServer.loop();

    // Handle MQTT (reconnection, message processing)
    if (configManager.isConfigured() && wifiManager.isConnected()) {
        mqttClient.loop();
    }

    // Handle inverter polling (if configured)
    #ifdef RADIO_NRF24
    if (configManager.isConfigured()) {
        hoymilesHM.loop();
    }
    #endif

    #ifdef RADIO_CMT2300A
    if (configManager.isConfigured()) {
        hoymilesHMS.loop();
    }
    #endif

    // Small delay to prevent watchdog triggers
    delay(10);
}
