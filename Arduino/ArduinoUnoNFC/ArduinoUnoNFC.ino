/*
 * Arduino UNO NFC / RFID (RC522) + Coin Microswitch + SG90 Servo + 5461AS-1
 * Display + Internal Software Real-Time Clock
 *
 * Hardware Wiring Reference:
 * ===============================================================================================
 * 1. RC522 RFID Module (Hardware SPI Interface):
 * -----------------------------------------------------------------------------------------------
 * RC522 Pin      | Arduino UNO Pin  | Note
 * ---------------|------------------|------------------------------------------------------------
 * VCC (3.3V)     | 3.3V             | CAUTION: Connect to 3.3V ONLY! (NOT 5V)
 * RST            | 3.3V             | Connect to 3.3V to keep chip enabled
 * (Frees Pin 9 for RTC!) GND            | GND              | Ground MISO | Pin
 * 12           | Hardware SPI MISO MOSI           | Pin 11           | Hardware
 * SPI MOSI SCK            | Pin 13           | Hardware SPI Clock SDA (SS) |
 * Pin 10           | Hardware SPI Chip Select
 *
 * 2. SG90 Servo Motor:
 * -----------------------------------------------------------------------------------------------
 * Servo Wire     | Arduino UNO Pin  | Note
 * ---------------|------------------|------------------------------------------------------------
 * Brown / Black  | GND              | Ground
 * Red            | 5V               | 5V Power (or external 5V supply)
 * Orange/Yellow  | Pin 3            | PWM Control Signal
 *
 * 3. Physical Coin Microswitch / Push Button:
 * -----------------------------------------------------------------------------------------------
 * Switch Wire    | Arduino UNO Pin  | Note
 * ---------------|------------------|------------------------------------------------------------
 * Terminal 1 (NO)| Pin 2            | Configured with internal INPUT_PULLUP
 * Terminal 2(COM)| GND              | Ground (No external resistor required!)
 *
 * 4. 5461AS-1 (12-Pin, 4-Digit 7-Segment Common Cathode Display):
 * -----------------------------------------------------------------------------------------------
 * 5461AS Pin     | Function         | Arduino UNO Pin  | Notes
 * ---------------|------------------|------------------|-----------------------------------------
 * Pin 12 (Top)   | Digit 1 (Left)   | Pin 4            | Connect via 1kΩ Resistor
 * Pin 9  (Top)   | Digit 2          | Pin 5            | Connect via 1kΩ Resistor
 * Pin 8  (Top)   | Digit 3          | Pin 6            | Connect via 1kΩ Resistor
 * Pin 6  (Btm)   | Digit 4 (Right)  | Pin 7            | Connect via 1kΩ Resistor
 * Pin 11 (Top)   | Segment A        | Pin 8            | Dedicated Direct Connection
 * Pin 7  (Top)   | Segment B        | Pin A0           | Dedicated Direct Connection
 * Pin 4  (Btm)   | Segment C        | Pin A1           | Dedicated Direct Connection
 * Pin 2  (Btm)   | Segment D        | Pin A2           | Dedicated Direct Connection
 * Pin 1  (Btm)   | Segment E        | Pin A3           | Dedicated Direct Connection
 * Pin 10 (Top)   | Segment F        | Pin A4           | Dedicated Direct Connection
 * Pin 5  (Btm)   | Segment G        | Pin A5           | Dedicated Direct Connection
 * Pin 3  (Btm)   | Decimal Point DP | Not connected    | Not needed for count
 *
 * 5. Real-Time Clock: Internal Software RTC (No external module needed!)
 * -----------------------------------------------------------------------------------------------
 * Uses Arduino Uno's onboard 16MHz crystal + millis() timer.
 * Automatically synchronizes with compile time (__DATE__ __TIME__) on upload/boot,
 * and can be dynamically synchronized live via Serial ('s' menu option).
 * Zero extra pins, zero wires, zero display flicker! (Pin 9 is completely free!)
 * ===============================================================================================
 */

#include <EEPROM.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>

// ==================== PIN DEFINITIONS ====================
#define RC522_SS_PIN 10
#define SERVO_PIN 3
#define BUTTON_PIN 2 // Physical Coin Microswitch (INPUT_PULLUP to GND)

#define SERVO_LOCKED_ANGLE 0 // Position in degrees when door is closed/locked
#define SERVO_UNLOCKED_ANGLE                                                   \
  90                        // Position in degrees when door is open/unlocked
#define UNLOCK_HOLD_MS 3000 // How long to stay open (milliseconds)

// MFRC522: Using UNUSED_PIN for RST because RC522 RST is wired directly to 3.3V
MFRC522 mfrc522(RC522_SS_PIN, MFRC522::UNUSED_PIN);
MFRC522::MIFARE_Key key;
Servo lockServo;

// ==================== EEPROM NON-VOLATILE STORAGE ====================
#define EEPROM_MAGIC_0 0x53      // 'S'
#define EEPROM_MAGIC_1 0x4E      // 'N'
#define EEPROM_MAGIC_ADDR 0      // 2 bytes: Signature check
#define EEPROM_TOTAL_ADDR 2      // 4 bytes: uint32_t lifetime total racks
#define EEPROM_HEAD_ADDR 6       // 2 bytes: uint16_t next write index
#define EEPROM_COUNT_ADDR 8      // 2 bytes: uint16_t total saved logs
#define EEPROM_LOG_START_ADDR 16 // Start of log array

#define MAX_LOG_ENTRIES                                                        \
  85 // 85 entries * 11 bytes = 935 bytes (16 + 935 = 951 <= 1024 bytes)

