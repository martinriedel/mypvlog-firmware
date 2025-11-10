/**
 * OTA (Over-The-Air) Firmware Updater Implementation
 */

#include "ota_updater.h"
#include "config.h"
#include "ssl_certificates.h"

#ifdef ESP32
    #include <Update.h>
    #include <HTTPClient.h>
    #include <WiFiClientSecure.h>
#elif defined(ESP8266)
    #include <Updater.h>
    #include <ESP8266HTTPClient.h>
    #include <WiFiClientSecureBearSSL.h>
#endif

OTAUpdater::OTAUpdater()
    : m_status(OTAStatus::IDLE)
    , m_lastError("")
{
}

void OTAUpdater::setStatus(OTAStatus status, int progress, const String& message) {
    m_status = status;

    DEBUG_PRINT("OTA: ");
    DEBUG_PRINT(message);
    if (progress >= 0) {
        DEBUG_PRINT(" (");
        DEBUG_PRINT(progress);
        DEBUG_PRINT("%)");
    }
    DEBUG_PRINTLN();

    if (m_progressCallback) {
        m_progressCallback(status, progress, message);
    }
}

bool OTAUpdater::performUpdate(const String& downloadUrl,
                               const String& expectedChecksum,
                               OTAProgressCallback progressCallback) {
    m_progressCallback = progressCallback;
    m_lastError = "";

    DEBUG_PRINTLN("OTA: Starting firmware update...");
    DEBUG_PRINT("OTA: Download URL: ");
    DEBUG_PRINTLN(downloadUrl);
    DEBUG_PRINT("OTA: Expected checksum: ");
    DEBUG_PRINTLN(expectedChecksum);

    setStatus(OTAStatus::CHECKING, 0, "Checking update");

    // Create HTTP client with SSL
#ifdef ESP32
    WiFiClientSecure client;
    #if MYPVLOG_SSL_VERIFY
        client.setCACert(api_mypvlog_net_cert);
        DEBUG_PRINTLN("OTA: SSL certificate validation enabled");
    #else
        client.setInsecure();
        DEBUG_PRINTLN("OTA: WARNING - SSL validation disabled (insecure!)");
    #endif
#elif defined(ESP8266)
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    #if MYPVLOG_SSL_VERIFY
        client->setCACert(api_mypvlog_net_cert);
        DEBUG_PRINTLN("OTA: SSL certificate validation enabled");
    #else
        client->setInsecure();
        DEBUG_PRINTLN("OTA: WARNING - SSL validation disabled (insecure!)");
    #endif
#endif

    HTTPClient http;

    // Begin HTTP connection
#ifdef ESP32
    if (!http.begin(client, downloadUrl)) {
#elif defined(ESP8266)
    if (!http.begin(*client, downloadUrl)) {
#endif
        m_lastError = "Failed to connect to update server";
        setStatus(OTAStatus::FAILED, -1, m_lastError);
        return false;
    }

    http.addHeader("User-Agent", "mypvlog-firmware/" VERSION);

    // Get firmware size
    int httpCode = http.GET();

    if (httpCode != 200) {
        m_lastError = "HTTP error: " + String(httpCode);
        setStatus(OTAStatus::FAILED, -1, m_lastError);
        http.end();
        return false;
    }

    int contentLength = http.getSize();

    if (contentLength <= 0) {
        m_lastError = "Invalid content length";
        setStatus(OTAStatus::FAILED, -1, m_lastError);
        http.end();
        return false;
    }

    DEBUG_PRINT("OTA: Firmware size: ");
    DEBUG_PRINT(contentLength);
    DEBUG_PRINTLN(" bytes");

    // Check if there's enough space
#ifdef ESP32
    if (!Update.begin(contentLength)) {
#elif defined(ESP8266)
    if (!Update.begin(contentLength)) {
#endif
        m_lastError = "Not enough space for update";
        setStatus(OTAStatus::FAILED, -1, m_lastError);
        http.end();
        return false;
    }

    setStatus(OTAStatus::DOWNLOADING, 0, "Downloading firmware");

    // Get stream
    WiFiClient* stream = http.getStreamPtr();

    // Download and write firmware
    uint8_t buffer[128];
    int downloaded = 0;
    int lastProgress = 0;

    while (http.connected() && downloaded < contentLength) {
        // Get available data size
        size_t available = stream->available();

        if (available) {
            // Read up to buffer size
            int bytesRead = stream->readBytes(buffer, min(available, sizeof(buffer)));

            // Write to flash
            size_t written = Update.write(buffer, bytesRead);

            if (written != bytesRead) {
                m_lastError = "Write failed";
                setStatus(OTAStatus::FAILED, -1, m_lastError);
                Update.abort();
                http.end();
                return false;
            }

            downloaded += bytesRead;

            // Update progress
            int progress = (downloaded * 100) / contentLength;
            if (progress != lastProgress && progress % 5 == 0) {
                setStatus(OTAStatus::DOWNLOADING, progress, "Downloading firmware");
                lastProgress = progress;
            }
        }

        delay(1);
    }

    http.end();

    if (downloaded != contentLength) {
        m_lastError = "Download incomplete";
        setStatus(OTAStatus::FAILED, -1, m_lastError);
        Update.abort();
        return false;
    }

    setStatus(OTAStatus::DOWNLOADING, 100, "Download complete");
    setStatus(OTAStatus::INSTALLING, 0, "Installing firmware");

    // Verify checksum if provided
    if (!expectedChecksum.isEmpty()) {
        if (!verifyChecksum(expectedChecksum)) {
            m_lastError = "Checksum verification failed";
            setStatus(OTAStatus::FAILED, -1, m_lastError);
            Update.abort();
            return false;
        }
    }

    // Finalize update
    if (!Update.end(true)) {
        m_lastError = "Update failed: ";
#ifdef ESP32
        m_lastError += Update.errorString();
#elif defined(ESP8266)
        m_lastError += String(Update.getError());
#endif
        setStatus(OTAStatus::FAILED, -1, m_lastError);
        return false;
    }

    setStatus(OTAStatus::SUCCESS, 100, "Update successful - Rebooting...");

    DEBUG_PRINTLN("OTA: Update successful!");
    DEBUG_PRINTLN("OTA: Rebooting in 3 seconds...");

    // Give time for callbacks to complete
    delay(3000);

    // Reboot
    ESP.restart();

    return true;
}

bool OTAUpdater::verifyChecksum(const String& expected) {
    DEBUG_PRINT("OTA: Verifying checksum... ");

    // Get MD5 from Update library
#ifdef ESP32
    String actualChecksum = Update.md5String();
#elif defined(ESP8266)
    String actualChecksum = Update.md5String();
#endif

    DEBUG_PRINT("Expected: ");
    DEBUG_PRINT(expected);
    DEBUG_PRINT(", Actual: ");
    DEBUG_PRINTLN(actualChecksum);

    // Compare checksums (case-insensitive)
    if (expected.equalsIgnoreCase(actualChecksum)) {
        DEBUG_PRINTLN("OTA: Checksum verified");
        return true;
    }

    DEBUG_PRINTLN("OTA: Checksum mismatch!");
    return false;
}
