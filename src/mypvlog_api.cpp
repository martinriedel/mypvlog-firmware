/**
 * mypvlog.net API Client Implementation
 */

#include "mypvlog_api.h"
#include "config.h"
#include <ArduinoJson.h>

#ifdef ESP32
    #include <HTTPClient.h>
    #include <WiFiClientSecure.h>
#elif defined(ESP8266)
    #include <ESP8266HTTPClient.h>
    #include <WiFiClientSecureBearSSL.h>
#endif

MypvlogAPI::MypvlogAPI()
    : m_apiUrl("")
    , m_authToken("")
{
}

void MypvlogAPI::begin(const String& apiUrl) {
    m_apiUrl = apiUrl;
    DEBUG_PRINT("mypvlog API: Initialized with URL: ");
    DEBUG_PRINTLN(m_apiUrl);
}

void MypvlogAPI::setAuthToken(const String& token) {
    m_authToken = token;
    DEBUG_PRINTLN("mypvlog API: Auth token set");
}

void MypvlogAPI::setErrorCallback(std::function<void(const String& error)> callback) {
    m_errorCallback = callback;
}

ProvisionResponse MypvlogAPI::provision(const String& deviceMac,
                                        const String& firmwareVersion,
                                        const String& hardwareModel) {
    DEBUG_PRINTLN("mypvlog API: Provisioning device...");

    ProvisionResponse response;
    response.success = false;

    // Check auth token
    if (m_authToken.isEmpty()) {
        response.error = "No authentication token set";
        DEBUG_PRINTLN("mypvlog API: ERROR - No auth token");
        if (m_errorCallback) {
            m_errorCallback(response.error);
        }
        return response;
    }

    // Build JSON request body
    JsonDocument doc;
    doc["deviceMac"] = deviceMac;
    doc["firmwareVersion"] = firmwareVersion;
    doc["hardwareModel"] = hardwareModel;

    String requestBody;
    serializeJson(doc, requestBody);

    DEBUG_PRINT("mypvlog API: Request body: ");
    DEBUG_PRINTLN(requestBody);

    // Make API request
    String responseBody = makePostRequest("/api/firmware/provision", requestBody, true);

    if (responseBody.isEmpty()) {
        response.error = "Empty response from server";
        DEBUG_PRINTLN("mypvlog API: ERROR - Empty response");
        if (m_errorCallback) {
            m_errorCallback(response.error);
        }
        return response;
    }

    DEBUG_PRINT("mypvlog API: Response: ");
    DEBUG_PRINTLN(responseBody);

    // Parse JSON response
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, responseBody);

    if (error) {
        response.error = "Failed to parse response: ";
        response.error += error.c_str();
        DEBUG_PRINT("mypvlog API: ERROR - ");
        DEBUG_PRINTLN(response.error);
        if (m_errorCallback) {
            m_errorCallback(response.error);
        }
        return response;
    }

    // Extract response data
    if (responseDoc.containsKey("dtuId")) {
        response.success = true;
        response.dtuId = responseDoc["dtuId"].as<String>();
        response.mqttUsername = responseDoc["mqttUsername"].as<String>();
        response.mqttPassword = responseDoc["mqttPassword"].as<String>();
        response.mqttBroker = responseDoc["mqttBroker"].as<String>();
        response.mqttPort = responseDoc["mqttPort"].as<int>();
        response.mqttUseSsl = responseDoc["mqttUseSsl"].as<bool>();
        response.mqttTopicPrefix = responseDoc["mqttTopicPrefix"].as<String>();

        DEBUG_PRINTLN("mypvlog API: Provisioning successful!");
        DEBUG_PRINT("  DTU ID: ");
        DEBUG_PRINTLN(response.dtuId);
        DEBUG_PRINT("  MQTT Broker: ");
        DEBUG_PRINTLN(response.mqttBroker);
        DEBUG_PRINT("  MQTT Port: ");
        DEBUG_PRINTLN(response.mqttPort);
    } else if (responseDoc.containsKey("error")) {
        response.error = responseDoc["error"].as<String>();
        DEBUG_PRINT("mypvlog API: Provisioning failed: ");
        DEBUG_PRINTLN(response.error);
        if (m_errorCallback) {
            m_errorCallback(response.error);
        }
    } else {
        response.error = "Invalid response format";
        DEBUG_PRINTLN("mypvlog API: ERROR - Invalid response format");
        if (m_errorCallback) {
            m_errorCallback(response.error);
        }
    }

    return response;
}