struct LogEntry {
  uint32_t timestamp;    // Epoch timestamp (seconds since 1970)
  uint16_t rackNumber;   // Sequential rack number (1..65535)
  uint8_t triggerType;   // 1 = Coin Switch, 2 = RFID Card
  uint8_t uidPreview[4]; // 4 bytes Card UID or 0 for coin
};

struct DateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Button debounce & session counter
int stableButtonState = HIGH;
int lastButtonReading = HIGH;
unsigned long lastButtonDebounceTime = 0;
const unsigned long BUTTON_DEBOUNCE_DELAY = 50; // 50ms software debounce
unsigned long sessionRackCount = 0;

// RC522 live connection monitoring
bool rc522Connected = false;
unsigned long lastRc522CheckTime = 0;
const unsigned long RC522_CHECK_INTERVAL = 1000; // Check health every 1 second

// ==================== 5461AS-1 (4-DIGIT 7-SEGMENT DISPLAY)
// ==================== Common Cathodes (active LOW): Dig 1 (Pin 12), Dig 2 (Pin
// 9), Dig 3 (Pin 8), Dig 4 (Pin 6)
const uint8_t DIGIT_PINS[4] = {4, 5, 6, 7};
// Segments A-G (active HIGH): A (Pin 11), B (Pin 7), C (Pin 4), D (Pin 2), E
// (Pin 1), F (Pin 10), G (Pin 5)
const uint8_t SEGMENT_PINS[7] = {8, A0, A1, A2, A3, A4, A5};

// Segment bitmasks: bits correspond to [G, F, E, D, C, B, A]
const uint8_t DIGIT_PATTERNS[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

volatile uint8_t displayDigits[4] = {0b00111111, 0b00111111, 0b00111111,
                                     0b00111111};
volatile uint8_t activeDigitIndex = 0;

// ==================== HELPER DECLARATIONS ====================
void printMenu();
bool waitForCard(unsigned long timeoutMs = 10000);
void stopCard();
void checkReaderConnection();
void checkReaderConnectionStatusManual();

// Display & RTC
void initDisplay();
void setDisplayNumber(unsigned long num);
void testDisplay();
void rtcInit();
bool rtcReadDateTime(DateTime &dt);
void rtcSetDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour,
                    uint8_t minute, uint8_t second);
void rtcPrintCurrentTime();
void rtcAutoSyncCompileTime();
void rtcSyncInteractive();
uint32_t rtcGetEpoch();
void rtcSetEpoch(uint32_t epoch);
uint32_t dateTimeToEpoch(const DateTime &dt);
DateTime epochToDateTime(uint32_t epoch);

// EEPROM
void initEEPROM();
void logRackEvent(uint8_t triggerType, const byte *cardUid = nullptr);
void printEEPROMLog();
void clearEEPROMData();

// Door lock & controls
void updateButton();
void triggerServoDoorLock();
void triggerServoDoorLock(const __FlashStringHelper *triggerSource);
void testServo();
void scanAndIdentify();
void readDataBlock();
void writeTextData();
void writeNumericValue();
void dumpFullCard();
String readLineFromSerial();

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ; // Wait for serial monitor

  SPI.begin();        // Initialize SPI bus
  mfrc522.PCD_Init(); // Initialize MFRC522 card reader
  delay(100);

  // Set default factory key (FFFFFFFFFFFF)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Initialize Microswitch Button with internal pull-up (active LOW)
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize Internal Software Real-Time Clock (auto-syncs to compile timestamp)
  rtcInit();

  // Initialize EEPROM persistent storage & load total rack count
  initEEPROM();

  // Initialize 5461AS-1 4-Digit Display with Timer2 ISR multiplexing
  initDisplay();

  // Initialize SG90 Servo and park at 0 deg, then detach to save power
  lockServo.attach(SERVO_PIN);
  lockServo.write(SERVO_LOCKED_ANGLE);
  delay(500);
  lockServo
      .detach(); // Detach on idle to eliminate humming, heat, and power drain

  Serial.println(F("\n=================================================="));
  Serial.println(F(" Arduino UNO NFC + Coin Switch + 5461AS + Internal Clock "));
  Serial.println(F("=================================================="));

  // Check RC522 hardware connection
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  rc522Connected = (version != 0x00 && version != 0xFF);
  Serial.print(F("RC522 Firmware Version: 0x"));
  Serial.print(version, HEX);
  if (version == 0x91 || version == 0x92) {
    Serial.println(F(" (Verified Genuine RC522 - CONNECTED)"));
  } else if (version == 0x12) {
    Serial.println(F(" (Counterfeit / Clone detected - CONNECTED)"));
  } else if (!rc522Connected) {
    Serial.println(F(" (WARNING: Reader NOT detected / DISCONNECTED! Check "
                     "3.3V/GND/SPI wires!)"));
  } else {
    Serial.println(F(" (Custom/Unknown Version - CONNECTED)"));
  }

  Serial.print(F("SG90 Servo: Initialized on Pin "));
  Serial.print(SERVO_PIN);
  Serial.println(F(" (Locked at 0 deg, power-save detach active)"));

  Serial.print(F("Coin Switch: Initialized on Pin "));
  Serial.print(BUTTON_PIN);
  Serial.println(F(" (INPUT_PULLUP to GND)"));

  Serial.println(F("5461AS-1 Display: Initialized (Digits 4-7, Segments 8, "
                   "A0-A5, Timer2 500Hz ISR)"));

  Serial.print(F("Internal Clock: "));
  rtcPrintCurrentTime();

  Serial.print(F("Lifetime Total Racks Restored from EEPROM: "));
  Serial.println(sessionRackCount);

  printMenu();
}

