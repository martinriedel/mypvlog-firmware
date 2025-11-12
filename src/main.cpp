/**
 * mypvlog Firmware - Main Entry Point
 *
 * Dual-mode firmware for ESP32/ESP8266:
 * - Generic MQTT Mode: OpenDTU/AhoyDTU compatible
 * - mypvlog Direct Mode: Cloud integration with mypvlog.net
 *
 * Copyright (c) 2025 mypvlog.net
 * Licensed under MIT License
 */

#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "web_server.h"
#include "config_manager.h"
#include "mqtt_client.h"
#include "mypvlog_api.h"
#include "ota_updater.h"

#ifdef RADIO_NRF24
#include "hoymiles_hm.h"
#endif

#ifdef RADIO_CMT2300A
#include "hoymiles_hms.h"
#endif

#ifdef APSYSTEMS_ECU
#include "apsystems_ecu.h"
#endif

// ============================================
// Global Instances
// ============================================

WiFiManager wifiManager;
WebServer webServer;
ConfigManager configManager;
MqttClient mqttClient;
MypvlogAPI mypvlogAPI;
OTAUpdater otaUpdater;

#ifdef RADIO_NRF24
HoymilesHM hoymilesHM;
#endif

#ifdef RADIO_CMT2300A
HoymilesHMS hoymilesHMS;
#endif

#ifdef APSYSTEMS_ECU
APSystemsECU apSystemsECU;
#endif

// ============================================
// Heartbeat Timer
// ============================================

unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 60000; // 60 seconds

unsigned long lastFirmwareCheck = 0;
const unsigned long FIRMWARE_CHECK_INTERVAL = 3600000; // 1 hour

bool updateInProgress = false;

// ============================================
// OTA Update Callback
// ============================================

void onOTAProgress(OTAStatus status, int progress, const String& message) {
    // Log progress to serial
    DEBUG_PRINT("OTA Update: ");
    DEBUG_PRINT(message);
    if (progress >= 0) {
        DEBUG_PRINT(" (");
        DEBUG_PRINT(progress);
        DEBUG_PRINTLN("%)");
    } else {
        DEBUG_PRINTLN();
    }

    // TODO: Update web UI with progress
    // Could be done via WebSocket or SSE
}

// ============================================
// Inverter Data Callbacks
// ============================================

// Hoymiles/TSUN callback
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

#ifdef APSYSTEMS_ECU
// APSystems callback - converts multi-channel data to MQTT
void onAPSystemsData(const char* uid, const APSystemsInverterData& data) {
    DEBUG_PRINT("APSystems Inverter ");
    DEBUG_PRINT(uid);
    DEBUG_PRINT(": ");

    // Calculate total power and average voltage
    float totalPower = 0.0;
    float totalVoltage = 0.0;
    for (uint8_t i = 0; i < data.channelCount; i++) {
        totalPower += data.power[i];
        totalVoltage += data.voltage[i];
    }
    float avgVoltage = (data.channelCount > 0) ? (totalVoltage / data.channelCount) : 0.0;
    float avgCurrent = (avgVoltage > 0) ? (totalPower / avgVoltage) : 0.0;

    DEBUG_PRINT("Online=");
    DEBUG_PRINT(data.online ? "yes" : "no");
    DEBUG_PRINT(", Power=");
    DEBUG_PRINT(totalPower);
    DEBUG_PRINT("W, Temp=");
    DEBUG_PRINT(data.temperature);
    DEBUG_PRINTLN("°C");

    // Publish to MQTT if connected
    if (mqttClient.isConnected() && data.online) {
        String topic;
        OperationMode mode = configManager.getMode();

        if (mode == OperationMode::GENERIC_MQTT) {
            MqttConfig config = configManager.getMqttConfig();
            topic = config.topic_prefix + "/" + wifiManager.getMacAddress() + "/" + String(uid);
        } else if (mode == OperationMode::MYPVLOG_DIRECT) {
            MyPVLogConfig config = configManager.getMyPVLogConfig();
            topic = "opendtu/" + config.dtu_id + "/" + String(uid);
        }

        if (topic.length() > 0) {
            // Main data payload
            String payload = "{\"power\":" + String(totalPower) +
                           ",\"voltage\":" + String(avgVoltage) +
                           ",\"current\":" + String(avgCurrent) +
                           ",\"temperature\":" + String(data.temperature) +
                           ",\"frequency\":" + String(data.frequency) +
                           ",\"signal\":" + String(data.signalStrength);

            // Add per-channel data
            payload += ",\"channels\":[";
            for (uint8_t i = 0; i < data.channelCount; i++) {
                if (i > 0) payload += ",";
                payload += "{\"power\":" + String(data.power[i]) +
                          ",\"voltage\":" + String(data.voltage[i]) + "}";
            }
            payload += "]}";

            mqttClient.publish(topic + "/data", payload);
        }
    }
}
#endif

