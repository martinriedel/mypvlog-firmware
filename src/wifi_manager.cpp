/**
 * WiFi Manager - Handle WiFi connection and AP mode
 *
 * Features:
 * - Auto-connect to saved WiFi credentials
 * - Fallback to AP mode if connection fails
 * - Captive portal for easy setup
 * - Automatic reconnection
 * - WiFi network scanning
 */

#include "wifi_manager.h"
#include "config.h"
#include <Preferences.h>

#ifdef ESP32
    #include <WiFi.h>
    #include <esp_wifi.h>
#else
    #include <ESP8266WiFi.h>
#endif

// Global preferences object for persistent storage
Preferences preferences;

WiFiManager::WiFiManager()
    : m_connected(false)
    , m_lastReconnectAttempt(0)
    , m_reconnectAttempts(0)
    , m_apMode(false)
{
}

void WiFiManager::begin() {
    DEBUG_PRINTLN("WiFi Manager: Initializing...");

    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    #ifdef ESP32
    // Set hostname
    WiFi.setHostname("mypvlog-dtu");
    #else
    WiFi.hostname("mypvlog-dtu");
    #endif

    // Try to load saved credentials
    loadCredentials();

    if (m_savedSSID.length() > 0) {
        DEBUG_PRINT("WiFi Manager: Attempting to connect to saved network: ");
        DEBUG_PRINTLN(m_savedSSID);

        if (connect(m_savedSSID, m_savedPassword)) {
            DEBUG_PRINTLN("WiFi Manager: Connected to saved network");
            m_connected = true;
        } else {
            DEBUG_PRINTLN("WiFi Manager: Failed to connect, starting AP mode");
            startAP();
        }
    } else {
        DEBUG_PRINTLN("WiFi Manager: No saved credentials, starting AP mode");
        startAP();
    }
}

void WiFiManager::loop() {
    if (m_apMode) {
        // In AP mode, nothing to do (DNS server handles captive portal)
        return;
    }

    // Check connection status
    bool currentlyConnected = (WiFi.status() == WL_CONNECTED);

    if (currentlyConnected != m_connected) {
        m_connected = currentlyConnected;

        if (m_connected) {
            DEBUG_PRINTLN("WiFi Manager: Connected!");
            DEBUG_PRINT("WiFi Manager: IP Address: ");
            DEBUG_PRINTLN(WiFi.localIP());
            m_reconnectAttempts = 0;
        } else {
            DEBUG_PRINTLN("WiFi Manager: Disconnected!");
        }
    }

    // Handle reconnection
    if (!m_connected) {
        unsigned long now = millis();

        // Try to reconnect every 5 seconds
        if (now - m_lastReconnectAttempt > 5000) {
            m_lastReconnectAttempt = now;
            reconnect();
        }
    }
}

bool WiFiManager::isConnected() {
    return m_connected && (WiFi.status() == WL_CONNECTED);
}

String WiFiManager::getSSID() {
    return WiFi.SSID();
}

int8_t WiFiManager::getRSSI() {
    return WiFi.RSSI();
}

String WiFiManager::getIPAddress() {
    if (m_apMode) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

String WiFiManager::getMacAddress() {
    return WiFi.macAddress();
}

bool WiFiManager::isAPMode() {
    return m_apMode;
}

bool WiFiManager::connect(const String& ssid, const String& password) {
    DEBUG_PRINT("WiFi Manager: Connecting to: ");
    DEBUG_PRINTLN(ssid);

    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait up to 20 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        DEBUG_PRINT(".");
        attempts++;
    }
    DEBUG_PRINTLN();

    if (WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi Manager: Connection successful!");
        DEBUG_PRINT("WiFi Manager: IP Address: ");
        DEBUG_PRINTLN(WiFi.localIP());

        // Save credentials
        saveCredentials(ssid, password);
        m_connected = true;

        // If we were in AP mode, stop it
        if (m_apMode) {
            stopAP();
        }

        return true;
    } else {
        DEBUG_PRINTLN("WiFi Manager: Connection failed!");
        return false;
    }
}