void loop() {
  // 1. Monitor RC522 connection state in real-time (Hot-plug & disconnect
  // detection)
  checkReaderConnection();

  // 2. Hardware Coin Microswitch / Button Check
  updateButton();

  // 3. Standalone Tap: Only query card if reader is actually connected
  if (rc522Connected && mfrc522.PICC_IsNewCardPresent() &&
      mfrc522.PICC_ReadCardSerial()) {
    logRackEvent(2, mfrc522.uid.uidByte); // 2 = RFID Card
    triggerServoDoorLock(F("RFID Card"));
  }

  // 4. Serial Commands: Handle terminal options if user types in Serial Monitor
  if (Serial.available() > 0) {
    char choice = Serial.read();

    // Skip carriage returns / newlines
    if (choice == '\r' || choice == '\n')
      return;

    Serial.println();
    switch (choice) {
    case 'h':
    case 'H':
      printEEPROMLog();
      break;
    case 't':
    case 'T':
      rtcPrintCurrentTime();
      break;
    case 's':
    case 'S':
      rtcSyncInteractive();
      break;
    case 'c':
    case 'C':
      clearEEPROMData();
      break;
    case '1':
      scanAndIdentify();
      break;
    case '2':
      readDataBlock();
      break;
    case '3':
      writeTextData();
      break;
    case '4':
      writeNumericValue();
      break;
    case '5':
      dumpFullCard();
      break;
    case '6':
      testServo();
      break;
    case '7':
      Serial.print(F("\n[STATS] Lifetime Total Racks: "));
      Serial.println(sessionRackCount);
      break;
    case '8':
      checkReaderConnectionStatusManual();
      break;
    case '9':
      testDisplay();
      break;
    case 'm':
    case 'M':
      printMenu();
      break;
    default:
      Serial.println(F("Invalid command. Type 'm' to reprint menu."));
      break;
    }

    // Flush any leftover characters from Serial buffer
    while (Serial.available())
      Serial.read();

    Serial.println(F("\nReady. Tap card, trip coin switch, or select option "
                     "(1-9 / h / t / s / c) / 'm':"));
  }

  // Small delay to yield CPU without missing rapid switch contacts
  delay(5);
}

// ==================== MENU DISPLAY ====================
void printMenu() {
  Serial.println(F("\n--------------------- MAIN MENU ---------------------"));
  Serial.println(
      F("  [AUTO] Tap RFID card OR trip Coin Switch to Open Lock (90 deg)"));
  Serial.println(
      F("  [h] View Audit Log History (Timestamped EEPROM Records)"));
  Serial.println(F("  [t] Check Live Clock (Date & Time)"));
  Serial.println(F("  [s] Sync Internal Clock (Compile Time / Live Timestamp)"));
  Serial.println(F("  [c] Reset Lifetime Total & Clear EEPROM History"));
  Serial.println(F("  [1] Scan & Identify Tag (Read UID & Card Type)"));
  Serial.println(F("  [2] Read Sector Block (ASCII Text & Hex Dump)"));
  Serial.println(F("  [3] Write Custom Text to Block (e.g. Player Name)"));
  Serial.println(F("  [4] Write Numeric Token / Value (e.g. 1000 Naira)"));
  Serial.println(F("  [5] Full Sector & Memory Dump (Inspect All Data)"));
  Serial.println(
      F("  [6] Test Servo Motor (Manual Open 90 deg -> Close 0 deg)"));
  Serial.println(F("  [7] View Rack Count (Session Statistics)"));
  Serial.println(
      F("  [8] Check RC522 Reader Diagnostic (Live Connection Status)"));
  Serial.println(F("  [9] Test 5461AS-1 Display (Cycle Segments & Digits)"));
  Serial.println(F("  [m] Show Menu Again"));
  Serial.println(F("-----------------------------------------------------"));
  Serial.print(F("Enter choice (or just tap a card): "));
}

// ==================== WAIT FOR CARD TAP ====================
bool waitForCard(unsigned long timeoutMs) {
  if (!rc522Connected) {
    Serial.println(F("[ERROR] RFID Reader is currently DISCONNECTED. Please "
                     "plug it in first!"));
    return false;
  }

  Serial.println(F("[WAIT] Place your NFC / RFID card on the reader..."));
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    checkReaderConnection();
    if (!rc522Connected) {
      Serial.println(F("[ABORT] RFID Reader was disconnected while waiting."));
      return false;
    }

    // Look for new cards
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      Serial.println(F("[DETECTED] Card found!"));
      return true;
    }
    delay(50);
  }

  Serial.println(F("[TIMEOUT] No card detected. Returning to menu."));
  return false;
}

