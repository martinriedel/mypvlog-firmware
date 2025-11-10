# Security Policy

## Security Status

This document outlines security considerations for the mypvlog firmware.

---

## ✅ Secure Design Principles

### 1. No Hardcoded Secrets
- ✅ **No API keys** in source code
- ✅ **No passwords or tokens** embedded in firmware
- ✅ **User authentication** happens via web login (not in firmware)
- ✅ **Device credentials** generated server-side during provisioning

### 2. Proper Authentication Flow
```
User Flow:
1. User logs in to mypvlog.net (gets JWT token)
2. User enters JWT in device web UI
3. Device calls /api/firmware/provision with JWT
4. Backend validates JWT, creates DTU, returns MQTT credentials
5. Device stores MQTT credentials encrypted in NVS
6. Device uses MQTT credentials for future authentication
```

**Why this is secure:**
- JWT tokens are short-lived (1 hour)
- MQTT credentials are unique per device
- No shared secrets across devices
- Backend controls access via subscription tiers

### 3. Encrypted Storage
- Device credentials stored in ESP32/ESP8266 **NVS (Non-Volatile Storage)**
- NVS is encrypted by default on ESP32 with flash encryption
- WiFi passwords never transmitted to backend

---

## ⚠️ Known Security Limitations

### 1. SSL Certificate Validation Disabled (CRITICAL)

**Location:** `src/mypvlog_api.cpp`, `src/ota_updater.cpp`

**Current code:**
```cpp
client.setInsecure(); // TODO: Add certificate validation
```

**Risk:** Man-in-the-middle (MITM) attacks possible
- Attacker on same network could intercept API calls
- Could steal MQTT credentials during provisioning
- Could serve malicious firmware during OTA updates

**Mitigation (To Be Implemented):**

**Option A: Certificate Pinning (Recommended)**
```cpp
// Pin specific certificate for api.mypvlog.net
const char* api_mypvlog_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n" \
"...\n" \
"-----END CERTIFICATE-----\n";

WiFiClientSecure client;
client.setCACert(api_mypvlog_cert);
```

**Option B: Root CA Bundle**
```cpp
// Include standard root CA certificates
#include <WiFiClientSecure.h>
client.setCACert(root_ca_bundle);
```

**Status:** ⏳ Planned for v1.1.0
**Workaround:** Users should only flash from trusted networks

### 2. OTA Update Integrity

**Current:**
- ✅ MD5 checksum validation before flashing
- ✅ Backend must sign releases with checksum
- ⚠️ No cryptographic signature verification

**Risk:** If backend compromised, malicious firmware could be served

**Mitigation (To Be Implemented):**
- Add RSA signature verification
- Firmware releases signed with private key
- Device validates with public key (embedded in firmware)

**Status:** ⏳ Planned for v1.2.0

### 3. Device Authentication in Heartbeat

**Current:**
```cpp
// Heartbeat uses DTU ID + MQTT password for auth
POST /api/firmware/heartbeat
{
  "dtuId": "...",
  "mqttPassword": "..."
}
```

**Why this is acceptable:**
- MQTT password is unique per device
- Only valid for that specific DTU ID
- HTTPS encrypts transmission (once SSL cert validation added)
- Backend validates DTU ID exists and password matches

**Better approach (future):**
- Use HMAC-signed requests
- Rotate device secrets periodically

---

## 🔐 Security Best Practices for Users

### 1. Provisioning (mypvlog Direct Mode)
- ✅ **DO** provision on trusted WiFi network
- ✅ **DO** use HTTPS for mypvlog.net login
- ❌ **DON'T** provision on public WiFi without VPN
- ❌ **DON'T** share JWT tokens

### 2. Generic MQTT Mode
- ✅ **DO** use MQTT over TLS (port 8883)
- ✅ **DO** use strong MQTT passwords
- ✅ **DO** enable MQTT ACLs to restrict topic access
- ❌ **DON'T** use MQTT without authentication
- ❌ **DON'T** expose MQTT broker to internet without firewall

