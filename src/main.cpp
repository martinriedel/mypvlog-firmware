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

#ifdef RADIO_NRF24
#include "hoymiles_hm.h"
#endif

#ifdef RADIO_CMT2300A
#include "hoymiles_hms.h"
#endif

// Global instances
WiFiManager wifiManager;
WebServer webServer;

#ifdef RADIO_NRF24
HoymilesHM hoymilesHM;
#endif

#ifdef RADIO_CMT2300A
HoymilesHMS hoymilesHMS;
#endif

void setup() {
    // Initialize serial for debugging
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Wait up to 3s for serial

    Serial.println();
    Serial.println("========================================");
    Serial.println("  MyPVLog Firmware v" VERSION);
    Serial.println("  Build: " __DATE__ " " __TIME__);
    Serial.println("========================================");

    #ifdef ESP32
    Serial.println("Platform: ESP32");
    #elif defined(ESP8266)
    Serial.println("Platform: ESP8266");
    #endif

    #ifdef RADIO_NRF24
    Serial.println("Radio: NRF24L01+");
    #endif

    #ifdef RADIO_CMT2300A
    Serial.println("Radio: CMT2300A");
    #endif

    Serial.println();

    // Initialize WiFi Manager
    // Will create AP if no credentials saved, or connect to saved WiFi
    wifiManager.begin();

    // Initialize web server (setup wizard or local UI)
    webServer.begin();

    #ifdef RADIO_NRF24
    // Initialize Hoymiles HM protocol (NRF24)
    hoymilesHM.begin();
    #endif

    #ifdef RADIO_CMT2300A
    // Initialize Hoymiles HMS/HMT protocol (CMT2300A)
    hoymilesHMS.begin();
    #endif

    Serial.println("Initialization complete!");
    Serial.println("========================================");
}

void loop() {
    // Handle WiFi
    wifiManager.loop();

    // Handle web server
    webServer.loop();

    #ifdef RADIO_NRF24
    // Poll Hoymiles HM inverters
    hoymilesHM.loop();
    #endif

    #ifdef RADIO_CMT2300A
    // Poll Hoymiles HMS/HMT inverters
    hoymilesHMS.loop();
    #endif

    // Small delay to prevent watchdog triggers
    delay(10);
}