void stopCard() {
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// ==================== RC522 HOT-PLUG & DISCONNECT MONITOR ====================
void checkReaderConnection() {
  if (millis() - lastRc522CheckTime < RC522_CHECK_INTERVAL) {
    return;
  }
  lastRc522CheckTime = millis();

  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  bool isConnected = (version != 0x00 && version != 0xFF);

  if (isConnected && !rc522Connected) {
    // Transition: Disconnected -> Connected (Hot-plugged or wires restored!)
    rc522Connected = true;
    mfrc522.PCD_Init(); // Re-initialize the chip registers & antenna field
    delay(50);
    Serial.println(F("\n=================================================="));
    Serial.println(F("  >>> [RC522 RFID BOARD RECONNECTED / DETECTED] <<<"));
    Serial.print(F("  Firmware Version: 0x"));
    Serial.print(version, HEX);
    if (version == 0x91 || version == 0x92) {
      Serial.println(F(" (Genuine RC522)"));
    } else if (version == 0x12) {
      Serial.println(F(" (Clone RC522)"));
    } else {
      Serial.println(F(" (Active)"));
    }
    Serial.println(F("  RFID card reading is now ACTIVE."));
    Serial.println(F("=================================================="));
  } else if (!isConnected && rc522Connected) {
    // Transition: Connected -> Disconnected!
    rc522Connected = false;
    Serial.println(F("\n=================================================="));
    Serial.println(F("  !!! [RC522 RFID BOARD DISCONNECTED / LOST] !!!"));
    Serial.println(F("  No response on SPI bus (reads 0x00 / 0xFF)."));
    Serial.println(F("  Check 3.3V (VCC), GND, or SPI jumper wires."));
    Serial.println(F("  (Coin microswitch is still functional)"));
    Serial.println(F("=================================================="));
  }
}

void checkReaderConnectionStatusManual() {
  byte version = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  bool isConnected = (version != 0x00 && version != 0xFF);
  Serial.println(F("\n--- RC522 Connection Diagnostic ---"));
  Serial.print(F("Status: "));
  if (isConnected) {
    Serial.println(F("CONNECTED / ONLINE"));
    Serial.print(F("Firmware Version: 0x"));
    Serial.print(version, HEX);
    if (version == 0x91 || version == 0x92) {
      Serial.println(F(" (Genuine RC522)"));
    } else if (version == 0x12) {
      Serial.println(F(" (Clone RC522)"));
    } else {
      Serial.println(F(" (Custom)"));
    }
    Serial.println(F("SPI Bus & Power: OK"));
  } else {
    Serial.println(F("DISCONNECTED / OFFLINE"));
    Serial.print(F("Version Register: 0x"));
    Serial.print(version, HEX);
    Serial.println(F(" (No response on SPI bus)"));
    Serial.println(F("Troubleshooting: Check 3.3V (VCC), GND, Pin 10 (SDA), "
                     "Pin 13 (SCK), Pin 11 (MOSI), Pin 12 (MISO)."));
  }
  Serial.println(F("-----------------------------------"));
}

// ==================== 5461AS-1 DISPLAY ROUTINES & TIMER2 ISR
// ==================== Timer2 Compare Match A Interrupt - refreshes 1 digit
// every 2ms (~500Hz total multiplex rate)
ISR(TIMER2_COMPA_vect) {
  // 1. Turn off all digits to prevent ghosting (Common Cathode OFF = HIGH)
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(DIGIT_PINS[i], HIGH);
  }

  // 2. Advance to next digit
  activeDigitIndex = (activeDigitIndex + 1) & 0x03; // modulo 4

  // 3. Set segment states for this digit
  uint8_t pattern = displayDigits[activeDigitIndex];
  for (uint8_t s = 0; s < 7; s++) {
    digitalWrite(SEGMENT_PINS[s], (pattern & (1 << s)) ? HIGH : LOW);
  }

  // 4. Turn on active digit (Common Cathode ON = LOW)
  digitalWrite(DIGIT_PINS[activeDigitIndex], LOW);
}

void initDisplay() {
  for (uint8_t i = 0; i < 4; i++) {
    pinMode(DIGIT_PINS[i], OUTPUT);
    digitalWrite(DIGIT_PINS[i], HIGH); // Digits off
  }
  for (uint8_t s = 0; s < 7; s++) {
    pinMode(SEGMENT_PINS[s], OUTPUT);
    digitalWrite(SEGMENT_PINS[s], LOW); // Segments off
  }

  setDisplayNumber(sessionRackCount);

  // Setup Timer2 compare match A interrupt (~500Hz refresh rate)
  cli();
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;
  OCR2A = 249;                         // 16MHz / (128 * 500Hz) - 1 = 249
  TCCR2A |= (1 << WGM21);              // CTC mode
  TCCR2B |= (1 << CS22) | (1 << CS20); // 128 prescaler
  TIMSK2 |= (1 << OCIE2A);             // Enable Timer2 Compare Interrupt
  sei();
}

void setDisplayNumber(unsigned long num) {
  if (num > 9999)
    num = 9999;
  displayDigits[3] = DIGIT_PATTERNS[num % 10];
  displayDigits[2] = DIGIT_PATTERNS[(num / 10) % 10];
  displayDigits[1] = DIGIT_PATTERNS[(num / 100) % 10];
  displayDigits[0] = DIGIT_PATTERNS[(num / 1000) % 10];
}

void testDisplay() {
  Serial.println(F("\n[TEST] Testing 5461AS-1 4-Digit Display..."));
  Serial.println(F("[TEST] Cycling numbers 0 to 99 on display..."));
  unsigned long orig = sessionRackCount;
  for (int i = 0; i <= 99; i += 11) {
    setDisplayNumber(i);
    delay(250);
  }
  setDisplayNumber(8888);
  Serial.println(F("[TEST] Showing '8888' (all segments ON) for 1 second..."));
  delay(1000);
  setDisplayNumber(orig);
  Serial.println(F("[TEST] Display test complete. Restored session count."));
}

// ==================== INTERNAL SOFTWARE REAL-TIME CLOCK ====================
// Uses Arduino Uno's onboard 16MHz crystal + millis() timer.
// No external RTC hardware, wires, or coin battery required!

