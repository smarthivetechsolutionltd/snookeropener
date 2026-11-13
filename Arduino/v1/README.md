# Smarthive Pool Controller Firmware - V1.0 (ESP32 Edition)

This directory contains the production **V1.0 general product firmware** ported specifically for the **ESP32 DevKit V1**.

## Primary File
- [**`v1.ino`**](file:///Users/user/Documents/Projects/arduino/snookeropener/Arduino/v1/v1.ino): Native ESP32 production firmware with hardware PWM, smooth soft-start servo control, captive portal, and fast hardware TLS encryption.
- [**`web_pages.h`**](file:///Users/user/Documents/Projects/arduino/snookeropener/Arduino/v1/web_pages.h): Responsive embedded Web Portal dashboard.

---

## Hardware Specifications & Pinout (ESP32 DevKit V1)

### 1. RC522 RFID / NFC Reader (Hardware VSPI)
| RC522 Pin | ESP32 Pin | GPIO | Function |
| :--- | :--- | :--- | :--- |
| **3.3V (VCC)** | 3V3 | 3.3V | **CAUTION:** Connect to 3.3V ONLY! Never 5V! |
| **GND** | GND | GND | Common Ground |
| **RST** | D22 | GPIO 22 | Reader Reset |
| **SDA (SS)** | D5 | GPIO 5 | VSPI Chip Select (Slave Select) |
| **SCK** | D18 | GPIO 18 | VSPI Clock |
| **MOSI** | D23 | GPIO 23 | VSPI Master Out Slave In |
| **MISO** | D19 | GPIO 19 | VSPI Master In Slave Out |

### 2. MG996R High-Torque Servo
| Servo Wire | Destination | Notes |
| :--- | :--- | :--- |
| **Orange / Yellow (Signal)** | **GPIO 13** | Hardware LEDC PWM via `ESP32Servo` |
| **Red (+5V Power)** | **External 5V Power Rail** | **NEVER pass through ESP32 pins!** Buffer with 470µF 25V capacitor across 5V and GND. |
| **Brown / Black (GND)** | **Common GND** | Connect to capacitor negative and common ground. |

### 3. RGB Status Indicator (Common Cathode)
| RGB Pin | Destination | Color Channel |
| :--- | :--- | :--- |
| **R (Red)** | **GPIO 26** | Warning / Error / Offline indicator |
| **G (Green)** | **GPIO 27** | Solid Green during Authorized Card Unlock |
| **– (GND)** | **ESP32 GND** | Common Ground |

---

## Required Arduino IDE Libraries
Ensure you have installed:
1. **esp32 by Espressif Systems** (in Board Manager: `Tools > Board > esp32 > DOIT ESP32 DEVKIT V1`)
2. **MFRC522** by GithubCommunity (via Library Manager)
3. **ESP32Servo** by Kevin Harrington (via Library Manager)

---

## Key Features
- **Hardware PWM:** Eliminates all servo twitching and motor buzzing during WiFi activity.
- **Fast HTTPS / TLS:** Uses ESP32's hardware crypto engine for fast, non-blocking cloud events.
- **Micro-Stepped Soft-Start:** Sweeps between 180° and 0° smoothly (4° every 12ms) to eliminate high inrush current spikes.
- **Auto-Detaching Idle (0 mA):** Servo detaches after locking to prevent power drain and motor heating.
- **Native Web OTA:** Browse to `http://192.168.4.1/update` or `http://smarthive.local/update` to flash updated firmware over WiFi.
