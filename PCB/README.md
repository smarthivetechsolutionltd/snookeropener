# SmartHive Pool Opener - Main Controller PCB (V2.0 ESP32 DevKit V1)

Custom PCB for the SmartHive Snooker/Pool Table Ball Release & RFID Authentication System, upgraded from ESP8266 to the **ESP32 DevKit V1 (30-pin)** and high-torque **MG996R servo**.

---

## 1. Specifications

| Parameter | Value |
| :--- | :--- |
| **Dimensions** | 76.2 mm × 53.34 mm (3.0" × 2.1") |
| **Layers** | 2-Layer (FR-4, 1.6 mm thickness, 1 oz copper) |
| **MCU Socket** | ESP32 DevKit V1 (30-Pin, 25.4 mm / 1.0" row pitch) |
| **Power Input** | 5V DC via 5.08 mm screw terminal / header (from RFID Pod USB-C & slide switch) |
| **Reverse Polarity Protection** | SS34 / 1N5819 Schottky Diode (3A) |
| **Servo Power Rail** | Dedicated 5V high-current trace with **470 µF 25V** bulk electrolytic + 100 nF ceramic |
| **Mounting Holes** | 4× M2.5 mounting holes (68.2 mm × 45.34 mm hole spacing) |
| **Manufacturing Package** | Standard JLCPCB / PCBWay ready zip ([smarthive_pool_gerbers.zip](file:///Users/user/Documents/Projects/arduino/snookeropener/PCB/smarthive_pool_gerbers.zip)) |

---

## 2. Pin Connections & Wiring Mapping

### ESP32 DevKit V1 (30-Pin)

| ESP32 Pin | GPIO | Function | Destination Component / Header |
| :--- | :--- | :--- | :--- |
| **VIN** | - | 5V Power In | From 5V Rail (protected by SS34 diode) |
| **GND** | - | System Ground | Common Ground Plane |
| **3V3** | - | 3.3V Power Out | Power to MFRC522 RFID & 3.3V Power LED |
| **GPIO 18** | IO18 | Hardware VSPI SCK | `J2` RFID Pin 7 (SCK) |
| **GPIO 19** | IO19 | Hardware VSPI MISO | `J2` RFID Pin 6 (MISO) |
| **GPIO 23** | IO23 | Hardware VSPI MOSI | `J2` RFID Pin 5 (MOSI) |
| **GPIO 5** | IO5 | Hardware VSPI SS / SDA | `J2` RFID Pin 1 (SDA) |
| **GPIO 22** | IO22 | RFID Hard Reset | `J2` RFID Pin 2 (RST) |
| **GPIO 13** | IO13 | Hardware PWM Servo Signal | `J3` MG996R Servo Pin 3 (Signal) |
| **GPIO 26** | IO26 | Red LED PWM | `J6` HW-479 RGB Module Pin 2 (Red) |
| **GPIO 27** | IO27 | Green LED PWM | `J6` HW-479 RGB Module Pin 3 (Green) |
| **GPIO 32** | IO32 | Manual Release / Bench Test | `SW1` Tactile Pushbutton (Internal Pull-Up) |

---

## 3. Peripheral Connectors

1. **`J5` - 5V Power Terminal (2-Pin 5.08 mm pitch):**
   - Pin 1: `+5V` (Switched 5V from SS-12D00 slide switch)
   - Pin 2: `GND`
2. **`J3` - MG996R Servo Header (1×3 Pin 2.54 mm pitch):**
   - Pin 1: `GND` (Brown / Black)
   - Pin 2: `5V` (Red) — Buffered by 470 µF 25V capacitor
   - Pin 3: `SIG` (Orange / White) — Connected to ESP32 GPIO 13
3. **`J2` - RC522 RFID Header (1×8 Pin 2.54 mm pitch):**
   - Pin 1: `SDA` (GPIO 5)
   - Pin 2: `RST` (GPIO 22)
   - Pin 3: `3V3` (From ESP32 3V3 regulator)
   - Pin 4: `GND`
   - Pin 5: `MOSI` (GPIO 23)
   - Pin 6: `MISO` (GPIO 19)
   - Pin 7: `SCK` (GPIO 18)
   - Pin 8: `IRQ` (N/C)
4. **`J6` - HW-479 RGB LED Module Header (1×4 Pin 2.54 mm pitch):**
   - Pin 1: `GND` (Cathode)
   - Pin 2: `RED` (GPIO 26)
   - Pin 3: `GREEN` (GPIO 27)
   - Pin 4: `BLUE` (Optional / N/C)
5. **`SW1` - Bench Test Button (6×6 mm Tactile Button):**
   - Triggers ball release manually without needing an RFID card during servicing or installation.

---

## 4. Production Files

- **Gerbers & Drill Package:** [`smarthive_pool_gerbers.zip`](file:///Users/user/Documents/Projects/arduino/snookeropener/PCB/smarthive_pool_gerbers.zip)
- **Bill of Materials:** [`smarthive_pool_bom.csv`](file:///Users/user/Documents/Projects/arduino/snookeropener/PCB/smarthive_pool_bom.csv)
- **KiCad PCB Layout:** [`smarthive_pool.kicad_pcb`](file:///Users/user/Documents/Projects/arduino/snookeropener/PCB/smarthive_pool.kicad_pcb)
- **KiCad Schematic:** [`smarthive_pool.kicad_sch`](file:///Users/user/Documents/Projects/arduino/snookeropener/PCB/smarthive_pool.kicad_sch)
- **3D Render (Isometric):** [`pcb_render_3d_isometric.png`](file:///Users/user/Documents/Projects/arduino/snookeropener/PCB/pcb_render_3d_isometric.png)
- **3D Render (Top View):** [`pcb_render_3d_top.png`](file:///Users/user/Documents/Projects/arduino/snookeropener/PCB/pcb_render_3d_top.png)
- **3D Render (Bottom View):** [`pcb_render_3d_bottom.png`](file:///Users/user/Documents/Projects/arduino/snookeropener/PCB/pcb_render_3d_bottom.png)