uint32_t syncEpoch = 0;       // Base timestamp in seconds (Unix Epoch)
unsigned long syncMillis = 0; // millis() recorded at the moment of calibration

uint32_t dateTimeToEpoch(const DateTime &dt) {
  uint32_t days = 0;
  for (uint16_t y = 1970; y < dt.year; y++) {
    days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
  }
  for (uint8_t m = 1; m < dt.month; m++) {
    days += daysInMonth[m - 1];
    if (m == 2 &&
        (dt.year % 4 == 0 && (dt.year % 100 != 0 || dt.year % 400 == 0))) {
      days += 1;
    }
  }
  days += (dt.day - 1);
  return days * 86400UL + dt.hour * 3600UL + dt.minute * 60UL + dt.second;
}

DateTime epochToDateTime(uint32_t epoch) {
  DateTime dt;
  dt.second = epoch % 60;
  epoch /= 60;
  dt.minute = epoch % 60;
  epoch /= 60;
  dt.hour = epoch % 24;
  uint32_t days = epoch / 24;

  uint16_t y = 1970;
  while (true) {
    uint16_t dInY = (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    if (days >= dInY) {
      days -= dInY;
      y++;
    } else {
      break;
    }
  }
  dt.year = y;

  uint8_t m = 1;
  while (true) {
    uint8_t dInM = daysInMonth[m - 1];
    if (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0))) {
      dInM = 29;
    }
    if (days >= dInM) {
      days -= dInM;
      m++;
    } else {
      break;
    }
  }
  dt.month = m;
  dt.day = days + 1;
  return dt;
}

uint32_t rtcGetEpoch() {
  if (syncEpoch == 0) return 0;
  return syncEpoch + ((millis() - syncMillis) / 1000UL);
}

void rtcSetEpoch(uint32_t epoch) {
  syncEpoch = epoch;
  syncMillis = millis();
}

void rtcSetDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour,
                    uint8_t minute, uint8_t second) {
  DateTime dt;
  dt.year = year;
  dt.month = month;
  dt.day = day;
  dt.hour = hour;
  dt.minute = minute;
  dt.second = second;
  rtcSetEpoch(dateTimeToEpoch(dt));
}

bool rtcReadDateTime(DateTime &dt) {
  uint32_t epoch = rtcGetEpoch();
  if (epoch == 0) return false;
  dt = epochToDateTime(epoch);
  return true;
}

void rtcPrintCurrentTime() {
  DateTime dt;
  if (rtcReadDateTime(dt)) {
    char buf[35];
    snprintf(buf, sizeof(buf), "[CLOCK LIVE] %04u-%02u-%02u %02u:%02u:%02u", dt.year,
             dt.month, dt.day, dt.hour, dt.minute, dt.second);
    Serial.println(buf);
  } else {
    Serial.println(F("[CLOCK WARNING] Internal clock uncalibrated. Type 's' to sync."));
  }
}

void rtcAutoSyncCompileTime() {
  const char *dateStr = __DATE__;
  const char *timeStr = __TIME__;

  char monthName[4];
  int day, year, hour, minute, second;
  sscanf(dateStr, "%3s %d %d", monthName, &day, &year);
  sscanf(timeStr, "%d:%d:%d", &hour, &minute, &second);

  const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  int month = 1;
  for (int i = 0; i < 12; i++) {
    if (strncmp(monthName, months[i], 3) == 0) {
      month = i + 1;
      break;
    }
  }

  rtcSetDateTime(year, month, day, hour, minute, second);

  DateTime dt;
  if (rtcReadDateTime(dt)) {
    Serial.print(F("[CLOCK] Internal clock successfully calibrated to: "));
    char buf[30];
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u", dt.year, dt.month, dt.day,
             dt.hour, dt.minute, dt.second);
    Serial.println(buf);
  }
}

void rtcInit() {
  // Automatically initialize internal clock from compile timestamp at boot
  rtcAutoSyncCompileTime();
}

void rtcSyncInteractive() {
  Serial.println(F("\n--- INTERNAL CLOCK SYNCHRONIZATION ---"));
  Serial.println(F("Options:"));
  Serial.println(F("  1. Press ENTER or type 'c' to sync with code compile time"));
  Serial.println(F("  2. Enter a 10-digit Unix timestamp (e.g. 1771485600)"));
  Serial.println(F("  3. Enter custom time as: YYYY-MM-DD HH:MM:SS"));
  Serial.print(F("Enter selection: "));

  String input = readLineFromSerial();
  input.trim();

  if (input.length() == 0 || input.equalsIgnoreCase("c") || input.equalsIgnoreCase("now")) {
    rtcAutoSyncCompileTime();
    return;
  }

  // Check if user entered a Unix epoch number
  bool isAllDigits = true;
  for (unsigned int i = 0; i < input.length(); i++) {
    if (!isDigit(input[i])) {
      isAllDigits = false;
      break;
    }
  }

  if (isAllDigits && input.length() >= 9) {
    uint32_t epoch = strtoul(input.c_str(), nullptr, 10);
    if (epoch > 1600000000UL) {
      rtcSetEpoch(epoch);
      Serial.print(F("[CLOCK] Synced from Unix Epoch: "));
      rtcPrintCurrentTime();
      return;
    }
  }

  // Check if user entered "YYYY-MM-DD HH:MM:SS"
  int y, mo, d, h, mi, s;
  if (sscanf(input.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s) == 6) {
    if (y >= 2020 && mo >= 1 && mo <= 12 && d >= 1 && d <= 31 &&
        h >= 0 && h <= 23 && mi >= 0 && mi <= 59 && s >= 0 && s <= 59) {
      rtcSetDateTime(y, mo, d, h, mi, s);
      Serial.print(F("[CLOCK] Custom date/time set: "));
      rtcPrintCurrentTime();
      return;
    }
  }

  Serial.println(F("[ERROR] Unrecognized format. Synced to compile time instead."));
  rtcAutoSyncCompileTime();
}