HeartbeatResponse MypvlogAPI::sendHeartbeat(const String& dtuId,
                                           const String& mqttPassword,
                                           unsigned long uptime,
                                           uint32_t freeHeap,
                                           int rssi,
                                           const String& ipAddress) {
    DEBUG_PRINTLN("mypvlog API: Sending heartbeat...");

    HeartbeatResponse response;
    response.success = false;
    response.configChanged = false;

    // Build JSON request body
    JsonDocument doc;
    doc["dtuId"] = dtuId;
    doc["mqttPassword"] = mqttPassword;
    doc["uptime"] = uptime;
    doc["freeHeap"] = freeHeap;
    doc["rssiDbm"] = rssi;
    doc["ipAddress"] = ipAddress;

    String requestBody;
    serializeJson(doc, requestBody);

    DEBUG_PRINT("mypvlog API: Heartbeat data: uptime=");
    DEBUG_PRINT(uptime);
    DEBUG_PRINT("s, heap=");
    DEBUG_PRINT(freeHeap);
    DEBUG_PRINT(", rssi=");
    DEBUG_PRINT(rssi);
    DEBUG_PRINTLN("dBm");

    // Make API request (no authentication required - uses dtuId + mqttPassword)
    String responseBody = makePostRequest("/api/firmware/heartbeat", requestBody, false);

    if (responseBody.isEmpty()) {
        response.error = "Empty response from server";
        DEBUG_PRINTLN("mypvlog API: WARNING - Heartbeat empty response");
        // Don't call error callback for heartbeat failures - they're not critical
        return response;
    }

    // Parse JSON response
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, responseBody);

    if (error) {
        response.error = "Failed to parse response: ";
        response.error += error.c_str();
        DEBUG_PRINT("mypvlog API: WARNING - Heartbeat parse error: ");
        DEBUG_PRINTLN(response.error);
        return response;
    }

    // Extract response data
    if (responseDoc.containsKey("success") && responseDoc["success"].as<bool>()) {
        response.success = true;
        if (responseDoc.containsKey("configChanged")) {
            response.configChanged = responseDoc["configChanged"].as<bool>();
        }
        DEBUG_PRINTLN("mypvlog API: Heartbeat successful");
        if (response.configChanged) {
            DEBUG_PRINTLN("  Config changed - should re-provision");
        }
    } else if (responseDoc.containsKey("error")) {
        response.error = responseDoc["error"].as<String>();
        DEBUG_PRINT("mypvlog API: Heartbeat error: ");
        DEBUG_PRINTLN(response.error);
    }

    return response;
}

FirmwareUpdateInfo MypvlogAPI::checkFirmwareUpdate(const String& currentVersion,
                                                   const String& hardwareModel) {
    DEBUG_PRINTLN("mypvlog API: Checking for firmware updates...");

    FirmwareUpdateInfo info;
    info.updateAvailable = false;

    // Build query parameters
    String queryParams = "currentVersion=" + urlEncode(currentVersion);
    queryParams += "&hardwareModel=" + urlEncode(hardwareModel);

    // Make API request
    String responseBody = makeGetRequest("/api/firmware/update", queryParams);

    if (responseBody.isEmpty()) {
        DEBUG_PRINTLN("mypvlog API: No update available (empty response)");
        return info;
    }

    DEBUG_PRINT("mypvlog API: Update check response: ");
    DEBUG_PRINTLN(responseBody);

    // Parse JSON response
    JsonDocument responseDoc;
    DeserializationError error = deserializeJson(responseDoc, responseBody);

    if (error) {
        DEBUG_PRINT("mypvlog API: WARNING - Failed to parse update response: ");
        DEBUG_PRINTLN(error.c_str());
        return info;
    }

    // Extract update information
    if (responseDoc.containsKey("updateAvailable") && responseDoc["updateAvailable"].as<bool>()) {
        info.updateAvailable = true;
        info.version = responseDoc["version"].as<String>();
        info.downloadUrl = responseDoc["downloadUrl"].as<String>();
        info.releaseNotes = responseDoc["releaseNotes"].as<String>();
        info.fileSizeBytes = responseDoc["fileSizeBytes"].as<long>();
        info.checksum = responseDoc["checksum"].as<String>();

        DEBUG_PRINTLN("mypvlog API: Update available!");
        DEBUG_PRINT("  Version: ");
        DEBUG_PRINTLN(info.version);
        DEBUG_PRINT("  Size: ");
        DEBUG_PRINT(info.fileSizeBytes);
        DEBUG_PRINTLN(" bytes");
    } else {
        DEBUG_PRINTLN("mypvlog API: No update available");
    }

    return info;
}