void WiFiManager::startAP() {
    DEBUG_PRINTLN("WiFi Manager: Starting Access Point...");

    // Generate AP SSID with last 4 chars of MAC address
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String apSSID = String(WIFI_AP_SSID_PREFIX) + mac.substring(mac.length() - 4);

    // Stop any existing connections
    WiFi.disconnect();

    // Set WiFi mode to AP
    WiFi.mode(WIFI_AP);

    // Start AP
    bool success = WiFi.softAP(
        apSSID.c_str(),
        WIFI_AP_PASSWORD,
        WIFI_AP_CHANNEL,
        false,  // SSID hidden
        WIFI_AP_MAX_CONNECTIONS
    );

    if (success) {
        m_apMode = true;
        IPAddress apIP = WiFi.softAPIP();

        DEBUG_PRINT("WiFi Manager: AP started successfully!");
        DEBUG_PRINTLN();
        DEBUG_PRINT("WiFi Manager: SSID: ");
        DEBUG_PRINTLN(apSSID);
        DEBUG_PRINT("WiFi Manager: Password: ");
        DEBUG_PRINTLN(WIFI_AP_PASSWORD);
        DEBUG_PRINT("WiFi Manager: IP Address: ");
        DEBUG_PRINTLN(apIP);
        DEBUG_PRINTLN("WiFi Manager: Connect and open http://192.168.4.1");
    } else {
        DEBUG_PRINTLN("WiFi Manager: Failed to start AP!");
    }
}

void WiFiManager::stopAP() {
    if (m_apMode) {
        DEBUG_PRINTLN("WiFi Manager: Stopping Access Point...");
        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_STA);
        m_apMode = false;
    }
}

void WiFiManager::reconnect() {
    if (m_reconnectAttempts >= 10) {
        DEBUG_PRINTLN("WiFi Manager: Too many reconnect attempts, starting AP mode");
        startAP();
        return;
    }

    m_reconnectAttempts++;
    DEBUG_PRINT("WiFi Manager: Reconnecting (attempt ");
    DEBUG_PRINT(m_reconnectAttempts);
    DEBUG_PRINTLN("/10)...");

    WiFi.reconnect();
}

void WiFiManager::loadCredentials() {
    preferences.begin("wifi", true); // Read-only
    m_savedSSID = preferences.getString("ssid", "");
    m_savedPassword = preferences.getString("password", "");
    preferences.end();

    if (m_savedSSID.length() > 0) {
        DEBUG_PRINT("WiFi Manager: Loaded credentials for: ");
        DEBUG_PRINTLN(m_savedSSID);
    }
}

void WiFiManager::saveCredentials(const String& ssid, const String& password) {
    preferences.begin("wifi", false); // Read-write
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    m_savedSSID = ssid;
    m_savedPassword = password;

    DEBUG_PRINTLN("WiFi Manager: Credentials saved");
}

void WiFiManager::clearCredentials() {
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();

    m_savedSSID = "";
    m_savedPassword = "";

    DEBUG_PRINTLN("WiFi Manager: Credentials cleared");
}

String WiFiManager::scanNetworks() {
    DEBUG_PRINTLN("WiFi Manager: Scanning networks...");

    int n = WiFi.scanNetworks();
    String json = "[";

    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";

        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"encryption\":" + String(WiFi.encryptionType(i));
        json += "}";
    }

    json += "]";

    DEBUG_PRINT("WiFi Manager: Found ");
    DEBUG_PRINT(n);
    DEBUG_PRINTLN(" networks");

    return json;
}

void WiFiManager::reset() {
    DEBUG_PRINTLN("WiFi Manager: Resetting...");

    clearCredentials();
    WiFi.disconnect(true);

    delay(1000);

    #ifdef ESP32
    ESP.restart();
    #else
    ESP.reset();
    #endif
}
