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

#ifdef RADIO_NRF24
// #include "hoymiles_hm.h" // To be implemented
#endif

#ifdef RADIO_CMT2300A
// #include "hoymiles_hms.h" // To be implemented
#endif

// ============================================
// Global Instances
// ============================================

WiFiManager wifiManager;
WebServer webServer;
ConfigManager configManager;

#ifdef RADIO_NRF24
// HoymilesHM hoymilesHM; // To be implemented
#endif

#ifdef RADIO_CMT2300A
// HoymilesHMS hoymilesHMS; // To be implemented
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

    // Step 4: Initialize Hoymiles Protocol (if configured)
    #ifdef RADIO_NRF24
    if (configManager.isConfigured()) {
        // hoymilesHM.begin(); // To be implemented
        Serial.println("Hoymiles HM: Ready (implementation pending)");
    }
    #endif

    #ifdef RADIO_CMT2300A
    if (configManager.isConfigured()) {
        // hoymilesHMS.begin(); // To be implemented
        Serial.println("Hoymiles HMS/HMT: Ready (implementation pending)");
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

    // Handle inverter polling (if configured)
    #ifdef RADIO_NRF24
    if (configManager.isConfigured()) {
        // hoymilesHM.loop(); // To be implemented
    }
    #endif

    #ifdef RADIO_CMT2300A
    if (configManager.isConfigured()) {
        // hoymilesHMS.loop(); // To be implemented
    }
    #endif

    // Small delay to prevent watchdog triggers
    delay(10);
}
