/**
 * Web Server - Captive portal and local web UI
 *
 * Features:
 * - Serves web UI from LittleFS
 * - Captive portal for AP mode
 * - REST API endpoints for configuration
 * - CORS support for development
 */

#include "web_server.h"
#include "config.h"
#include "wifi_manager.h"

#ifdef ESP32
    #include <WiFi.h>
    #include <ESPAsyncWebServer.h>
    #include <AsyncTCP.h>
    #include <LittleFS.h>
    #include <DNSServer.h>
#else
    #include <ESP8266WiFi.h>
    #include <ESPAsyncWebServer.h>
    #include <ESPAsyncTCP.h>
    #include <LittleFS.h>
    #include <DNSServer.h>
#endif

#include <ArduinoJson.h>
#include <Preferences.h>

// External references
extern WiFiManager wifiManager;

// Web server and DNS server instances
AsyncWebServer* server = nullptr;
DNSServer* dnsServer = nullptr;

// Configuration storage
Preferences configPrefs;

WebServer::WebServer()
    : m_started(false)
{
}

void WebServer::begin() {
    DEBUG_PRINTLN("Web Server: Initializing...");

    // Initialize LittleFS for web files
    if (!LittleFS.begin(true)) {
        DEBUG_PRINTLN("Web Server: Failed to mount LittleFS");
        return;
    }

    DEBUG_PRINTLN("Web Server: LittleFS mounted");

    // Create web server instance
    server = new AsyncWebServer(WEB_SERVER_PORT);

    // Setup all routes
    setupRoutes();

    // Start DNS server for captive portal (only in AP mode)
    if (wifiManager.isAPMode()) {
        dnsServer = new DNSServer();
        dnsServer->start(53, "*", WiFi.softAPIP());
        DEBUG_PRINTLN("Web Server: DNS server started for captive portal");
    }

    // Start web server
    server->begin();
    m_started = true;

    DEBUG_PRINTLN("Web Server: Started on port 80");
}

void WebServer::loop() {
    // Process DNS requests (captive portal)
    if (dnsServer && wifiManager.isAPMode()) {
        dnsServer->processNextRequest();
    }
}

void WebServer::stop() {
    if (server) {
        server->end();
        delete server;
        server = nullptr;
    }

    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }

    m_started = false;
    DEBUG_PRINTLN("Web Server: Stopped");
}

