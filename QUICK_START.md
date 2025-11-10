# Quick Start Guide - mypvlog Firmware

**Get your Hoymiles inverter online in 10 minutes!**

---

## What You Need

### Hardware
- ✅ ESP32 development board (recommended) or ESP8266
- ✅ NRF24L01+ radio module (for HM-series inverters)
  - Standard version: ~100m range
  - PA+LNA version: ~1km range (recommended)
- ✅ Hoymiles inverter (HM-300 to HM-1500)
- ✅ USB cable for programming
- ✅ Jumper wires for connections

### Optional for HMS/HMT Series
- CMT2300A radio module (868MHz) for HMS-800 to HMS-2000, HMT inverters

---

## Step 1: Flash Firmware (2 minutes)

### Option A: Web Flasher (Easiest)

1. Go to **[flash.mypvlog.net](https://flash.mypvlog.net)**
2. Connect your ESP32/ESP8266 via USB
3. Click "Connect" and select your device
4. Choose firmware variant:
   - **esp32-nrf24** - Most common (HM-series only)
   - **esp32-dual** - Both HM and HMS/HMT support
   - **esp8266-nrf24** - Budget option (limited features)
5. Click "Install" and wait ~1 minute
6. Done!

**Requirements:** Chrome, Edge, or Opera browser (WebUSB support)

### Option B: Manual Flash (Advanced)

Download firmware from [releases](https://github.com/martinriedel/mypvlog-firmware/releases/latest):

```bash
# Install esptool
pip install esptool

# Flash ESP32
esptool.py --port /dev/ttyUSB0 write_flash 0x0 esp32-nrf24.bin

# Flash ESP8266
esptool.py --port /dev/ttyUSB0 write_flash 0x0 esp8266-nrf24.bin
```

---

## Step 2: Wire Radio Module (5 minutes)

### ESP32 + NRF24L01+ Wiring

```
ESP32 Pin    →    NRF24L01+
──────────────────────────────
GPIO 5  (CS)     →    CSN
GPIO 18 (SCK)    →    SCK
GPIO 19 (MISO)   →    MISO
GPIO 23 (MOSI)   →    MOSI
GPIO 2           →    CE
3.3V             →    VCC
GND              →    GND
```

**Important Notes:**
- ⚠️ Use **3.3V**, NOT 5V (will damage NRF24)
- ⚠️ Keep wires **short** (< 10cm) for stable connection
- ⚠️ Add 10µF capacitor between VCC and GND if experiencing instability
- ✅ External antenna points toward inverter

### Visual Check
```
ESP32                     NRF24L01+
┌─────────┐              ┌─────────┐
│         │              │  ┌───┐  │
│     GND ├──────────────┤ GND    │
│    3.3V ├──────────────┤ VCC    │
│   GPIO5 ├──────────────┤ CSN    │
│  GPIO18 ├──────────────┤ SCK    │
│  GPIO19 ├──────────────┤ MISO   │
│  GPIO23 ├──────────────┤ MOSI   │
│   GPIO2 ├──────────────┤ CE     │
│         │              │  └───┘  │
└─────────┘              └─────────┘
```

---

## Step 3: Connect to WiFi (2 minutes)

1. **Power on ESP32** (plug USB cable)
2. **Find WiFi network**: `mypvlog-XXXX` (XXXX = last 4 chars of MAC address)
3. **Connect** with password: `mypvlog123`
4. **Browser opens automatically** (or go to http://192.168.4.1)
5. **Enter your WiFi credentials** and click "Save"
6. **Device reboots** and connects to your WiFi

✅ **Status LED:**
- Blinking fast: Setup mode (AP active)
- Blinking slow: Connecting to WiFi
- Solid on: Connected

---

## Step 4: Choose Your Mode (1 minute)

### Option A: mypvlog Direct Mode (Recommended)

**Perfect if you want:**
- ✅ Zero configuration
- ✅ Cloud dashboard at mypvlog.net
- ✅ Mobile app access
- ✅ Automatic updates
- ✅ Historical data & analytics

**Setup:**
1. Go to web interface: `http://<device-ip>` (shown on serial console)
2. Click "mypvlog Direct Mode"
3. Sign in with Google or email
4. Click "Provision Device"
5. **Done!** View dashboard at [mypvlog.net](https://mypvlog.net)

**Subscription (optional):**
- Free: 1 DTU, 7 days history
- Basic (€1/mo): Unlimited history, email alerts
- Plus (€3.99/mo): 2 DTUs, analytics, power limiting
- Pro (€7.99/mo): Unlimited DTUs, API access

### Option B: Generic MQTT Mode

**Perfect if you want:**
- ✅ Self-hosted setup
- ✅ Home Assistant integration
- ✅ Full control over data
- ✅ No cloud dependency

**Setup:**
1. Go to web interface: `http://<device-ip>`
2. Click "Generic MQTT Mode"
3. Enter MQTT broker details:
   - Host: `mqtt.example.com`
   - Port: `1883` (or `8883` for SSL)
   - Username & Password
   - Topic prefix: `opendtu` (OpenDTU compatible)
4. Click "Save"
5. **Done!** Data published to your MQTT broker

**Topic Structure:**
```
opendtu/<mac-address>/<inverter-serial>/data
{
  "power": 245.3,
  "voltage": 230.1,
  "current": 1.07
}
```

---

## Step 5: Add Inverter (1 minute)

1. In web interface, go to "Inverters"
2. Click "Add Inverter"
3. Enter **Inverter Serial Number** (found on inverter label)
   - Example: `112182123456`
   - HM-series: 12 digits
   - HMS-series: 12 digits
4. Optional: Give it a friendly name ("Roof Panel", "Balcony", etc.)
5. Click "Save"
6. **Wait 5-30 seconds** for first data

✅ **Status Check:**
- Serial console shows "Inverter XXXXX: Power=XXXw"
- Web interface updates with power reading
- MQTT topic receives data (Generic mode)
- mypvlog.net dashboard updates (Direct mode)

---

## Troubleshooting

### No Data from Inverter

**Check wiring:**
```bash
# Connect to serial console (115200 baud)
# Look for errors like:
# "NRF24: Init failed"  →  Check wiring, especially VCC/GND
# "NRF24: No response"  →  Move closer to inverter, check antenna
```

**Solutions:**
- ✅ Verify 3.3V connection (NOT 5V!)
- ✅ Shorten wires between ESP32 and NRF24
- ✅ Add capacitor (10µF-100µF) between VCC and GND
- ✅ Check inverter is online (sunny weather, AC connected)
- ✅ Move ESP32 closer to inverter
- ✅ Use PA+LNA module for better range

### WiFi Connection Fails

- ✅ Check WiFi password (case-sensitive)
- ✅ Ensure 2.4GHz network (ESP doesn't support 5GHz)
- ✅ Move ESP closer to WiFi router
- ✅ Check router doesn't block new devices

### Device Not in AP Mode

1. Hold BOOT button for 10 seconds
2. Release - device resets to AP mode
3. LED blinks fast = AP mode active

### Web Interface Not Loading

- ✅ Find IP address on serial console: `IP Address: 192.168.1.x`
- ✅ Try: http://192.168.1.x (replace x)
- ✅ Check device and computer on same network

### MQTT Not Receiving Data

```bash
# Test MQTT connection:
mosquitto_sub -h mqtt.example.com -p 1883 -u user -P pass -t "opendtu/#" -v

# Should see:
# opendtu/AA:BB:CC:DD:EE:FF/112182123456/data {"power":245.3,"voltage":230.1,"current":1.07}
```

- ✅ Verify broker address and credentials
- ✅ Check firewall allows port 1883/8883
- ✅ Test broker with mosquitto_pub first

---

## Advanced Configuration

### Serial Console Access

```bash
# Linux/Mac
screen /dev/ttyUSB0 115200

# Windows
putty.exe -serial COM3 -seriespeed 115200

# PlatformIO
pio device monitor -b 115200
```

**Useful commands:**
- `wifi status` - Show WiFi info
- `inverter list` - Show registered inverters
- `mqtt status` - Show MQTT connection
- `reboot` - Restart device

### Manual Firmware Update

1. Download latest from [releases](https://github.com/martinriedel/mypvlog-firmware/releases)
2. Web interface → "System" → "Firmware Update"
3. Click "Choose File" and select `.bin` file
4. Click "Upload"
5. Wait for reboot

**Or via OTA (mypvlog Direct mode):**
- Updates happen automatically
- Check "System" → "Firmware" for version

### Performance Tuning

**Faster Polling (mypvlog Direct mode):**
- Automatically uses 2-second intervals
- Provides more real-time data

**Adjust Poll Interval (Generic MQTT mode):**
```cpp
// Edit src/config.h
#define HOYMILES_POLL_INTERVAL 3000 // 3 seconds (default: 5000)
```

**Extend WiFi Range:**
```cpp
// Edit src/wifi_manager.cpp
WiFi.setTxPower(WIFI_POWER_19_5dBm); // Maximum power
```

---

## What's Next?

### Home Assistant Integration (Generic MQTT Mode)

```yaml
# configuration.yaml
mqtt:
  sensor:
    - name: "Solar Power"
      state_topic: "opendtu/AA:BB:CC:DD:EE:FF/112182123456/data"
      value_template: "{{ value_json.power }}"
      unit_of_measurement: "W"
      device_class: power

    - name: "Solar Voltage"
      state_topic: "opendtu/AA:BB:CC:DD:EE:FF/112182123456/data"
      value_template: "{{ value_json.voltage }}"
      unit_of_measurement: "V"
      device_class: voltage
```

### Grafana Dashboard (Generic MQTT Mode)

1. Set up InfluxDB to store MQTT data
2. Use Telegraf to subscribe to MQTT topics
3. Create Grafana dashboard for visualizations

### Multiple Inverters

Repeat Step 5 for each inverter (up to 8 inverters per device)

---

## Support

- 📖 **Documentation:** [README.md](README.md)
- 🐛 **Report Issues:** [GitHub Issues](https://github.com/martinriedel/mypvlog-firmware/issues)
- 💬 **Discussions:** [GitHub Discussions](https://github.com/martinriedel/mypvlog-firmware/discussions)
- 🌐 **mypvlog.net:** [https://mypvlog.net](https://mypvlog.net)

---

## Hardware Shopping List

**Minimum Setup (~€15):**
- ESP32 DevKit v1: €5
- NRF24L01+: €2
- Jumper wires: €2
- USB cable: €3
- **Total: ~€12**

**Recommended Setup (~€25):**
- ESP32 DevKit v1: €5
- NRF24L01+ PA+LNA (long range): €5
- PCB prototype board: €3
- Enclosure (weatherproof): €8
- USB power adapter: €4
- **Total: ~€25**

**Pre-Built Option (~€30):**
- OpenDTU Fusion Board (ESP32 + NRF24 + CMT2300A): €30
- Ready to use, professional quality

**Where to Buy:**
- AliExpress, eBay, Amazon
- Local electronics stores
- Maker/DIY shops

---

**Ready to go solar? Flash now at [flash.mypvlog.net](https://flash.mypvlog.net)!** ☀️