// ==================== EEPROM AUDIT LOG FUNCTIONS ====================
void initEEPROM() {
  uint8_t m0 = EEPROM.read(EEPROM_MAGIC_ADDR);
  uint8_t m1 = EEPROM.read(EEPROM_MAGIC_ADDR + 1);

  if (m0 != EEPROM_MAGIC_0 || m1 != EEPROM_MAGIC_1) {
    // Brand new or uninitialized EEPROM: format it
    Serial.println(
        F("[EEPROM] First-time setup detected. Initializing storage..."));
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_0);
    EEPROM.write(EEPROM_MAGIC_ADDR + 1, EEPROM_MAGIC_1);
    uint32_t zeroTotal = 0;
    uint16_t zero16 = 0;
    EEPROM.put(EEPROM_TOTAL_ADDR, zeroTotal);
    EEPROM.put(EEPROM_HEAD_ADDR, zero16);
    EEPROM.put(EEPROM_COUNT_ADDR, zero16);
    sessionRackCount = 0;
  } else {
    // Load lifetime total count
    uint32_t savedTotal = 0;
    EEPROM.get(EEPROM_TOTAL_ADDR, savedTotal);
    sessionRackCount = savedTotal;
  }
}

void logRackEvent(uint8_t triggerType, const byte *cardUid) {
  sessionRackCount++;
  EEPROM.put(EEPROM_TOTAL_ADDR, (uint32_t)sessionRackCount);
  setDisplayNumber(sessionRackCount);

  // Read current date and time from RTC
  DateTime dt;
  uint32_t epoch = 0;
  if (rtcReadDateTime(dt)) {
    epoch = dateTimeToEpoch(dt);
  }

  // Read ring buffer pointers
  uint16_t head = 0;
  uint16_t count = 0;
  EEPROM.get(EEPROM_HEAD_ADDR, head);
  EEPROM.get(EEPROM_COUNT_ADDR, count);

  if (head >= MAX_LOG_ENTRIES)
    head = 0;

  LogEntry entry;
  entry.timestamp = epoch;
  entry.rackNumber =
      (uint16_t)(sessionRackCount > 65535 ? 65535 : sessionRackCount);
  entry.triggerType = triggerType;
  if (cardUid != nullptr) {
    for (uint8_t i = 0; i < 4; i++)
      entry.uidPreview[i] = cardUid[i];
  } else {
    for (uint8_t i = 0; i < 4; i++)
      entry.uidPreview[i] = 0;
  }

  // Write entry to circular buffer
  int writeAddr = EEPROM_LOG_START_ADDR + (head * sizeof(LogEntry));
  EEPROM.put(writeAddr, entry);

  // Advance head pointer
  head = (head + 1) % MAX_LOG_ENTRIES;
  if (count < MAX_LOG_ENTRIES)
    count++;

  EEPROM.put(EEPROM_HEAD_ADDR, head);
  EEPROM.put(EEPROM_COUNT_ADDR, count);
}

void printEEPROMLog() {
  uint16_t head = 0;
  uint16_t count = 0;
  EEPROM.get(EEPROM_HEAD_ADDR, head);
  EEPROM.get(EEPROM_COUNT_ADDR, count);

  if (head >= MAX_LOG_ENTRIES)
    head = 0;
  if (count > MAX_LOG_ENTRIES)
    count = MAX_LOG_ENTRIES;

  Serial.println(F("\n========================================================="
                   "============="));
  Serial.println(F("              SNOOKER RACK AUDIT LOG (EEPROM HISTORY)      "
                   "           "));
  Serial.println(F("==========================================================="
                   "==========="));
  Serial.print(F("Lifetime Total Racks Counted: "));
  Serial.println(sessionRackCount);
  Serial.print(F("Saved Historical Events:      "));
  Serial.print(count);
  Serial.print(F(" / "));
  Serial.println(MAX_LOG_ENTRIES);
  Serial.println(F("-----------------------------------------------------------"
                   "-----------"));
  Serial.println(F("RACK # | DATE & TIME           | TRIGGER SOURCE"));
  Serial.println(F("-------|-----------------------|---------------------------"
                   "-----------"));

  if (count == 0) {
    Serial.println(F("  No rack events logged in memory yet."));
  } else {
    // Calculate oldest entry index in the ring buffer
    int startIndex;
    if (count < MAX_LOG_ENTRIES) {
      startIndex = 0;
    } else {
      startIndex =
          head; // In full buffer, head points to the oldest overwritten slot
    }

    for (uint16_t i = 0; i < count; i++) {
      int idx = (startIndex + i) % MAX_LOG_ENTRIES;
      int addr = EEPROM_LOG_START_ADDR + (idx * sizeof(LogEntry));
      LogEntry entry;
      EEPROM.get(addr, entry);

      char buf[30];
      snprintf(buf, sizeof(buf), "#%04u  | ", entry.rackNumber);
      Serial.print(buf);

      if (entry.timestamp > 0) {
        DateTime dt = epochToDateTime(entry.timestamp);
        snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u | ", dt.year,
                 dt.month, dt.day, dt.hour, dt.minute, dt.second);
        Serial.print(buf);
      } else {
        Serial.print(F("0000-00-00 00:00:00   | "));
      }

      if (entry.triggerType == 1) {
        Serial.println(F("Coin Switch / Button"));
      } else if (entry.triggerType == 2) {
        Serial.print(F("RFID Card [UID: "));
        for (uint8_t u = 0; u < 4; u++) {
          if (entry.uidPreview[u] < 0x10)
            Serial.print(F("0"));
          Serial.print(entry.uidPreview[u], HEX);
          if (u < 3)
            Serial.print(F(" "));
        }
        Serial.println(F("]"));
      } else {
        Serial.println(F("Manual / Other"));
      }
    }
  }
  Serial.println(F("==========================================================="
                   "==========="));
}