void WebServer::setupRoutes() {
    // ============================================
    // Static Files (Web UI)
    // ============================================

    // Serve index.html for root
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });

    // Serve static files
    server->serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

    // Captive portal redirects
    server->on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });

    server->on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/");
    });

    // ============================================
    // API: Version
    // ============================================

    server->on("/api/version", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["version"] = VERSION;
        doc["build"] = __DATE__ " " __TIME__;

        #ifdef ESP32
        doc["platform"] = "ESP32";
        #elif defined(ESP32S3)
        doc["platform"] = "ESP32-S3";
        #elif defined(ESP8266)
        doc["platform"] = "ESP8266";
        #endif

        #ifdef RADIO_NRF24
        doc["radio_nrf24"] = true;
        #endif

        #ifdef RADIO_CMT2300A
        doc["radio_cmt2300a"] = true;
        #endif

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // ============================================
    // API: WiFi Scan
    // ============================================

    server->on("/api/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        String networks = wifiManager.scanNetworks();
        request->send(200, "application/json", networks);
    });

    // ============================================
    // API: WiFi Connect
    // ============================================

    server->on("/api/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (const char*)data);

            if (error) {
                request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            String ssid = doc["ssid"] | "";
            String password = doc["password"] | "";

            if (ssid.length() == 0) {
                request->send(400, "application/json", "{\"success\":false,\"error\":\"SSID required\"}");
                return;
            }

            DEBUG_PRINT("Web Server: Connecting to WiFi: ");
            DEBUG_PRINTLN(ssid);

            // Try to connect (this will save credentials if successful)
            bool connected = wifiManager.connect(ssid, password);

            JsonDocument response;
            response["success"] = connected;

            if (connected) {
                response["ip"] = wifiManager.getIPAddress();
            } else {
                response["error"] = "Connection failed";
            }

            String responseStr;
            serializeJson(response, responseStr);
            request->send(connected ? 200 : 400, "application/json", responseStr);
        }
    );

    // ============================================
    // API: WiFi Status
    // ============================================

    server->on("/api/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["connected"] = wifiManager.isConnected();
        doc["ap_mode"] = wifiManager.isAPMode();
        doc["ssid"] = wifiManager.getSSID();
        doc["ip"] = wifiManager.getIPAddress();
        doc["mac"] = wifiManager.getMacAddress();

        if (wifiManager.isConnected()) {
            doc["rssi"] = wifiManager.getRSSI();
        }

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // ============================================
    // API: System Status
    // ============================================

    server->on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;

        // System info
        doc["uptime"] = millis() / 1000;
        doc["free_heap"] = ESP.getFreeHeap();

        #ifdef ESP32
        doc["chip_model"] = ESP.getChipModel();
        doc["chip_revision"] = ESP.getChipRevision();
        doc["cpu_freq"] = ESP.getCpuFreqMHz();
        #endif

        // WiFi status
        doc["wifi_connected"] = wifiManager.isConnected();
        doc["wifi_ap_mode"] = wifiManager.isAPMode();

        // Configuration
        configPrefs.begin("config", true);
        doc["mode"] = configPrefs.getString("mode", "");
        configPrefs.end();

        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // ============================================
    // API: MQTT Configuration (Generic Mode)
    // ============================================

    server->on("/api/mqtt/configure", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (const char*)data);

            if (error) {
                request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            // Extract MQTT configuration
            String host = doc["host"] | "";
            int port = doc["port"] | 1883;
            bool ssl = doc["ssl"] | false;
            String username = doc["username"] | "";
            String password = doc["password"] | "";
            String topic = doc["topic"] | "opendtu";

            if (host.length() == 0) {
                request->send(400, "application/json", "{\"success\":false,\"error\":\"Host required\"}");
                return;
            }

            // Save configuration
            configPrefs.begin("config", false);
            configPrefs.putString("mode", "generic");
            configPrefs.putString("mqtt_host", host);
            configPrefs.putInt("mqtt_port", port);
            configPrefs.putBool("mqtt_ssl", ssl);
            configPrefs.putString("mqtt_user", username);
            configPrefs.putString("mqtt_pass", password);
            configPrefs.putString("mqtt_topic", topic);
            configPrefs.end();

            DEBUG_PRINTLN("Web Server: MQTT configuration saved");

            // TODO: Test MQTT connection here

            request->send(200, "application/json", "{\"success\":true}");

            // Reboot after 2 seconds
            delay(2000);
            ESP.restart();
        }
    );

    // ============================================
    // API: MyPVLog Login (Direct Mode)
    // ============================================

    server->on("/api/mypvlog/login", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, (const char*)data);

            if (error) {
                request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
                return;
            }

            String email = doc["email"] | "";
            String password = doc["password"] | "";

            // TODO: Implement actual login to mypvlog.net API
            // For now, just return a mock response

            DEBUG_PRINT("Web Server: MyPVLog login attempt: ");
            DEBUG_PRINTLN(email);

            // Mock response
            request->send(501, "application/json",
                "{\"success\":false,\"error\":\"MyPVLog Direct mode not yet implemented\"}");
        }
    );

    // ============================================
    // API: MyPVLog Provision (Direct Mode)
    // ============================================

    server->on("/api/mypvlog/provision", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            // TODO: Implement device provisioning with mypvlog.net

            DEBUG_PRINTLN("Web Server: MyPVLog provisioning requested");

            request->send(501, "application/json",
                "{\"success\":false,\"error\":\"MyPVLog Direct mode not yet implemented\"}");
        }
    );

    // ============================================
    // API: System Reset
    // ============================================

    server->on("/api/system/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        DEBUG_PRINTLN("Web Server: System reset requested");

        request->send(200, "application/json", "{\"success\":true,\"message\":\"Resetting...\"}");

        delay(1000);
        wifiManager.reset();
    });

    // ============================================
    // API: Factory Reset
    // ============================================

    server->on("/api/system/factory-reset", HTTP_POST, [](AsyncWebServerRequest *request) {
        DEBUG_PRINTLN("Web Server: Factory reset requested");

        // Clear all stored configuration
        configPrefs.begin("config", false);
        configPrefs.clear();
        configPrefs.end();

        wifiManager.clearCredentials();

        request->send(200, "application/json", "{\"success\":true,\"message\":\"Factory reset complete\"}");

        delay(1000);
        ESP.restart();
    });

    // ============================================
    // 404 Handler
    // ============================================

    server->onNotFound([](AsyncWebServerRequest *request) {
        // Redirect to index for captive portal
        if (wifiManager.isAPMode()) {
            request->redirect("/");
        } else {
            request->send(404, "text/plain", "Not Found");
        }
    });

    DEBUG_PRINTLN("Web Server: Routes configured");
}