// ============================================
// Setup Function
// ============================================

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Wait up to 3s for serial

    Serial.println();
    Serial.println("========================================");
    Serial.println("  mypvlog Firmware v" VERSION);
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
    Serial.println("Radio: NRF24L01+ (Hoymiles HM/TSUN)");
    #endif

    #ifdef RADIO_CMT2300A
    Serial.println("Radio: CMT2300A (Hoymiles HMS/HMT)");
    #endif

    #ifdef APSYSTEMS_ECU
    Serial.println("Manufacturer: APSystems (ECU Gateway)");
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
            Serial.println("mypvlog Direct");
            break;
        default:
            Serial.println("Not Configured (Setup Required)");
            break;
    }
    Serial.println();

    // Initialize mypvlog API client (used in Direct mode)
    if (mode == OperationMode::MYPVLOG_DIRECT) {
        mypvlogAPI.begin(MYPVLOG_API_URL);
        Serial.println("mypvlog API: Initialized");
        Serial.print("  API URL: ");
        Serial.println(MYPVLOG_API_URL);
        Serial.println();
    }

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
        Serial.print("  Connect to WiFi: mypvlog-");
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

    // Step 6: Initialize APSystems ECU (if configured)
    #ifdef APSYSTEMS_ECU
    if (configManager.isConfigured()) {
        Serial.println();

        // TODO: Get ECU IP from configuration
        // For now, using a placeholder - this would come from config
        const char* ecuIP = "192.168.1.100";  // Default - should be configured via web UI

        if (apSystemsECU.begin(ecuIP)) {
            apSystemsECU.setDataCallback(onAPSystemsData);

            // Set poll interval based on mode
            if (mode == OperationMode::MYPVLOG_DIRECT) {
                apSystemsECU.setPollInterval(HOYMILES_POLL_INTERVAL_FAST);
            }

            Serial.println("APSystems ECU: Ready");
            Serial.print("  ECU ID: ");
            Serial.println(apSystemsECU.getECUID());
            Serial.print("  Inverters: ");
            Serial.println(apSystemsECU.getInverterCount());
        } else {
            Serial.println("APSystems ECU: Failed to initialize");
            Serial.println("  Check ECU IP address in configuration");
        }
    }
    #endif

    // Check for firmware updates (mypvlog Direct mode only)
    if (mode == OperationMode::MYPVLOG_DIRECT && wifiManager.isConnected()) {
        Serial.println();
        Serial.println("Checking for firmware updates...");

        #ifdef ESP32
            String hardwareModel = "esp32-";
        #elif defined(ESP8266)
            String hardwareModel = "esp8266-";
        #endif

        #ifdef RADIO_NRF24
            hardwareModel += "nrf24";
        #elif defined(RADIO_CMT2300A)
            hardwareModel += "cmt2300a";
        #else
            hardwareModel += "dual";
        #endif

        FirmwareUpdateInfo updateInfo = mypvlogAPI.checkFirmwareUpdate(VERSION, hardwareModel);
        if (updateInfo.updateAvailable) {
            Serial.println("  Update available!");
            Serial.print("  New version: ");
            Serial.println(updateInfo.version);
            Serial.println();
            Serial.println("  Starting OTA update...");

            // Trigger OTA update
            bool updateSuccess = otaUpdater.performUpdate(
                updateInfo.downloadUrl,
                updateInfo.checksum,
                onOTAProgress
            );

            if (!updateSuccess) {
                Serial.print("  OTA update failed: ");
                Serial.println(otaUpdater.getLastError());
            }
            // If successful, device will reboot
        } else {
            Serial.println("  Firmware is up to date");
        }
    }

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

    #ifdef APSYSTEMS_ECU
    if (configManager.isConfigured()) {
        apSystemsECU.loop();
    }
    #endif

    // mypvlog Direct mode: Send heartbeat
    if (configManager.getMode() == OperationMode::MYPVLOG_DIRECT &&
        wifiManager.isConnected() &&
        millis() - lastHeartbeat >= HEARTBEAT_INTERVAL) {

        lastHeartbeat = millis();

        MyPVLogConfig config = configManager.getMyPVLogConfig();

        HeartbeatResponse response = mypvlogAPI.sendHeartbeat(
            config.dtu_id,
            config.mqtt_password,
            millis() / 1000, // uptime in seconds
            ESP.getFreeHeap(),
            wifiManager.getRSSI(),
            wifiManager.getIPAddress()
        );

        if (response.success && response.configChanged) {
            DEBUG_PRINTLN("Config changed on server - device should re-provision");
            // TODO: Implement re-provisioning logic
        }
    }

    // mypvlog Direct mode: Check for firmware updates periodically
    if (configManager.getMode() == OperationMode::MYPVLOG_DIRECT &&
        wifiManager.isConnected() &&
        !updateInProgress &&
        millis() - lastFirmwareCheck >= FIRMWARE_CHECK_INTERVAL) {

        lastFirmwareCheck = millis();

        #ifdef ESP32
            String hardwareModel = "esp32-";
        #elif defined(ESP8266)
            String hardwareModel = "esp8266-";
        #endif

        #ifdef RADIO_NRF24
            hardwareModel += "nrf24";
        #elif defined(RADIO_CMT2300A)
            hardwareModel += "cmt2300a";
        #else
            hardwareModel += "dual";
        #endif

        FirmwareUpdateInfo updateInfo = mypvlogAPI.checkFirmwareUpdate(VERSION, hardwareModel);
        if (updateInfo.updateAvailable) {
            DEBUG_PRINT("Firmware update available: ");
            DEBUG_PRINT(updateInfo.version);
            DEBUG_PRINTLN(" - Starting OTA update...");

            updateInProgress = true;

            // Trigger OTA update
            bool updateSuccess = otaUpdater.performUpdate(
                updateInfo.downloadUrl,
                updateInfo.checksum,
                onOTAProgress
            );

            if (!updateSuccess) {
                DEBUG_PRINT("OTA update failed: ");
                DEBUG_PRINTLN(otaUpdater.getLastError());
                updateInProgress = false;
            }
            // If successful, device will reboot
        }
    }

    // Small delay to prevent watchdog triggers
    delay(10);
}
