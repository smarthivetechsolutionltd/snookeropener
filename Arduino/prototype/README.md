# Adrenaline Recreation Hub - Prototype Controller Firmware

This sketch (`nodemcuESP8266_adrenaline.ino`) contains the prototype firmware dedicated specifically to the **Adrenaline Recreation Hub** deployment.

## Specifications
- **Target Hardware**: NodeMCU ESP8266 (ESP-12E / ESP-12F)
- **Cloud Backend**: Adrenaline Recreation Hub (`https://adrenalinerecreationhub.com/api/snooker/sync`)
- **Default Device Secret**: `adrenaline_snooker_secret_2026`
- **Authentication Realm**: `Adrenaline_Snooker_Hub`
- **SoftAP Default**: `NodeMCU_Hub` / `setup1234`

## How to Flash in Arduino IDE
1. Open Arduino IDE.
2. Select **File > Open...** and select `nodemcuESP8266_adrenaline/nodemcuESP8266_adrenaline.ino`.
3. Choose Board: **NodeMCU 1.0 (ESP-12E Module)**.
4. Select the matching USB Serial COM port.
5. Click **Upload**.