void clearEEPROMData() {
  Serial.println(
      F("\n[EEPROM] Resetting Lifetime Total & Clearing History Log..."));
  sessionRackCount = 0;
  uint32_t zeroTotal = 0;
  uint16_t zero16 = 0;
  EEPROM.put(EEPROM_TOTAL_ADDR, zeroTotal);
  EEPROM.put(EEPROM_HEAD_ADDR, zero16);
  EEPROM.put(EEPROM_COUNT_ADDR, zero16);
  setDisplayNumber(0);
  Serial.println(F("[EEPROM] Reset complete. Count is 0000 and log is empty."));
}

// ==================== HARDWARE BUTTON / MICROSWITCH LOGIC ====================
void updateButton() {
  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {
    lastButtonDebounceTime = millis();
  }

  if ((millis() - lastButtonDebounceTime) > BUTTON_DEBOUNCE_DELAY) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      // With INPUT_PULLUP, LOW indicates contact closed to GND (coin passed /
      // button pressed)
      if (stableButtonState == LOW) {
        logRackEvent(1); // 1 = Coin Switch / Button
        triggerServoDoorLock(F("Coin Switch / Button"));
      }
    }
  }

  lastButtonReading = reading;
}

// ==================== SERVO DOOR LOCK CONTROLLER ====================
void triggerServoDoorLock() { triggerServoDoorLock(F("RFID Card")); }

void triggerServoDoorLock(const __FlashStringHelper *triggerSource) {
  Serial.println(F("\n=================================================="));
  Serial.print(F("    >>> [TRIGGER: "));
  Serial.print(triggerSource);
  Serial.println(F(" / ACCESS GRANTED] <<<"));
  Serial.println(F("=================================================="));

  // Print current timestamp from RTC
  DateTime dt;
  if (rtcReadDateTime(dt)) {
    char buf[25];
    snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u", dt.year,
             dt.month, dt.day, dt.hour, dt.minute, dt.second);
    Serial.print(F("Event Timestamp:     "));
    Serial.println(buf);
  }

  // If RFID triggered it, print Card UID
  if (mfrc522.uid.size > 0 && triggerSource == F("RFID Card")) {
    Serial.print(F("Scanned Card UID:    "));
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      if (mfrc522.uid.uidByte[i] < 0x10)
        Serial.print(F("0"));
      Serial.print(mfrc522.uid.uidByte[i], HEX);
      if (i < mfrc522.uid.size - 1)
        Serial.print(F(" : "));
    }
    Serial.println();
  }

  Serial.print(F("Lifetime Total Racks: "));
  Serial.println(sessionRackCount);

  // Move Servo to 90 degrees (Unlocked / Open)
  lockServo.attach(SERVO_PIN);
  Serial.print(F("[SERVO] Opening to "));
  Serial.print(SERVO_UNLOCKED_ANGLE);
  Serial.println(F(" degrees... (Access Open)"));
  lockServo.write(SERVO_UNLOCKED_ANGLE);

  // Wait 3 seconds
  Serial.print(F("[SERVO] Holding open for "));
  Serial.print(UNLOCK_HOLD_MS / 1000);
  Serial.println(F(" seconds..."));
  delay(UNLOCK_HOLD_MS);

  // Return Servo to 0 degrees (Locked / Closed)
  Serial.print(F("[SERVO] Returning to "));
  Serial.print(SERVO_LOCKED_ANGLE);
  Serial.println(F(" degrees... (Locked)"));
  lockServo.write(SERVO_LOCKED_ANGLE);
  delay(500);         // Allow physical arm to reach 0 degrees
  lockServo.detach(); // Detach to prevent motor humming/heat and save power

  Serial.println(F("[SYSTEM] Cycle complete. Ready for next trigger."));
  Serial.println(F("---------------------------------------------"));

  stopCard();
  delay(500); // Debounce delay
}

void testServo() {
  Serial.println(F("\n[TEST] Testing SG90 Servo Motor..."));
  lockServo.attach(SERVO_PIN);
  Serial.print(F("[TEST] Rotating to "));
  Serial.print(SERVO_UNLOCKED_ANGLE);
  Serial.println(F(" degrees (Open)..."));
  lockServo.write(SERVO_UNLOCKED_ANGLE);

  delay(UNLOCK_HOLD_MS);

  Serial.print(F("[TEST] Rotating back to "));
  Serial.print(SERVO_LOCKED_ANGLE);
  Serial.println(F(" degrees (Closed)..."));
  lockServo.write(SERVO_LOCKED_ANGLE);
  delay(500);
  lockServo.detach();

  Serial.println(F("[TEST] Servo test successfully completed!"));
}