// Helper methods

String MypvlogAPI::makeGetRequest(const String& endpoint, const String& queryParams) {
    String url = m_apiUrl + endpoint;
    if (!queryParams.isEmpty()) {
        url += "?" + queryParams;
    }

    DEBUG_PRINT("mypvlog API: GET ");
    DEBUG_PRINTLN(url);

#ifdef ESP32
    WiFiClientSecure client;
    client.setInsecure(); // TODO: Add certificate validation
#elif defined(ESP8266)
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure(); // TODO: Add certificate validation
#endif

    HTTPClient http;

#ifdef ESP32
    if (!http.begin(client, url)) {
#elif defined(ESP8266)
    if (!http.begin(*client, url)) {
#endif
        DEBUG_PRINTLN("mypvlog API: ERROR - Failed to begin HTTP connection");
        return "";
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "mypvlog-firmware/" VERSION);

    int httpCode = http.GET();

    if (httpCode > 0) {
        DEBUG_PRINT("mypvlog API: HTTP ");
        DEBUG_PRINTLN(httpCode);

        if (httpCode == 200) {
            String response = http.getString();
            http.end();
            return response;
        } else if (httpCode == 204) {
            // No content - no update available
            DEBUG_PRINTLN("mypvlog API: No content (204)");
            http.end();
            return "";
        } else {
            DEBUG_PRINT("mypvlog API: ERROR - HTTP ");
            DEBUG_PRINTLN(httpCode);
            String errorResponse = http.getString();
            DEBUG_PRINTLN(errorResponse);
        }
    } else {
        DEBUG_PRINT("mypvlog API: ERROR - HTTP request failed: ");
        DEBUG_PRINTLN(http.errorToString(httpCode));
    }

    http.end();
    return "";
}

String MypvlogAPI::makePostRequest(const String& endpoint, const String& body, bool authenticated) {
    String url = m_apiUrl + endpoint;

    DEBUG_PRINT("mypvlog API: POST ");
    DEBUG_PRINTLN(url);

#ifdef ESP32
    WiFiClientSecure client;
    client.setInsecure(); // TODO: Add certificate validation
#elif defined(ESP8266)
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure(); // TODO: Add certificate validation
#endif

    HTTPClient http;

#ifdef ESP32
    if (!http.begin(client, url)) {
#elif defined(ESP8266)
    if (!http.begin(*client, url)) {
#endif
        DEBUG_PRINTLN("mypvlog API: ERROR - Failed to begin HTTP connection");
        return "";
    }

    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "mypvlog-firmware/" VERSION);

    // Add authentication header if needed
    if (authenticated && !m_authToken.isEmpty()) {
        http.addHeader("Authorization", "Bearer " + m_authToken);
        DEBUG_PRINTLN("mypvlog API: Added auth header");
    }

    int httpCode = http.POST(body);

    if (httpCode > 0) {
        DEBUG_PRINT("mypvlog API: HTTP ");
        DEBUG_PRINTLN(httpCode);

        if (httpCode == 200) {
            String response = http.getString();
            http.end();
            return response;
        } else if (httpCode == 400 || httpCode == 401 || httpCode == 403) {
            String errorResponse = http.getString();
            DEBUG_PRINT("mypvlog API: ERROR - ");
            DEBUG_PRINTLN(errorResponse);
            http.end();
            return errorResponse; // Return error response to parse error message
        } else {
            DEBUG_PRINT("mypvlog API: ERROR - HTTP ");
            DEBUG_PRINTLN(httpCode);
        }
    } else {
        DEBUG_PRINT("mypvlog API: ERROR - HTTP request failed: ");
        DEBUG_PRINTLN(http.errorToString(httpCode));
    }

    http.end();
    return "";
}

String MypvlogAPI::urlEncode(const String& str) {
    String encoded = "";
    char c;
    char code0;
    char code1;

    for (unsigned int i = 0; i < str.length(); i++) {
        c = str.charAt(i);
        if (c == ' ') {
            encoded += '+';
        } else if (isalnum(c)) {
            encoded += c;
        } else {
            code1 = (c & 0xf) + '0';
            if ((c & 0xf) > 9) {
                code1 = (c & 0xf) - 10 + 'A';
            }
            c = (c >> 4) & 0xf;
            code0 = c + '0';
            if (c > 9) {
                code0 = c - 10 + 'A';
            }
            encoded += '%';
            encoded += code0;
            encoded += code1;
        }
    }
    return encoded;
}