### 3. Device Physical Security
- ⚠️ **Physical access = full compromise**
- Anyone with USB access can read NVS storage
- Consider enabling ESP32 flash encryption for sensitive deployments

---

## 🛡️ Backend Security Requirements

### 1. API Rate Limiting
**Required on backend:**
```csharp
// Prevent brute-force attacks
[RateLimit(requests: 5, period: "1m")]
public async Task<IActionResult> ProvisionDevice(...)

[RateLimit(requests: 60, period: "1h")]
public async Task<IActionResult> Heartbeat(...)
```

### 2. JWT Validation
**Backend must validate:**
- ✅ Token signature
- ✅ Token expiration
- ✅ User subscription tier limits
- ✅ Email verification status

### 3. MQTT Broker Security
**mqtt.mypvlog.net must have:**
- ✅ TLS/SSL enabled (port 8883)
- ✅ Username/password authentication
- ✅ ACL rules per device (topic restrictions)
- ✅ Connection rate limiting

Example ACL for device:
```
# Device can only publish to its own topics
user deviceuser
topic write opendtu/{clientid}/#
```

### 4. Firmware Release Security
**Release process must:**
1. Build firmware in CI/CD
2. Generate MD5 checksum
3. (Future) Sign with RSA private key
4. Upload to secure CDN
5. Create FirmwareRelease record in database

---

## 🚨 Reporting Security Vulnerabilities

**DO NOT** open public GitHub issues for security vulnerabilities.

Instead:
- Email: security@mypvlog.net
- Include:
  - Description of vulnerability
  - Steps to reproduce
  - Impact assessment
  - Suggested fix (optional)

**Response time:** We aim to respond within 48 hours.

**Disclosure policy:**
- We follow coordinated disclosure
- We'll work with you on timeline
- Credit will be given in release notes

---

## 📋 Security Checklist for Deployment

### Before Public Launch:

**Firmware:**
- [ ] Enable SSL certificate validation (`setInsecure()` → `setCACert()`)
- [ ] Add certificate pinning for api.mypvlog.net
- [ ] Implement OTA signature verification
- [ ] Enable ESP32 secure boot (optional, for production hardware)
- [ ] Enable ESP32 flash encryption (optional, for sensitive deployments)

**Backend:**
- [ ] Rate limiting on all API endpoints
- [ ] JWT token validation working
- [ ] MQTT broker ACLs configured
- [ ] InfluxDB access restricted
- [ ] Database credentials not in source code
- [ ] SSL certificates installed and auto-renewing

**Infrastructure:**
- [ ] WAF (Web Application Firewall) enabled
- [ ] DDoS protection active
- [ ] Database backups encrypted
- [ ] Secrets management system (AWS Secrets Manager, etc.)
- [ ] Monitoring and alerting configured

---

## 🔄 Security Update Policy

### Automatic Updates (mypvlog Direct Mode)
- Security updates pushed automatically via OTA
- Critical vulnerabilities patched within 48 hours
- Users notified via email of security updates

### Manual Updates (Generic MQTT Mode)
- Security advisories posted to GitHub
- Users must manually update firmware
- Recommendation: Enable GitHub watch for security notifications

---

## 📚 References

- [OWASP IoT Security Guide](https://owasp.org/www-project-internet-of-things/)
- [ESP32 Security Features](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/security/index.html)
- [MQTT Security](https://mqtt.org/mqtt-specification/mqtt-security-fundamentals/)

---

## Version History

- **v1.0.0** (2025-11-10): Initial security policy
  - SSL validation disabled (known limitation)
  - MD5 checksum validation for OTA
  - No hardcoded secrets

**Next security improvements planned for v1.1.0:**
- SSL certificate validation
- Certificate pinning
- Enhanced device authentication