// ==================== 1. SCAN & IDENTIFY ====================
void scanAndIdentify() {
  if (!waitForCard())
    return;

  Serial.println(F("\n--- Card Identification ---"));

  // Card UID
  Serial.print(F("UID (Hex):     "));
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10)
      Serial.print(F("0"));
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1)
      Serial.print(F(" : "));
  }
  Serial.println();

  // Decimal UID format
  unsigned long decimalUid = 0;
  for (byte i = 0; i < ((mfrc522.uid.size < 4) ? mfrc522.uid.size : 4); i++) {
    decimalUid = (decimalUid << 8) | mfrc522.uid.uidByte[i];
  }
  Serial.print(F("UID (Decimal): "));
  Serial.println(decimalUid);

  // SAK & Card Type
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.print(F("Card Type:     "));
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  stopCard();
}

// ==================== 2. READ DATA BLOCK ====================
void readDataBlock() {
  Serial.print(F("Enter block number to read (e.g. 4): "));
  String blockStr = readLineFromSerial();
  byte blockAddr = blockStr.toInt();
  if (blockAddr == 0 && blockStr != "0")
    blockAddr = 4; // Default to block 4

  if (!waitForCard())
    return;

  // Authenticate using Key A
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
      MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    stopCard();
    return;
  }

  byte buffer[18];
  byte size = sizeof(buffer);

  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Read failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    stopCard();
    return;
  }

  Serial.println(F("\n--- Block Data ---"));
  Serial.print(F("Block "));
  Serial.print(blockAddr);
  Serial.print(F(" [Hex]:  "));
  for (byte i = 0; i < 16; i++) {
    if (buffer[i] < 0x10)
      Serial.print(F("0"));
    Serial.print(buffer[i], HEX);
    Serial.print(F(" "));
  }
  Serial.println();

  Serial.print(F("Block "));
  Serial.print(blockAddr);
  Serial.print(F(" [Text]: "));
  for (byte i = 0; i < 16; i++) {
    if (buffer[i] >= 32 && buffer[i] <= 126) {
      Serial.write(buffer[i]);
    } else {
      Serial.print(F("."));
    }
  }
  Serial.println();

  stopCard();
}

// ==================== 3. WRITE TEXT TO BLOCK ====================
void writeTextData() {
  Serial.print(F("Enter block number to write (e.g. 4): "));
  String blockStr = readLineFromSerial();
  byte blockAddr = blockStr.toInt();
  if (blockAddr == 0 && blockStr != "0")
    blockAddr = 4;

  // Protect Sector Trailer blocks (block 3, 7, 11, 15, etc.) and manufacturer
  // block 0
  if ((blockAddr + 1) % 4 == 0 || blockAddr == 0) {
    Serial.println(F("WARNING: Block 0 or Sector Trailer blocks (3, 7, 11...) "
                     "should not be overwritten. Aborted."));
    return;
  }

  Serial.print(F("Enter text string to write (up to 16 chars): "));
  String text = readLineFromSerial();

  byte dataBlock[16];
  for (byte i = 0; i < 16; i++) {
    if (i < text.length()) {
      dataBlock[i] = text[i];
    } else {
      dataBlock[i] = ' '; // Pad with spaces
    }
  }

  if (!waitForCard())
    return;

  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
      MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    stopCard();
    return;
  }

  status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Write failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  } else {
    Serial.println(F("[SUCCESS] Block written successfully!"));
  }

  stopCard();
}

// ==================== 4. WRITE NUMERIC VALUE / TOKEN ====================
void writeNumericValue() {
  Serial.print(F("Enter token/amount to store (e.g. 1000): "));
  String valStr = readLineFromSerial();
  long value = valStr.toInt();

  byte blockAddr = 4; // Use block 4 by default for value storage

  if (!waitForCard())
    return;

  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
      MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    stopCard();
    return;
  }

  // Value block format: 4-byte value, 4-byte inverted value, 4-byte value,
  // 1-byte addr, 1-byte ~addr, 1-byte addr, 1-byte ~addr
  byte valueBlock[16];
  valueBlock[0] = (value) & 0xFF;
  valueBlock[1] = (value >> 8) & 0xFF;
  valueBlock[2] = (value >> 16) & 0xFF;
  valueBlock[3] = (value >> 24) & 0xFF;

  valueBlock[4] = ~valueBlock[0];
  valueBlock[5] = ~valueBlock[1];
  valueBlock[6] = ~valueBlock[2];
  valueBlock[7] = ~valueBlock[3];

  valueBlock[8] = valueBlock[0];
  valueBlock[9] = valueBlock[1];
  valueBlock[10] = valueBlock[2];
  valueBlock[11] = valueBlock[3];

  valueBlock[12] = blockAddr;
  valueBlock[13] = ~blockAddr;
  valueBlock[14] = blockAddr;
  valueBlock[15] = ~blockAddr;

  status = mfrc522.MIFARE_Write(blockAddr, valueBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Failed to write value: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  } else {
    Serial.print(F("[SUCCESS] Stored numeric value: "));
    Serial.println(value);
  }

  stopCard();
}

// ==================== 5. FULL SECTOR DUMP ====================
void dumpFullCard() {
  if (!waitForCard())
    return;

  Serial.println(F("\n--- Dumping Card Memory ---"));
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

  stopCard();
}

// ==================== SERIAL INPUT HELPER ====================
String readLineFromSerial() {
  while (Serial.available())
    Serial.read(); // Clear buffer

  String line = "";
  unsigned long start = millis();
  while (millis() - start < 15000) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '\n' || c == '\r') {
        if (line.length() > 0)
          break;
      } else {
        line += c;
        Serial.print(c);
      }
    }
  }
  Serial.println();
  line.trim();
  return line;
}
