#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <Update.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <esp_system.h>
#include <time.h>
#include <sys/time.h>
#include <WiFiUdp.h>
#include "web_pages.h"

#define FIRMWARE_VERSION "Smarthive Pool V2.4-ESP32-Adrenaline"

// ==================== FORWARD DECLARATIONS ====================
uint32_t getChipId();
String getResetReasonStr();
void beepFeedback(int count, int onMs, int offMs);
void initHardwareIdentity();
uint32_t computeClaimPin(uint32_t chipId);
void handleDashboard();
void handleAPI();
void handleConfig();
void handleScan();
void handleSettings();
void handleSave();
void handleRestart();
void handleFactoryReset();
void handleNotFound();
void handleCardBlock();
void handleCardUnblock();
void handleBlockedCardsList();
bool isCardBlocked(const String& uid);
bool blockCard(const String& uid);
bool unblockCard(const String& uid);
void loadBlockedCards();
void saveBlockedCards();
void handleCardWrite();
void handleCardWriteStatus();
void handleCardWriteCancel();
bool writeStaffDataToCard(const char* name, const char* id, String& errOut);
void readStaffDataFromCard(String& outName, String& outId);
void handleReportsAPI();
void handleReportsClear();
void loadDailyRecords();
void saveDailyRecords();
void pruneOldReports();
void recordRackCount();
void getCurrentDateTime(char* dateBuf, size_t dateBufSize, char* timeBuf, size_t timeBufSize);
void loadCredentials();
void saveCredentials();
bool isSoftAPClient();
bool requireAuth();
String escapeJson(const String& str);
void setRGB(bool red, bool green, bool blue = false);
void updateLED();
void updateNFC();
bool isCardPresent();
void updateServo();
void triggerServoDoorLock();
void loadServoAngles();
void saveServoAngles();
void handleServoConfig();
void setServoInFlight(bool active);
void appendDeviceLog(const char* level, const char* tag, const String& msg);
void checkServoBrownoutOnBoot();
void handleDeviceLogs();
void handleDeviceLogsClear();
void sendCloudEvent(const char* eventType, const char* cardUid = "");
void handleCounterReset();
void handleKeepAlive();
void updateKeepAlive();
void loadSavedWiFi();
void saveSavedWiFi();
void loadBootCount();
void processCloudSyncResponse(const String& json);
void performCloudOtaIfPending();
bool addSavedWiFi(const char* ssid, const char* pass);
bool removeSavedWiFi(const char* ssid);
void connectWiFiWithoutReboot(const char* ssid, const char* pass);
void updateWiFiReconnect();
void handleWifiConnect();
void handleWifiForget();
void handleSavedWifiList();
void loadRackCounter();
void saveRackCounter();
void loadOfflineQueue();
void saveOfflineQueue();
void loadAuthorizedCards();
void saveAuthorizedCards();

// ==================== EEPROM LAYOUT (EXPANDED TO 4096 BYTES) ====================
#define EEPROM_SIZE       4096
#define ADDR_SSID         0    // 33 bytes (max 32 chars + null)
#define ADDR_PASS         33   // 65 bytes (max 64 chars + null, standard WPA2)
#define ADDR_GUI_USER     98   // 17 bytes (max 16 chars + null)
#define ADDR_GUI_PASS     115  // 17 bytes (max 16 chars + null)
#define ADDR_FLAG         132  // 1 byte
#define INIT_FLAG         0xB1 // Migration flag for expanded credential schema
#define ADDR_KEEPALIVE    470  // 1 byte (1 = enabled, 0 = disabled)
#define ADDR_DEVICE_ID    472  // 32 bytes (Adrenaline cloud table binding, e.g. "snooker_table_1")

// Persisted Servo Calibration Angles (510..512) - Survives reboots
#define ADDR_SERVO_FLAG   510 // 1 byte
#define SERVO_FLAG_VAL    0x5A
#define ADDR_SERVO_START  511 // 1 byte uint8_t (0..180, starting/locked angle)
#define ADDR_SERVO_END    512 // 1 byte uint8_t (0..180, ending/unlocked angle)
#define ADDR_SERVO_IN_FLIGHT 513 // 1 byte (0xA5 while servo sweep active; detects mid-sweep brownout/reboot)
#define SERVO_IN_FLIGHT_VAL  0xA5
#define ADDR_SERVO_HOLD_FLAG 514 // 1 byte
#define SERVO_HOLD_FLAG_VAL  0x5B
#define ADDR_SERVO_HOLD_SEC  515 // 1 byte uint8_t (2..10 seconds, gate open hold time)

// Multi-WiFi Memory Block (710..1105) - Remembers previously connected networks
#define ADDR_SAVED_WIFI_FLAG  710 // 1 byte
#define SAVED_WIFI_FLAG_VAL   0xE5
#define ADDR_SAVED_WIFI_COUNT 711 // 1 byte uint8_t (0..MAX_SAVED_WIFI)
#define ADDR_SAVED_WIFI_START 712 // 4 entries * 98 bytes = 392 bytes (712..1103)
#define MAX_SAVED_WIFI        4

// Persisted Session Rack Counter (1110..1114) - Survives power bank disconnect/reboot
#define ADDR_RACK_COUNTER_FLAG 1110 // 1 byte
#define RACK_COUNTER_FLAG_VAL  0xA3
#define ADDR_RACK_COUNTER_VAL  1111 // 4 bytes (uint32_t)

// Persisted Boot Counter (1115..1119) - Used for unique idempotent client_event_id
#define ADDR_BOOT_COUNT_FLAG   1115 // 1 byte
#define BOOT_COUNT_FLAG_VAL    0xB7
#define ADDR_BOOT_COUNT_VAL    1116 // 4 bytes (uint32_t)

// Persisted Offline Event Spooling Queue (1120..3041) - Up to 12 offline games * 160 bytes
#define ADDR_OFFLINE_Q_FLAG    1120 // 1 byte
#define OFFLINE_Q_FLAG_VAL     0xD4
#define ADDR_OFFLINE_Q_COUNT   1121 // 1 byte
#define ADDR_OFFLINE_Q_START   1122 // 12 * 160 = 1920 bytes (1122..3041)
#define MAX_PERSISTED_OFFLINE_Q 12

// Persisted Authorized Cards Cache (3080..4033) - Up to 14 cards * 68 bytes
#define ADDR_AUTH_CARDS_FLAG   3080 // 1 byte
#define AUTH_CARDS_FLAG_VAL    0xF2
#define ADDR_AUTH_CARDS_COUNT  3081 // 1 byte
#define ADDR_AUTH_CARDS_START  3082 // 14 * 68 = 952 bytes (3082..4033)
#define MAX_PERSISTED_AUTH_CARDS 14

struct SavedWiFiNetwork {
  char ssid[33];
  char pass[65];
};

SavedWiFiNetwork savedNetworks[MAX_SAVED_WIFI];
uint8_t savedNetworkCount = 0;

// Snooker 48-Hour Daily Reports EEPROM block (133..275)
#define SNOOKER_FLAG_ADDR  133
#define SNOOKER_FLAG_VAL   0xC7
#define ADDR_HIST_COUNT    134 // 1 byte uint8_t
#define ADDR_HIST_START    135 // up to 5 entries * 28 bytes = 140 bytes (135..275)

#define MAX_DAILY_RECORDS 5

struct DailyRecord {
  uint32_t epoch;      // timestamp of entry (epoch or uptime seconds)
  char date[11];       // "YYYY-MM-DD\0" or "Day X\0"
  uint16_t racks;      // count for that day
  char lastTime[9];    // "HH:MM:SS\0"
};

DailyRecord dailyRecords[MAX_DAILY_RECORDS];
uint8_t dailyRecordCount = 0;

// Blocked Cards Registry EEPROM block (280..445)
#define ADDR_BLOCKED_FLAG  280
#define BLOCKED_FLAG_VAL   0xDA
#define ADDR_BLOCKED_COUNT 281 // 1 byte uint8_t
#define ADDR_BLOCKED_START 282 // up to 10 entries * 16 bytes = 160 bytes (282..442)
#define MAX_BLOCKED_CARDS  10

char blockedCards[MAX_BLOCKED_CARDS][16];
uint8_t blockedCardsCount = 0;

// ==================== DYNAMIC HARDWARE IDENTITY & CLOUD PAIRING ====================
char deviceSerial[24] = "SMP-000000";
char claimPinStr[8]   = "000000";
char deviceId[32]     = "snooker_table_1"; // Default Adrenaline fleet table binding
char apSSID[32]       = "Smarthive-000000";
const char* apPassword = "setup1234";

uint32_t computeClaimPin(uint32_t chipId) {
  uint32_t hash = chipId ^ 0x5a3c9e71;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = (hash >> 16) ^ hash;
  return 100000 + (hash % 900000);
}

uint32_t getChipId() {
  uint64_t mac = ESP.getEfuseMac();
  return (uint32_t)((mac >> 24) & 0xFFFFFF);
}

String getResetReasonStr() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXT_RESET";
    case ESP_RST_SW:        return "SW_RESET";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    default:                return "UNKNOWN";
  }
}

void initHardwareIdentity() {
  uint32_t chipId = getChipId();
  snprintf(deviceSerial, sizeof(deviceSerial), "SMP-%06X", chipId);
  // Default to standard Adrenaline cloud table identifier ("snooker_table_1")
  // Can be customized via local Settings portal to snooker_table_2, etc.
  if (strlen(deviceId) == 0 || strcmp(deviceId, "snooker_000000") == 0) {
    strncpy(deviceId, "snooker_table_1", sizeof(deviceId) - 1);
    deviceId[sizeof(deviceId) - 1] = '\0';
  }
  snprintf(apSSID, sizeof(apSSID), "Smarthive-%06X", chipId);

  uint32_t pin = computeClaimPin(chipId);
  snprintf(claimPinStr, sizeof(claimPinStr), "%06u", pin);
}

// ==================== MUTABLE CREDENTIALS (LOADED FROM EEPROM) ====================
char routerSSID[33]     = "";
char routerPassword[65] = "";
char guiUsername[17]    = "";
char guiPassword[17]    = "";

WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

unsigned long lastStatusUpdate = 0;
unsigned long lastLedToggle = 0;
bool ledState = HIGH;

// ==================== SNOOKER HARDWARE CONFIGURATION ====================
// 1. RC522 RFID / NFC Reader (Hardware VSPI on ESP32):
// -------------------------------------------------------------------------
// RC522 Pin  | ESP32 GPIO | Notes
// -----------|------------|------------------------------------------------
// 3.3V (VCC) | 3V3        | CAUTION: Connect to 3.3V ONLY! NOT 5V!
// RST        | GPIO 22    | Configurable Reset pin
// GND        | GND        | Common Ground
// MISO       | GPIO 19    | Hardware VSPI MISO
// MOSI       | GPIO 23    | Hardware VSPI MOSI
// SCK        | GPIO 18    | Hardware VSPI Clock
// SDA (SS)   | GPIO 5     | Hardware VSPI Chip Select
// -------------------------------------------------------------------------
#define RC522_SS_PIN   5  // GPIO 5 (VSPI SS / SDA)
#define RC522_RST_PIN  22 // GPIO 22 (Reset)

MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
byte rc522FirmwareVersion = 0x00;
bool rfidWritePending = false;
unsigned long rfidWriteArmTime = 0;
const unsigned long RFID_WRITE_TIMEOUT_MS = 65000; // 65s window to match Adrenaline Cloud arming duration
char pendingStaffName[17] = "";
char pendingStaffId[17] = "";
String rfidWriteStatus = "IDLE";
String rfidWriteMessage = "";
String lastWrittenUid = "";
String lastStaffName = "None";
String lastStaffId = "None";
byte activeCsPin = RC522_SS_PIN;
String lastScannedUid = "None";
String lastTriggerSource = "None";
unsigned long lastNfcScanTime = 0;
const unsigned long NFC_COOLDOWN_MS = 800; // 0.8s responsive cooldown between consecutive card taps

// 2. MG996R High-Torque Metal Gear Servo Door Lock:
// -------------------------------------------------------------------------
// Wire Connection:
// Orange / Yellow (PWM Signal) -> ESP32 GPIO 13 (Hardware LEDC PWM)
// Red (5V Power)               -> 5V High-Current Rail (from Power Bank / 2A+ Supply) [NEVER 3.3V!]
// Brown / Black (Ground)       -> Common GND
// IMPORTANT: Place a 470uF - 1000uF electrolytic capacitor across Servo 5V & GND
//            to buffer the 1.5A - 2.5A peak inrush current and prevent ESP brownout.
// -------------------------------------------------------------------------
#define SERVO_PIN            13   // GPIO 13 (Hardware PWM)
#define SERVO_LOCKED_ANGLE   180  // Default starting/locked angle (configurable via Web UI)
#define SERVO_UNLOCKED_ANGLE 0    // Default ending/unlocked angle (configurable via Web UI)
#define UNLOCK_HOLD_MS       3000 // How long to stay open (milliseconds)

int servoStartAngle = SERVO_LOCKED_ANGLE;
int servoEndAngle   = SERVO_UNLOCKED_ANGLE;
int servoHoldSec    = 4; // Configurable gate open hold time (2..10 seconds, default 4s)

Servo lockServo;
bool servoUnlocked = false;
bool servoDetached = true;

// Non-blocking smooth soft-start motion state machine
enum ServoMotionState {
  SERVO_IDLE_DETACHED,
  SERVO_PRE_ATTACH_DELAY, // Waits 120ms so HTTP/Wi-Fi TX finishes before MG996R attaches (anti-brownout)
  SERVO_SWEEPING_OPEN,
  SERVO_HOLDING_OPEN,
  SERVO_SWEEPING_CLOSE,
  SERVO_SETTLING_CLOSE
};

ServoMotionState servoState = SERVO_IDLE_DETACHED;
int currentServoAngle = SERVO_LOCKED_ANGLE;
unsigned long lastServoStepTime = 0;
unsigned long servoPreAttachStartTime = 0;
unsigned long servoHoldStartTime = 0;
unsigned long servoCloseSettleStartTime = 0;
unsigned long lastServoDetachMs = 0;

// Built-in Multi-Rack Stress Test Runner (runs autonomously on ESP32)
bool stressTestActive = false;
int stressTestTotal = 0;
int stressTestDone = 0;
int stressTestGapSec = 3;
unsigned long stressTestNextMs = 0;

// 3. Status Indicator (Full RGB LED Module + Active Piezo Buzzer):
// -------------------------------------------------------------------------
// Provides clear visual feedback on the table cabinet:
//   - Green Heartbeat: Online / Table Ready
//   - Rapid Green Blink (5Hz): Card Authorized / Ball Gate Open
//   - Cyan Pulse (Blue+Green): Multi-Rack Stress Test / Cloud OTA Active
//   - Amber Pulse (Red+Green): Reader Armed in Tap-to-Program Mode
//   - Blue Blink: Offline / SoftAP / Wi-Fi Connecting
//   - Red Flash / Strobe: Unauthorized or Blacklisted Card Rejected
//
// Wiring for Standard RGB Module (Common Cathode, HW-479) + Active Buzzer:
//   RGB Pin R (Red)   -> ESP32 GPIO 26
//   RGB Pin G (Green) -> ESP32 GPIO 27
//   RGB Pin B (Blue)  -> ESP32 GPIO 14
//   RGB Pin - (GND)   -> ESP32 GND
//   Active Buzzer (+) -> ESP32 GPIO 25  (GND -> Common GND)
// -------------------------------------------------------------------------
#define LED_RED_PIN   26 // GPIO 26 (Red Status Channel)
#define LED_GREEN_PIN 27 // GPIO 27 (Green Status Channel)
#define LED_BLUE_PIN  14 // GPIO 14 (Blue Status Channel)
#define BUZZER_PIN    25 // GPIO 25 (Active Piezo Buzzer - HIGH = ON)
#define RGB_COMMON_CATHODE 1 // 1 = Common Cathode (GND pin, HIGH = ON); 0 = Common Anode (VCC pin, LOW = ON)

#ifndef LED_BUILTIN
#define LED_BUILTIN   2  // Standard onboard blue LED on ESP32 DevKit (GPIO 2)
#endif

volatile unsigned long sessionRackCount = 0;
bool keepAliveEnabled = false;

// ==================== ADRENALINE CLOUD SYNC CONFIGURATION ====================
// Change to your public live domain, tunnel URL, or LAN IP (e.g. "http://192.168.5.6:3000/api/snooker/sync")
char adrenalineCloudUrl[160] = "https://adrenalinerecreationhub.com/api/snooker/sync";
// char adrenalineCloudUrl[160] = "http://192.168.5.6:3000/api/snooker/sync";

const char* adrenalineDeviceToken = "adrenaline_snooker_secret_2026";
bool adrenalineSyncPending = false;
unsigned long lastCloudSyncAttempt = 0;
const unsigned long CLOUD_HEARTBEAT_INTERVAL = 8000; // 8s heartbeat interval (responsive for rapid Adrenaline cloud card arming)

// Anti-jiggle / double-trigger refractory lockout
unsigned long lastRackTriggerTime = 0;
#define RACK_LOCKOUT_MS 20000 // 20-second refractory lockout between games to prevent cheating/flicking

// Offline event spooling queue
struct QueuedRackEvent {
  char event[20];
  char trigger[16];
  char cardUid[24];
  char staffName[24];
  char staffId[24];
  char date[12];
  char time[10];
  char clientEventId[24];
  uint32_t queuedUptimeSec;
};

uint32_t bootCount = 1;
uint32_t eventSeq = 0;
bool littleFsReady = false;
int lastCloudHttpCode = 0;
String lastCloudStatus = "IDLE";
String cloudTableName = "";
long cloudPricePerGame = 500;
String cloudCurrency = "NGN";
bool cloudMaintenanceLocked = false;
bool cloudOtaPending = false;
String cloudOtaTargetVersion = "";
String cloudOtaUrl = "";

#define MAX_OFFLINE_QUEUE 30
QueuedRackEvent offlineQueue[MAX_OFFLINE_QUEUE];
int offlineQueueCount = 0;
void queueOfflineEvent(const char* eventType, const char* cardUid);
void flushOfflineQueue();

// Authorized RFID Cards whitelist cache
struct AuthorizedCardCache {
  char uid[16];
  char name[24];
  char id[16];
  char role[12];
};

#define MAX_AUTHORIZED_CARDS 30
AuthorizedCardCache authorizedCardsList[MAX_AUTHORIZED_CARDS];
int authorizedCardsCount = 0;
bool isCardAuthorized(const String& uid, String& outName, String& outId);
void updateAuthorizedCardsFromJson(const String& json);
void updateRfidWriteCmdFromJson(const String& json);

// ==================== STRING ESCAPING HELPER ====================
String escapeJson(const String& str) {
  String out = "";
  out.reserve(str.length() + 12);
  for (size_t i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"':  out += "\\\""; break;
      case '\b': out += "\\b"; break;
      case '\f': out += "\\f"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:   out += c; break;
    }
  }
  return out;
}

// ==================== EEPROM HELPERS ====================
void loadCredentials() {
  EEPROM.begin(EEPROM_SIZE);
  byte flag = EEPROM.read(ADDR_FLAG);

  if (flag == INIT_FLAG) {
    for (int i = 0; i < 32; i++) routerSSID[i]     = EEPROM.read(ADDR_SSID + i);
    routerSSID[32] = '\0';
    for (int i = 0; i < 64; i++) routerPassword[i] = EEPROM.read(ADDR_PASS + i);
    routerPassword[64] = '\0';
    for (int i = 0; i < 16; i++) guiUsername[i]    = EEPROM.read(ADDR_GUI_USER + i);
    guiUsername[16] = '\0';
    for (int i = 0; i < 16; i++) guiPassword[i]    = EEPROM.read(ADDR_GUI_PASS + i);
    guiPassword[16] = '\0';
    Serial.println(F("[EEPROM] Credentials loaded successfully"));
  } else {
    // Initial configuration: prompt setup via AP without personal defaults
    routerSSID[0] = '\0';
    routerPassword[0] = '\0';
    strcpy(guiUsername, "admin");
    strcpy(guiPassword, "admin");
    saveCredentials();
    Serial.println(F("[EEPROM] First boot: initialized default admin credentials"));
  }

  byte ka = EEPROM.read(ADDR_KEEPALIVE);
  if (ka == 1) {
    keepAliveEnabled = true;
  } else if (ka == 0) {
    keepAliveEnabled = false;
  } else {
    keepAliveEnabled = true; // Default enabled to support power banks out-of-the-box
    EEPROM.write(ADDR_KEEPALIVE, 1);
    EEPROM.commit();
  }
  Serial.printf("[KEEPALIVE] Power bank keep-alive initialized: %s\n", keepAliveEnabled ? "ENABLED" : "DISABLED");

  // Load configured Adrenaline Table Node ID (Default: "snooker_table_1")
  char storedDevId[32];
  for (int i = 0; i < 31; i++) storedDevId[i] = EEPROM.read(ADDR_DEVICE_ID + i);
  storedDevId[31] = '\0';
  if (strlen(storedDevId) > 0 && (byte)storedDevId[0] != 0xFF && isPrintable(storedDevId[0])) {
    strncpy(deviceId, storedDevId, sizeof(deviceId) - 1);
    deviceId[sizeof(deviceId) - 1] = '\0';
  } else {
    strncpy(deviceId, "snooker_table_1", sizeof(deviceId) - 1);
    deviceId[sizeof(deviceId) - 1] = '\0';
  }
  Serial.printf("[ADRENALINE] Bound to Cloud Table Node ID: %s\n", deviceId);
  loadSavedWiFi();
}

void saveCredentials() {
  for (int i = 0; i < 32; i++) EEPROM.write(ADDR_SSID + i, routerSSID[i]);
  EEPROM.write(ADDR_SSID + 32, 0);

  for (int i = 0; i < 64; i++) EEPROM.write(ADDR_PASS + i, routerPassword[i]);
  EEPROM.write(ADDR_PASS + 64, 0);

  for (int i = 0; i < 16; i++) EEPROM.write(ADDR_GUI_USER + i, guiUsername[i]);
  EEPROM.write(ADDR_GUI_USER + 16, 0);

  for (int i = 0; i < 16; i++) EEPROM.write(ADDR_GUI_PASS + i, guiPassword[i]);
  EEPROM.write(ADDR_GUI_PASS + 16, 0);

  EEPROM.write(ADDR_FLAG, INIT_FLAG);
  EEPROM.commit();
}

// ==================== MULTI-WIFI REGISTRY & PERSISTENCE ====================
void loadSavedWiFi() {
  byte flag = EEPROM.read(ADDR_SAVED_WIFI_FLAG);
  if (flag == SAVED_WIFI_FLAG_VAL) {
    savedNetworkCount = EEPROM.read(ADDR_SAVED_WIFI_COUNT);
    if (savedNetworkCount > MAX_SAVED_WIFI) savedNetworkCount = 0;

    for (uint8_t i = 0; i < savedNetworkCount; i++) {
      int base = ADDR_SAVED_WIFI_START + (i * 98);
      for (int c = 0; c < 32; c++) savedNetworks[i].ssid[c] = EEPROM.read(base + c);
      savedNetworks[i].ssid[32] = '\0';
      for (int c = 0; c < 64; c++) savedNetworks[i].pass[c] = EEPROM.read(base + 33 + c);
      savedNetworks[i].pass[64] = '\0';
    }
    Serial.printf("[WIFI-MEM] Loaded %d saved WiFi networks from flash memory\n", savedNetworkCount);
  } else {
    savedNetworkCount = 0;
    // Migrate active legacy routerSSID to index 0 if present
    if (strlen(routerSSID) > 0) {
      strncpy(savedNetworks[0].ssid, routerSSID, 32);
      savedNetworks[0].ssid[32] = '\0';
      strncpy(savedNetworks[0].pass, routerPassword, 64);
      savedNetworks[0].pass[64] = '\0';
      savedNetworkCount = 1;
      saveSavedWiFi();
      Serial.printf("[WIFI-MEM] Initialized saved WiFi history with '%s'\n", routerSSID);
    }
  }
}

void saveSavedWiFi() {
  EEPROM.write(ADDR_SAVED_WIFI_FLAG, SAVED_WIFI_FLAG_VAL);
  EEPROM.write(ADDR_SAVED_WIFI_COUNT, savedNetworkCount);

  for (uint8_t i = 0; i < MAX_SAVED_WIFI; i++) {
    int base = ADDR_SAVED_WIFI_START + (i * 98);
    if (i < savedNetworkCount) {
      for (int c = 0; c < 32; c++) EEPROM.write(base + c, savedNetworks[i].ssid[c]);
      EEPROM.write(base + 32, 0);
      for (int c = 0; c < 64; c++) EEPROM.write(base + 33 + c, savedNetworks[i].pass[c]);
      EEPROM.write(base + 97, 0);
    } else {
      for (int c = 0; c < 98; c++) EEPROM.write(base + c, 0);
    }
  }
  EEPROM.commit();
}

bool addSavedWiFi(const char* ssid, const char* pass) {
  if (!ssid || strlen(ssid) == 0) return false;

  // Check if network is already in the list
  for (uint8_t i = 0; i < savedNetworkCount; i++) {
    if (strcmp(savedNetworks[i].ssid, ssid) == 0) {
      if (pass && strlen(pass) > 0) {
        strncpy(savedNetworks[i].pass, pass, 64);
        savedNetworks[i].pass[64] = '\0';
      }
      // Move to position 0 (most recent / highest priority)
      if (i > 0) {
        SavedWiFiNetwork temp = savedNetworks[i];
        for (int j = i; j > 0; j--) {
          savedNetworks[j] = savedNetworks[j - 1];
        }
        savedNetworks[0] = temp;
      }
      saveSavedWiFi();
      return true;
    }
  }

  // Shift existing entries down by 1
  uint8_t limit = (savedNetworkCount < MAX_SAVED_WIFI) ? savedNetworkCount : (MAX_SAVED_WIFI - 1);
  for (int j = limit; j > 0; j--) {
    savedNetworks[j] = savedNetworks[j - 1];
  }

  strncpy(savedNetworks[0].ssid, ssid, 32);
  savedNetworks[0].ssid[32] = '\0';
  if (pass) {
    strncpy(savedNetworks[0].pass, pass, 64);
    savedNetworks[0].pass[64] = '\0';
  } else {
    savedNetworks[0].pass[0] = '\0';
  }

  if (savedNetworkCount < MAX_SAVED_WIFI) {
    savedNetworkCount++;
  }

  saveSavedWiFi();
  Serial.printf("[WIFI-MEM] Saved network '%s' (total remembered: %d)\n", ssid, savedNetworkCount);
  return true;
}

bool removeSavedWiFi(const char* ssid) {
  if (!ssid) return false;
  int foundIdx = -1;
  for (uint8_t i = 0; i < savedNetworkCount; i++) {
    if (strcmp(savedNetworks[i].ssid, ssid) == 0) {
      foundIdx = i;
      break;
    }
  }

  if (foundIdx < 0) return false;

  for (uint8_t i = foundIdx; i < savedNetworkCount - 1; i++) {
    savedNetworks[i] = savedNetworks[i + 1];
  }
  savedNetworkCount--;
  saveSavedWiFi();
  Serial.printf("[WIFI-MEM] Removed '%s' from saved network registry\n", ssid);
  return true;
}

// ==================== LIVE WIFI SWITCHING & RECONNECT ENGINE ====================
bool wifiConnecting = false;
unsigned long wifiConnectAttemptStart = 0;
unsigned long lastWiFiScanTime = 0;
bool wifiScanActive = false;
bool lastWiFiConnectedStatus = false;

void connectWiFiWithoutReboot(const char* ssid, const char* pass) {
  if (!ssid || strlen(ssid) == 0) {
    WiFi.disconnect(false, false);
    routerSSID[0] = '\0';
    routerPassword[0] = '\0';
    saveCredentials();
    return;
  }

  strncpy(routerSSID, ssid, 32);
  routerSSID[32] = '\0';
  if (pass) {
    strncpy(routerPassword, pass, 64);
    routerPassword[64] = '\0';
  }

  saveCredentials();
  addSavedWiFi(routerSSID, routerPassword);

  Serial.printf("[WIFI] Connecting to '%s' in background without rebooting...\n", routerSSID);
  // false, false: Keeps SoftAP radio online so connected phones are NOT disconnected!
  WiFi.disconnect(false, false);
  WiFi.begin(routerSSID, routerPassword);
  wifiConnecting = true;
  wifiConnectAttemptStart = millis();
}

void updateWiFiReconnect() {
  bool currentlyConnected = (WiFi.status() == WL_CONNECTED);

  // 1. Connection established!
  if (currentlyConnected && !lastWiFiConnectedStatus) {
    lastWiFiConnectedStatus = true;
    wifiConnecting = false;
    wifiScanActive = false;
    Serial.printf("\n[WIFI] Connected to '%s'! IP: %s (RSSI: %d dBm) [Zero Reboots]\n",
                  WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI());
    appendDeviceLog("INFO", "WIFI", "Connected " + WiFi.SSID() + " IP=" + WiFi.localIP().toString() + " RSSI=" + String(WiFi.RSSI()) + "dBm");
    lastCloudSyncAttempt = millis() - 3000; // Wait ~2s for DHCP/ARP/DNS to settle before TLS sync

    // Sync active SSID with primary configuration
    if (WiFi.SSID().length() > 0 && String(routerSSID) != WiFi.SSID()) {
      strncpy(routerSSID, WiFi.SSID().c_str(), 32);
      routerSSID[32] = '\0';
      for (uint8_t s = 0; s < savedNetworkCount; s++) {
        if (strcmp(savedNetworks[s].ssid, routerSSID) == 0) {
          strncpy(routerPassword, savedNetworks[s].pass, 64);
          routerPassword[64] = '\0';
          break;
        }
      }
      saveCredentials();
    }
    return;
  }

  // 2. Connection dropped (e.g. router rebooted or power cut)!
  if (!currentlyConnected && lastWiFiConnectedStatus) {
    lastWiFiConnectedStatus = false;
    Serial.println(F("\n[WIFI] Connection dropped! Auto-reconnecting without reboot..."));
    appendDeviceLog("WARN", "WIFI", "Connection dropped; auto-reconnecting");
    if (strlen(routerSSID) > 0) {
      WiFi.disconnect(false, false); // Keep SoftAP alive
      WiFi.begin(routerSSID, routerPassword);
      wifiConnecting = true;
      wifiConnectAttemptStart = millis();
    }
    return;
  }

  // 3. If currently connected, nothing more to do
  if (currentlyConnected) return;

  // 4. Timeout check on active connection attempt (12s)
  if (wifiConnecting) {
    if (millis() - wifiConnectAttemptStart > 12000) {
      wifiConnecting = false;
      Serial.println(F("[WIFI] Initial attempt timed out. Scanning for known saved networks in the air..."));
    } else {
      return;
    }
  }

  // 5. Asynchronous background scan to detect rebooted router or alternate saved networks
  if (savedNetworkCount > 0 && !wifiScanActive && (millis() - lastWiFiScanTime > 15000)) {
    lastWiFiScanTime = millis();
    int res = WiFi.scanNetworks(true, true); // Non-blocking async scan
    if (res == WIFI_SCAN_RUNNING) {
      wifiScanActive = true;
    }
  }

  // 6. Process completed async scan results
  if (wifiScanActive) {
    int scanStatus = WiFi.scanComplete();
    if (scanStatus >= 0) {
      wifiScanActive = false;
      int bestSavedIdx = -1;
      int bestScannedRssi = -999;

      for (int i = 0; i < scanStatus; i++) {
        String scannedSsid = WiFi.SSID(i);
        int scannedRssi = WiFi.RSSI(i);

        for (uint8_t s = 0; s < savedNetworkCount; s++) {
          if (scannedSsid == savedNetworks[s].ssid) {
            if (scannedRssi > bestScannedRssi) {
              bestScannedRssi = scannedRssi;
              bestSavedIdx = s;
            }
            break;
          }
        }
      }

      if (bestSavedIdx >= 0) {
        Serial.printf("[WIFI] Auto-detected previously connected network '%s' (RSSI: %d dBm). Reconnecting...\n",
                      savedNetworks[bestSavedIdx].ssid, bestScannedRssi);
        strncpy(routerSSID, savedNetworks[bestSavedIdx].ssid, 32);
        routerSSID[32] = '\0';
        strncpy(routerPassword, savedNetworks[bestSavedIdx].pass, 64);
        routerPassword[64] = '\0';
        saveCredentials();

        WiFi.disconnect(false, false);
        WiFi.begin(routerSSID, routerPassword);
        wifiConnecting = true;
        wifiConnectAttemptStart = millis();
      }

      WiFi.scanDelete();
    } else if (scanStatus == WIFI_SCAN_FAILED) {
      wifiScanActive = false;
    }
  }
}

// ==================== AUTH GUARD (SESSION COOKIE + LOGIN PAGE) ====================
#define MAX_AUTH_SESSIONS 4
#define SESSION_TTL_MS    (12UL * 3600UL * 1000UL) // 12-hour session expiry

struct AuthSession {
  char token[17]; // 16 hex chars + null
  unsigned long lastSeen;
};

AuthSession authSessions[MAX_AUTH_SESSIONS];

void invalidateAllSessions() {
  for (int i = 0; i < MAX_AUTH_SESSIONS; i++) {
    authSessions[i].token[0] = '\0';
    authSessions[i].lastSeen = 0;
  }
}

String createAuthSession() {
  int slot = 0;
  unsigned long oldest = 0xFFFFFFFFUL;
  for (int i = 0; i < MAX_AUTH_SESSIONS; i++) {
    if (authSessions[i].token[0] == '\0') {
      slot = i;
      break;
    }
    if (authSessions[i].lastSeen < oldest) {
      oldest = authSessions[i].lastSeen;
      slot = i;
    }
  }
  uint32_t r1 = esp_random();
  uint32_t r2 = esp_random();
  snprintf(authSessions[slot].token, sizeof(authSessions[slot].token), "%08x%08x", r1, r2);
  authSessions[slot].lastSeen = millis();
  return String(authSessions[slot].token);
}

String extractSessionToken() {
  if (!server.hasHeader("Cookie")) return "";
  String cookie = server.header("Cookie");
  int idx = cookie.indexOf("SMP_SESSION=");
  if (idx < 0) return "";
  int start = idx + 12;
  int end = cookie.indexOf(';', start);
  String tok = (end < 0) ? cookie.substring(start) : cookie.substring(start, end);
  tok.trim();
  return tok;
}

bool isSessionValid() {
  String tok = extractSessionToken();
  if (tok.length() != 16) return false;
  unsigned long now = millis();
  for (int i = 0; i < MAX_AUTH_SESSIONS; i++) {
    if (authSessions[i].token[0] != '\0' && tok.equals(authSessions[i].token)) {
      if (now - authSessions[i].lastSeen < SESSION_TTL_MS) {
        authSessions[i].lastSeen = now;
        return true;
      } else {
        authSessions[i].token[0] = '\0';
        return false;
      }
    }
  }
  return false;
}

bool isSoftAPClient() {
  IPAddress clientIP = server.client().remoteIP();
  IPAddress apIP = WiFi.softAPIP();
  // Check if client is on the SoftAP subnet (192.168.4.x)
  return (clientIP[0] == apIP[0] && clientIP[1] == apIP[1] && clientIP[2] == apIP[2]);
}

bool requireAuth() {
  if (isSessionValid()) {
    return true;
  }
  // Allow HTTP Basic Auth fallback for API/automation clients (e.g. curl / Apple Shortcuts)
  if (server.hasHeader("Authorization") && server.authenticate(guiUsername, guiPassword)) {
    return true;
  }
  if (server.uri().startsWith("/api/")) {
    server.send(401, "application/json", "{\"error\":\"unauthorized\",\"message\":\"Login required\"}");
  } else {
    server.sendHeader("Location", "/login", true);
    server.send(302, "text/plain", "");
  }
  return false;
}

void handleLoginPage() {
  if (isSessionValid()) {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send_P(200, "text/html", PAGE_LOGIN);
}

void handleLoginPost() {
  String user = server.arg("username");
  String pass = server.arg("password");
  user.trim();
  if (user.equals(guiUsername) && pass.equals(guiPassword)) {
    String tok = createAuthSession();
    server.sendHeader("Set-Cookie", "SMP_SESSION=" + tok + "; Path=/; HttpOnly; SameSite=Lax");
    server.send(200, "application/json", "{\"ok\":true}");
    Serial.printf("[AUTH] Admin '%s' signed in from %s\n", user.c_str(), server.client().remoteIP().toString().c_str());
  } else {
    Serial.printf("[AUTH] Failed login attempt for '%s' from %s\n", user.c_str(), server.client().remoteIP().toString().c_str());
    server.send(401, "application/json", "{\"ok\":false,\"message\":\"Invalid username or password.\"}");
  }
}

void handleLogout() {
  String tok = extractSessionToken();
  for (int i = 0; i < MAX_AUTH_SESSIONS; i++) {
    if (tok.length() == 16 && tok.equals(authSessions[i].token)) {
      authSessions[i].token[0] = '\0';
      authSessions[i].lastSeen = 0;
    }
  }
  server.sendHeader("Set-Cookie", "SMP_SESSION=deleted; Path=/; Max-Age=0; HttpOnly; SameSite=Lax");
  if (server.uri().startsWith("/api/")) {
    server.send(200, "application/json", "{\"ok\":true}");
  } else {
    server.sendHeader("Location", "/login", true);
    server.send(302, "text/plain", "");
  }
}

// ==================== HARDWARE RGB LED STATUS FEEDBACK ====================
void beepFeedback(int count, int onMs, int offMs) {
  for (int i = 0; i < count; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < count - 1 && offMs > 0) delay(offMs);
  }
}

void setRGB(bool red, bool green, bool blue) {
#if RGB_COMMON_CATHODE
  digitalWrite(LED_RED_PIN,   red   ? HIGH : LOW);
  digitalWrite(LED_GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(LED_BLUE_PIN,  blue  ? HIGH : LOW);
#else
  digitalWrite(LED_RED_PIN,   red   ? LOW : HIGH);
  digitalWrite(LED_GREEN_PIN, green ? LOW : HIGH);
  digitalWrite(LED_BLUE_PIN,  blue  ? LOW : HIGH);
#endif
}

void updateLED() {
  unsigned long now = millis();

  // 1. Table Unlocked / Authorized Card Accepted: RAPID GREEN BLINK!
  // Blinks green rapidly (100ms on / 100ms off) while the servo unlocks, holds the ball gate open, and returns
  if (servoUnlocked || servoState != SERVO_IDLE_DETACHED) {
    bool rapidGreen = ((now / 100) % 2 == 0); // 5Hz rapid green blink
    setRGB(false, rapidGreen, false);
    return;
  }

  // 2. Multi-Rack Stress Test / Cloud OTA Active: Cyan pulse (Green + Blue)
  if (stressTestActive || cloudOtaPending) {
    bool cyanPulse = ((now / 200) % 2 == 0);
    setRGB(false, cyanPulse, cyanPulse);
    return;
  }

  // 3. Reader Armed in Programming Mode: Amber / Yellow pulse (Red + Green)
  if (rfidWritePending) {
    bool armedPulse = ((now / 250) % 2 == 0); // 4Hz amber pulse
    setRGB(armedPulse, armedPulse, false);
    return;
  }
  
  // 4. Online Heartbeat (Connected to Router): Green pulse every 2.5s
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long phase = now % 2500;
    setRGB(false, phase < 60, false);
    return;
  }
  
  // 5. Offline / SoftAP / Wi-Fi Connecting Mode: Gentle Blue blink (reserves Red strictly for errors/denials)
  unsigned long interval = (strlen(routerSSID) > 0) ? 500 : 800;
  if (now - lastLedToggle >= interval) {
    lastLedToggle = now;
    ledState = !ledState;
  }
  setRGB(false, false, ledState);
}

// ==================== CAPTIVE PORTAL HANDLER ====================
void handleNotFound() {
  String host = server.hostHeader();
  String apIp = WiFi.softAPIP().toString();

  // If request is not targeted directly at our AP IP or mDNS hostname, redirect to captive portal landing
  if (host != apIp && host != "nodemcu.local" && host != (apIp + ":80") && host != "nodemcu.local:80") {
    server.sendHeader("Location", String("http://") + apIp + "/", true);
    server.send(302, "text/plain", "");
    return;
  }

  // If client is on SoftAP and requests an unknown page, redirect to root dashboard
  if (isSoftAPClient() && server.uri() != "/favicon.ico") {
    server.sendHeader("Location", String("http://") + apIp + "/", true);
    server.send(302, "text/plain", "");
    return;
  }

  server.send(404, "text/plain", "Not Found");
}

// Web UI pages are stored in web_pages.h to ensure clean compilation and prevent Arduino preprocessor conflicts

// ==================== HTTP ENDPOINT HANDLERS ====================
void handleDashboard() {
  server.send_P(200, "text/html", PAGE_DASHBOARD);
}

void handleSettings() {
  server.send_P(200, "text/html", PAGE_DASHBOARD);
}

void handleAPI() {
  String json = "{";
  json.reserve(1024);
  json += "\"serial_number\":\"" + escapeJson(String(deviceSerial)) + "\",";
  json += "\"claim_pin\":\"" + escapeJson(String(claimPinStr)) + "\",";
  json += "\"device_id\":\"" + escapeJson(String(deviceId)) + "\",";
  json += "\"ap_ssid\":\"" + escapeJson(String(apSSID)) + "\",";
  json += "\"ap_ip\":\"" + escapeJson(WiFi.softAPIP().toString()) + "\",";
  json += "\"ap_mac\":\"" + escapeJson(WiFi.softAPmacAddress()) + "\",";
  json += "\"ap_clients\":" + String(WiFi.softAPgetStationNum()) + ",";
  json += "\"sta_ssid\":\"" + escapeJson(String(routerSSID)) + "\",";
  json += "\"sta_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"sta_connecting\":" + String(wifiConnecting ? "true" : "false") + ",";
  json += "\"sta_ip\":\"" + escapeJson(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0") + "\",";
  json += "\"sta_gateway\":\"" + escapeJson(WiFi.status() == WL_CONNECTED ? WiFi.gatewayIP().toString() : "0.0.0.0") + "\",";
  json += "\"sta_mac\":\"" + escapeJson(WiFi.macAddress()) + "\",";
  json += "\"sta_rssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0) + ",";
  json += "\"saved_wifi_count\":" + String(savedNetworkCount) + ",";
  json += "\"saved_networks\":[";
  for (uint8_t s = 0; s < savedNetworkCount; s++) {
    if (s > 0) json += ",";
    json += "{\"ssid\":\"" + escapeJson(String(savedNetworks[s].ssid)) + "\",";
    json += "\"is_active\":" + String((WiFi.status() == WL_CONNECTED && String(savedNetworks[s].ssid) == WiFi.SSID()) ? "true" : "false") + "}";
  }
  json += "],";
  json += "\"client_ip\":\"" + escapeJson(server.client().remoteIP().toString()) + "\",";
  json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"heap_frag\":" + String(0) + ",";
  json += "\"uptime_secs\":" + String(millis() / 1000) + ",";
  json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"chip_id\":\"" + String(getChipId(), HEX) + "\",";
  json += "\"game_count\":" + String(sessionRackCount) + ",";
  json += "\"button_count\":" + String(sessionRackCount) + ",";
  json += "\"offline_queue_count\":" + String(offlineQueueCount) + ",";
  json += "\"authorized_cards_count\":" + String(authorizedCardsCount) + ",";
  json += "\"table_name\":\"" + escapeJson(cloudTableName) + "\",";
  json += "\"price_per_game\":" + String(cloudPricePerGame) + ",";
  json += "\"currency\":\"" + escapeJson(cloudCurrency) + "\",";
  json += "\"maintenance_locked\":" + String(cloudMaintenanceLocked ? "true" : "false") + ",";
  json += "\"boot_count\":" + String(bootCount) + ",";
  json += "\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\",";
  json += "\"last_cloud_code\":" + String(lastCloudHttpCode) + ",";
  json += "\"last_cloud_status\":\"" + escapeJson(lastCloudStatus) + "\",";
  json += "\"last_trigger\":\"" + escapeJson(lastTriggerSource) + "\",";
  json += "\"last_card_uid\":\"" + escapeJson(lastScannedUid) + "\",";
  json += "\"staff_name\":\"" + escapeJson(lastStaffName) + "\",";
  json += "\"staff_id\":\"" + escapeJson(lastStaffId) + "\",";
  json += "\"rfid_write_status\":\"" + escapeJson(rfidWriteStatus) + "\",";
  json += "\"rc522_version\":\"0x" + String(rc522FirmwareVersion, HEX) + "\",";
  json += "\"rc522_connected\":" + String((rc522FirmwareVersion != 0x00 && rc522FirmwareVersion != 0xFF) ? "true" : "false") + ",";
  json += "\"servo_state\":\"" + String(servoUnlocked ? "UNLOCKED" : "LOCKED") + "\",";
  json += "\"servo_start_angle\":" + String(servoStartAngle) + ",";
  json += "\"servo_end_angle\":" + String(servoEndAngle) + ",";
  json += "\"servo_hold_sec\":" + String(servoHoldSec) + ",";
  json += "\"stress_active\":" + String(stressTestActive ? "true" : "false") + ",";
  json += "\"stress_done\":" + String(stressTestDone) + ",";
  json += "\"stress_total\":" + String(stressTestTotal) + ",";
  json += "\"keepalive_enabled\":" + String(keepAliveEnabled ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

// ==================== 48-HOUR DAILY RACK REPORTS PERSISTENCE & PRUNING ====================
void getCurrentDateTime(char* dateBuf, size_t dateBufSize, char* timeBuf, size_t timeBufSize) {
  time_t now = time(nullptr);
  if (now > 1700000000) { // Valid NTP epoch time (post-2023)
    struct tm* tm_info = localtime(&now);
    strftime(dateBuf, dateBufSize, "%Y-%m-%d", tm_info);
    strftime(timeBuf, timeBufSize, "%H:%M:%S", tm_info);
  } else {
    // Relative uptime fallback if offline/no NTP
    unsigned long secs = millis() / 1000;
    unsigned long days = secs / 86400;
    snprintf(dateBuf, dateBufSize, "Day %lu", days + 1);
    unsigned long rem = secs % 86400;
    unsigned int h = rem / 3600;
    unsigned int m = (rem % 3600) / 60;
    unsigned int s = rem % 60;
    snprintf(timeBuf, timeBufSize, "%02u:%02u:%02u", h, m, s);
  }
}

void pruneOldReports() {
  time_t now = time(nullptr);
  bool isNtp = (now > 1700000000);
  unsigned long nowUptime = millis() / 1000;
  bool changed = false;

  for (int i = (int)dailyRecordCount - 1; i >= 0; i--) {
    bool isExpired = false;
    if (isNtp && dailyRecords[i].epoch > 1700000000) {
      if (now > dailyRecords[i].epoch && (now - dailyRecords[i].epoch) > (48 * 3600)) {
        isExpired = true;
      }
    } else {
      if (nowUptime > dailyRecords[i].epoch && (nowUptime - dailyRecords[i].epoch) > (48 * 3600)) {
        isExpired = true;
      }
    }

    if (isExpired) {
      Serial.printf("[REPORTS] Auto-deleted expired record: %s (>48 hours old)\n", dailyRecords[i].date);
      for (int j = i; j < (int)dailyRecordCount - 1; j++) {
        dailyRecords[j] = dailyRecords[j + 1];
      }
      dailyRecordCount--;
      changed = true;
    }
  }

  if (changed) {
    saveDailyRecords();
  }
}

void loadDailyRecords() {
  byte flag = EEPROM.read(SNOOKER_FLAG_ADDR);
  if (flag == SNOOKER_FLAG_VAL) {
    dailyRecordCount = EEPROM.read(ADDR_HIST_COUNT);
    if (dailyRecordCount > MAX_DAILY_RECORDS) dailyRecordCount = 0;
    for (int i = 0; i < dailyRecordCount; i++) {
      int base = ADDR_HIST_START + (i * sizeof(DailyRecord));
      EEPROM.get(base, dailyRecords[i]);
    }
    pruneOldReports();
    Serial.printf("[REPORTS] Loaded %d daily records from EEPROM\n", dailyRecordCount);
  } else {
    dailyRecordCount = 0;
    EEPROM.write(SNOOKER_FLAG_ADDR, SNOOKER_FLAG_VAL);
    EEPROM.write(ADDR_HIST_COUNT, 0);
    EEPROM.commit();
    Serial.println(F("[REPORTS] Initialized fresh daily records schema in EEPROM"));
  }
}

void saveDailyRecords() {
  EEPROM.write(SNOOKER_FLAG_ADDR, SNOOKER_FLAG_VAL);
  EEPROM.write(ADDR_HIST_COUNT, dailyRecordCount);
  for (int i = 0; i < dailyRecordCount; i++) {
    int base = ADDR_HIST_START + (i * sizeof(DailyRecord));
    EEPROM.put(base, dailyRecords[i]);
  }
  EEPROM.commit();
}

void recordRackCount() {
  char curDate[11];
  char curTime[9];
  getCurrentDateTime(curDate, sizeof(curDate), curTime, sizeof(curTime));

  pruneOldReports();

  time_t now = time(nullptr);
  uint32_t currentStamp = (now > 1700000000) ? (uint32_t)now : (uint32_t)(millis() / 1000);

  int foundIdx = -1;
  for (int i = 0; i < dailyRecordCount; i++) {
    if (strcmp(dailyRecords[i].date, curDate) == 0) {
      foundIdx = i;
      break;
    }
  }

  if (foundIdx >= 0) {
    dailyRecords[foundIdx].racks++;
    strncpy(dailyRecords[foundIdx].lastTime, curTime, 8);
    dailyRecords[foundIdx].lastTime[8] = '\0';
    dailyRecords[foundIdx].epoch = currentStamp;
  } else {
    if (dailyRecordCount >= MAX_DAILY_RECORDS) {
      for (int j = 0; j < MAX_DAILY_RECORDS - 1; j++) {
        dailyRecords[j] = dailyRecords[j + 1];
      }
      dailyRecordCount = MAX_DAILY_RECORDS - 1;
    }
    int newIdx = dailyRecordCount++;
    strncpy(dailyRecords[newIdx].date, curDate, 10);
    dailyRecords[newIdx].date[10] = '\0';
    dailyRecords[newIdx].racks = 1;
    strncpy(dailyRecords[newIdx].lastTime, curTime, 8);
    dailyRecords[newIdx].lastTime[8] = '\0';
    dailyRecords[newIdx].epoch = currentStamp;
  }

  saveDailyRecords();
  Serial.printf("[REPORTS] Daily rack logged: Date=%s, Count=%d\n", curDate, (foundIdx >= 0) ? dailyRecords[foundIdx].racks : 1);
}

void handleReportsAPI() {
  pruneOldReports();
  String json = "{";
  json.reserve(512);

  uint32_t total48h = 0;
  for (int i = 0; i < dailyRecordCount; i++) {
    total48h += dailyRecords[i].racks;
  }
  json += "\"total_48h\":" + String(total48h) + ",";
  json += "\"count\":" + String(dailyRecordCount) + ",";
  json += "\"records\":[";

  time_t now = time(nullptr);
  bool isNtp = (now > 1700000000);
  unsigned long nowUptime = millis() / 1000;

  for (int i = (int)dailyRecordCount - 1; i >= 0; i--) {
    if (i < (int)dailyRecordCount - 1) json += ",";
    json += "{";
    json += "\"date\":\"" + escapeJson(String(dailyRecords[i].date)) + "\",";
    json += "\"racks\":" + String(dailyRecords[i].racks) + ",";
    json += "\"last_time\":\"" + escapeJson(String(dailyRecords[i].lastTime)) + "\",";

    float ageHours = 0.0;
    if (isNtp && dailyRecords[i].epoch > 1700000000) {
      if (now > dailyRecords[i].epoch) {
        ageHours = (float)(now - dailyRecords[i].epoch) / 3600.0f;
      }
    } else {
      if (nowUptime > dailyRecords[i].epoch) {
        ageHours = (float)(nowUptime - dailyRecords[i].epoch) / 3600.0f;
      }
    }
    json += "\"age_hours\":" + String(ageHours, 1);
    json += "}";
  }
  json += "]}";

  server.send(200, "application/json", json);
}

void handleReportsClear() {
  dailyRecordCount = 0;
  saveDailyRecords();
  server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"reports_cleared\"}");
  Serial.println(F("[REPORTS] Daily reports history manually cleared via Web UI"));
}

// ==================== MIFARE CLASSIC RFID CARD READ / WRITE ====================
bool writeStaffDataToCard(const char* name, const char* id, String& errOut) {
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
      piccType != MFRC522::PICC_TYPE_MIFARE_1K && 
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    errOut = "Card is not MIFARE Classic (" + String(mfrc522.PICC_GetTypeName(piccType)) + ")";
    return false;
  }

  const byte numKeys = 4;
  byte knownKeys[numKeys][6] = {
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // Factory default
    {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}, // NXP transport key
    {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}, // NFC forum NDEF key
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // Blank zeroes
  };

  byte trailerBlock = 7; // Sector 1 trailer
  byte blockName = 4;    // Sector 1 Block 0
  byte blockId = 5;      // Sector 1 Block 1

  MFRC522::StatusCode status = MFRC522::STATUS_ERROR;
  bool authenticated = false;

  for (byte k = 0; k < numKeys; k++) {
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) key.keyByte[i] = knownKeys[k][i];

    // Try Key A
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status == MFRC522::STATUS_OK) {
      authenticated = true;
      break;
    }
    mfrc522.PCD_StopCrypto1();

    // Try Key B
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
    if (status == MFRC522::STATUS_OK) {
      authenticated = true;
      break;
    }
    mfrc522.PCD_StopCrypto1();
  }

  if (!authenticated) {
    errOut = "Key Auth failed: custom or protected sector key";
    return false;
  }

  // 1. Prepare 16-byte Name buffer
  byte bufferName[16];
  memset(bufferName, 0, sizeof(bufferName));
  strncpy((char*)bufferName, name, 15);

  status = mfrc522.MIFARE_Write(blockName, bufferName, 16);
  if (status != MFRC522::STATUS_OK) {
    errOut = "Name write failed: " + String(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // 2. Prepare 16-byte ID buffer
  byte bufferId[16];
  memset(bufferId, 0, sizeof(bufferId));
  strncpy((char*)bufferId, id, 15);

  status = mfrc522.MIFARE_Write(blockId, bufferId, 16);
  if (status != MFRC522::STATUS_OK) {
    errOut = "ID write failed: " + String(mfrc522.GetStatusCodeName(status));
    return false;
  }

  return true;
}

void readStaffDataFromCard(String& outName, String& outId) {
  outName = "";
  outId = "";

  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI && 
      piccType != MFRC522::PICC_TYPE_MIFARE_1K && 
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    return;
  }

  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  byte trailerBlock = 7;
  byte blockName = 4;
  byte blockId = 5;

  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) return;
  }

  byte buffer[18];
  byte size = sizeof(buffer);

  // Read Block 4 (Name)
  status = mfrc522.MIFARE_Read(blockName, buffer, &size);
  if (status == MFRC522::STATUS_OK) {
    buffer[16] = '\0';
    bool hasPrintable = false;
    for (int i = 0; i < 16 && buffer[i] != '\0'; i++) {
      if (isPrintable(buffer[i])) hasPrintable = true;
    }
    if (hasPrintable) {
      outName = String((char*)buffer);
      outName.trim();
    }
  }

  // Read Block 5 (ID)
  size = sizeof(buffer);
  status = mfrc522.MIFARE_Read(blockId, buffer, &size);
  if (status == MFRC522::STATUS_OK) {
    buffer[16] = '\0';
    bool hasPrintable = false;
    for (int i = 0; i < 16 && buffer[i] != '\0'; i++) {
      if (isPrintable(buffer[i])) hasPrintable = true;
    }
    if (hasPrintable) {
      outId = String((char*)buffer);
      outId.trim();
    }
  }
}

void processCloudSyncResponse(const String& json) {
  updateAuthorizedCardsFromJson(json);
  updateRfidWriteCmdFromJson(json);

  // 1. Sync system clock from serverEpoch (works even when UDP NTP port 123 is blocked)
  int epochPos = json.indexOf("\"serverEpoch\"");
  if (epochPos != -1) {
    int colon = json.indexOf(":", epochPos);
    if (colon != -1) {
      long long srvEpoch = strtoll(json.substring(colon + 1).c_str(), NULL, 10);
      if (srvEpoch > 1700000000LL) {
        struct timeval tv;
        tv.tv_sec = (time_t)srvEpoch;
        tv.tv_usec = 0;
        settimeofday(&tv, NULL);
      }
    }
  }

  // 2. Cache table metadata & maintenance lock state
  int tblPos = json.indexOf("\"tableName\"");
  if (tblPos != -1) {
    int q1 = json.indexOf("\"", json.indexOf(":", tblPos));
    int q2 = (q1 != -1) ? json.indexOf("\"", q1 + 1) : -1;
    if (q1 != -1 && q2 != -1) cloudTableName = json.substring(q1 + 1, q2);
  }
  int pricePos = json.indexOf("\"pricePerGame\"");
  if (pricePos != -1) {
    int colon = json.indexOf(":", pricePos);
    if (colon != -1) {
      long p = atol(json.substring(colon + 1).c_str());
      if (p > 0) cloudPricePerGame = p;
    }
  }
  int curPos = json.indexOf("\"currency\"");
  if (curPos != -1) {
    int q1 = json.indexOf("\"", json.indexOf(":", curPos));
    int q2 = (q1 != -1) ? json.indexOf("\"", q1 + 1) : -1;
    if (q1 != -1 && q2 != -1) cloudCurrency = json.substring(q1 + 1, q2);
  }
  if (json.indexOf("\"maintenanceLocked\":true") != -1) {
    cloudMaintenanceLocked = true;
  } else if (json.indexOf("\"maintenanceLocked\":false") != -1) {
    cloudMaintenanceLocked = false;
  }

  // 3. Handle one-shot remote unlock command from Dashboard
  int unlPos = json.indexOf("\"unlockCmd\"");
  if (unlPos != -1) {
    int uStart = json.indexOf("{", unlPos);
    int uEnd = (uStart != -1) ? json.indexOf("}", uStart) : -1;
    if (uStart != -1 && uEnd != -1) {
      String uObj = json.substring(uStart, uEnd + 1);
      if (uObj.indexOf("\"pending\":true") != -1) {
        String staff = "Cloud Remote";
        int sPos = uObj.indexOf("\"staffName\"");
        if (sPos != -1) {
          int q1 = uObj.indexOf("\"", uObj.indexOf(":", sPos));
          int q2 = (q1 != -1) ? uObj.indexOf("\"", q1 + 1) : -1;
          if (q1 != -1 && q2 != -1 && q2 > q1 + 1) staff = uObj.substring(q1 + 1, q2);
        }
        Serial.printf("[CLOUD-UNLOCK] Remote unlock requested by %s!\n", staff.c_str());
        lastTriggerSource = "Cloud (" + staff + ")";
        lastStaffName = staff;
        lastStaffId = "CLOUD";
        sessionRackCount++;
        saveRackCounter();
        recordRackCount();
        triggerServoDoorLock();
        queueOfflineEvent("remote_unlock", "");
      }
    }
  }

  // 4. Handle pending OTA firmware update command
  int otaPos = json.indexOf("\"otaCmd\"");
  if (otaPos != -1) {
    int oStart = json.indexOf("{", otaPos);
    int oEnd = (oStart != -1) ? json.indexOf("}", oStart) : -1;
    if (oStart != -1 && oEnd != -1) {
      String oObj = json.substring(oStart, oEnd + 1);
      if (oObj.indexOf("\"pending\":true") != -1) {
        int vPos = oObj.indexOf("\"targetVersion\"");
        int uPos = oObj.indexOf("\"firmwareUrl\"");
        String tVer = "";
        String fUrl = "";
        if (vPos != -1) {
          int q1 = oObj.indexOf("\"", oObj.indexOf(":", vPos));
          int q2 = (q1 != -1) ? oObj.indexOf("\"", q1 + 1) : -1;
          if (q1 != -1 && q2 != -1) tVer = oObj.substring(q1 + 1, q2);
        }
        if (uPos != -1) {
          int q1 = oObj.indexOf("\"", oObj.indexOf(":", uPos));
          int q2 = (q1 != -1) ? oObj.indexOf("\"", q1 + 1) : -1;
          if (q1 != -1 && q2 != -1) fUrl = oObj.substring(q1 + 1, q2);
        }
        if (fUrl.length() > 8 && tVer != FIRMWARE_VERSION) {
          cloudOtaTargetVersion = tVer;
          cloudOtaUrl = fUrl;
          cloudOtaPending = true;
          Serial.printf("[CLOUD-OTA] Update queued: %s -> %s\n", tVer.c_str(), fUrl.c_str());
        }
      }
    }
  }
}

void performCloudOtaIfPending() {
  if (!cloudOtaPending || WiFi.status() != WL_CONNECTED || servoState != SERVO_IDLE_DETACHED) return;
  cloudOtaPending = false;
  Serial.printf("[CLOUD-OTA] Starting OTA download from: %s\n", cloudOtaUrl.c_str());

  httpUpdate.setLedPin(LED_BUILTIN, HIGH);
  httpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  httpUpdate.rebootOnUpdate(true);
  t_httpUpdate_return ret;
  if (cloudOtaUrl.startsWith("https://")) {
    WiFiClientSecure otaClient;
    otaClient.setInsecure();
    otaClient.setTimeout(15000);
    ret = httpUpdate.update(otaClient, cloudOtaUrl);
  } else {
    WiFiClient otaClient;
    ret = httpUpdate.update(otaClient, cloudOtaUrl);
  }

  if (ret == HTTP_UPDATE_FAILED) {
    String errMsg = httpUpdate.getLastErrorString();
    Serial.printf("[CLOUD-OTA] Failed (%d): %s\n", httpUpdate.getLastError(), errMsg.c_str());
    sendCloudEvent("ota_failed", errMsg.substring(0, 60).c_str());
  }
}

void handleCardWrite() {
  String name = server.arg("name");
  String id = server.arg("id");
  name.trim();
  id.trim();

  if (name.length() == 0 || id.length() == 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Staff Name and ID cannot be empty.\"}");
    return;
  }
  if (name.length() > 15 || id.length() > 15) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Name and ID cannot exceed 15 characters.\"}");
    return;
  }

  strncpy(pendingStaffName, name.c_str(), 16);
  pendingStaffName[16] = '\0';
  strncpy(pendingStaffId, id.c_str(), 16);
  pendingStaffId[16] = '\0';

  rfidWritePending = true;
  rfidWriteArmTime = millis();
  rfidWriteStatus = "WAITING_FOR_CARD";
  rfidWriteMessage = "Armed and waiting for card presentation.";
  lastWrittenUid = "";

  Serial.printf("[RC522-WRITE] Armed writer for Name='%s', ID='%s'\n", pendingStaffName, pendingStaffId);
  server.send(200, "application/json", "{\"status\":\"ok\",\"mode\":\"waiting_for_card\"}");
}

void handleCardWriteStatus() {
  if (rfidWritePending && (millis() - rfidWriteArmTime > RFID_WRITE_TIMEOUT_MS)) {
    rfidWritePending = false;
    rfidWriteStatus = "ERROR";
    rfidWriteMessage = "Programming timed out. No card detected within 25 seconds.";
    Serial.println(F("[RC522-WRITE] Session timed out"));
  }

  String json = "{";
  json += "\"status\":\"" + rfidWriteStatus + "\",";
  json += "\"message\":\"" + escapeJson(rfidWriteMessage) + "\",";
  json += "\"pending\":" + String(rfidWritePending ? "true" : "false") + ",";
  json += "\"card_uid\":\"" + escapeJson(lastWrittenUid) + "\",";
  json += "\"staff_name\":\"" + escapeJson(String(pendingStaffName)) + "\",";
  json += "\"staff_id\":\"" + escapeJson(String(pendingStaffId)) + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleCardWriteCancel() {
  rfidWritePending = false;
  rfidWriteStatus = "IDLE";
  rfidWriteMessage = "Programming cancelled by user.";
  Serial.println(F("[RC522-WRITE] Card programming cancelled"));
  server.send(200, "application/json", "{\"status\":\"ok\",\"mode\":\"cancelled\"}");
}

// ==================== CARD BLACKLIST & SECURITY ACCESS CONTROL ====================
bool isCardBlocked(const String& uid) {
  String clean = uid;
  clean.replace(":", "");
  clean.toUpperCase();
  for (int i = 0; i < blockedCardsCount; i++) {
    String bl = String(blockedCards[i]);
    bl.replace(":", "");
    bl.toUpperCase();
    if (clean == bl) {
      return true;
    }
  }
  return false;
}

bool isCardAuthorized(const String& uid, String& outName, String& outId) {
  String cleanUid = uid;
  cleanUid.replace(":", "");
  cleanUid.trim();
  cleanUid.toUpperCase();

  Serial.printf("[AUTH-CHECK] Checking card UID: %s (Hex: %s). Whitelist has %d cards.\n",
                uid.c_str(), cleanUid.c_str(), authorizedCardsCount);

  // Strict Security Check: Card UID MUST exist in the cloud authorized cards whitelist
  for (int i = 0; i < authorizedCardsCount; i++) {
    String listUid = String(authorizedCardsList[i].uid);
    listUid.replace(":", "");
    listUid.trim();
    listUid.toUpperCase();
    if (cleanUid == listUid) {
      outName = String(authorizedCardsList[i].name);
      outId = String(authorizedCardsList[i].id);
      Serial.printf("[AUTH-CHECK] Access GRANTED: Card %s belongs to %s (%s, %s)\n",
                    cleanUid.c_str(), outName.c_str(), outId.c_str(), authorizedCardsList[i].role);
      return true;
    }
  }

  // Not in authorized whitelist -> DENIED!
  Serial.printf("[AUTH-CHECK] Access DENIED: Card %s is NOT in the authorized whitelist.\n", cleanUid.c_str());
  return false;
}

void loadAuthorizedCards() {
  byte flag = EEPROM.read(ADDR_AUTH_CARDS_FLAG);
  if (flag == AUTH_CARDS_FLAG_VAL) {
    uint8_t count = EEPROM.read(ADDR_AUTH_CARDS_COUNT);
    if (count > MAX_PERSISTED_AUTH_CARDS) count = 0;
    for (uint8_t i = 0; i < count; i++) {
      int base = ADDR_AUTH_CARDS_START + (i * sizeof(AuthorizedCardCache));
      EEPROM.get(base, authorizedCardsList[i]);
    }
    authorizedCardsCount = count;
    Serial.printf("[AUTH-CARDS] Loaded %d cached authorized cards from EEPROM\n", authorizedCardsCount);
  } else {
    authorizedCardsCount = 0;
  }
}

void saveAuthorizedCards() {
  uint8_t countToSave = (authorizedCardsCount > MAX_PERSISTED_AUTH_CARDS) ? MAX_PERSISTED_AUTH_CARDS : (uint8_t)authorizedCardsCount;
  EEPROM.write(ADDR_AUTH_CARDS_FLAG, AUTH_CARDS_FLAG_VAL);
  EEPROM.write(ADDR_AUTH_CARDS_COUNT, countToSave);
  for (uint8_t i = 0; i < countToSave; i++) {
    int base = ADDR_AUTH_CARDS_START + (i * sizeof(AuthorizedCardCache));
    EEPROM.put(base, authorizedCardsList[i]);
  }
  EEPROM.commit();
}

void updateAuthorizedCardsFromJson(const String& json) {
  int arrayStart = json.indexOf("\"authorizedCards\":[");
  if (arrayStart == -1) return;
  int arrayEnd = json.indexOf("]", arrayStart);
  if (arrayEnd == -1) return;

  String cardsBlock = json.substring(arrayStart + 18, arrayEnd);
  AuthorizedCardCache newList[MAX_AUTHORIZED_CARDS];
  int count = 0;
  int pos = 0;

  while (pos < (int)cardsBlock.length() && count < MAX_AUTHORIZED_CARDS) {
    int objStart = cardsBlock.indexOf("{", pos);
    if (objStart == -1) break;
    int objEnd = cardsBlock.indexOf("}", objStart);
    if (objEnd == -1) break;

    String obj = cardsBlock.substring(objStart, objEnd + 1);

    auto extractVal = [&](const char* key) -> String {
      int k = obj.indexOf(String("\"") + key + "\":\"");
      if (k == -1) return "";
      int vStart = k + strlen(key) + 4;
      int vEnd = obj.indexOf("\"", vStart);
      if (vEnd == -1) return "";
      return obj.substring(vStart, vEnd);
    };

    String uid = extractVal("cardUid");
    String name = extractVal("staffName");
    String id = extractVal("staffId");
    String role = extractVal("role");

    if (uid.length() > 0) {
      memset(&newList[count], 0, sizeof(AuthorizedCardCache));
      strncpy(newList[count].uid, uid.c_str(), sizeof(newList[count].uid) - 1);
      strncpy(newList[count].name, name.c_str(), sizeof(newList[count].name) - 1);
      strncpy(newList[count].id, id.c_str(), sizeof(newList[count].id) - 1);
      strncpy(newList[count].role, role.c_str(), sizeof(newList[count].role) - 1);
      count++;
    }

    pos = objEnd + 1;
  }

  if (count > 0 || cardsBlock.indexOf("{") == -1) {
    bool changed = (count != authorizedCardsCount);
    if (!changed && count > 0) {
      if (memcmp(authorizedCardsList, newList, count * sizeof(AuthorizedCardCache)) != 0) {
        changed = true;
      }
    }
    memcpy(authorizedCardsList, newList, count * sizeof(AuthorizedCardCache));
    authorizedCardsCount = count;
    if (changed) {
      saveAuthorizedCards();
      Serial.printf("[AUTH-CARDS] Synced & cached %d authorized cards to EEPROM.\n", authorizedCardsCount);
    }
  }
}

void updateRfidWriteCmdFromJson(const String& json) {
  int cmdPos = json.indexOf("\"rfidWriteCmd\"");
  if (cmdPos == -1) return;
  int colonPos = json.indexOf(":", cmdPos);
  if (colonPos == -1) return;
  int objStart = json.indexOf("{", colonPos);
  if (objStart == -1) return;
  int objEnd = json.indexOf("}", objStart);
  if (objEnd == -1) return;

  String cmdObj = json.substring(objStart, objEnd + 1);

  bool arm = (cmdObj.indexOf("\"arm\":true") != -1) ||
             (cmdObj.indexOf("\"arm\": true") != -1) ||
             (cmdObj.indexOf("\"arm\":1") != -1) ||
             (cmdObj.indexOf("\"arm\": 1") != -1);

  if (arm) {
    auto extractVal = [&](const char* key) -> String {
      int k = cmdObj.indexOf(String("\"") + key + "\"");
      if (k == -1) return "";
      int colon = cmdObj.indexOf(":", k);
      if (colon == -1) return "";
      int vStart = cmdObj.indexOf("\"", colon);
      if (vStart == -1) return "";
      int vEnd = cmdObj.indexOf("\"", vStart + 1);
      if (vEnd == -1) return "";
      return cmdObj.substring(vStart + 1, vEnd);
    };

    String name = extractVal("staffName");
    String id = extractVal("staffId");
    name.trim();
    id.trim();
    if (id.length() == 0) id = "STAFF";

    if (name.length() > 0) {
      if (!rfidWritePending || strcmp(pendingStaffName, name.c_str()) != 0 || strcmp(pendingStaffId, id.c_str()) != 0) {
        strncpy(pendingStaffName, name.c_str(), 16);
        pendingStaffName[16] = '\0';
        strncpy(pendingStaffId, id.c_str(), 16);
        pendingStaffId[16] = '\0';
        rfidWritePending = true;
        rfidWriteArmTime = millis();
        rfidWriteStatus = "WAITING_FOR_CARD";
        rfidWriteMessage = "Cloud armed reader for " + name + " (" + id + ")";
        Serial.printf("\n==================================================\n");
        Serial.printf("[CLOUD-RFID] >>> READER ARMED BY ADRENALINE CLOUD! <<<\n");
        Serial.printf("             Staff: %s | ID: %s\n", pendingStaffName, pendingStaffId);
        Serial.printf("             Tap physical card on reader now!\n");
        Serial.printf("==================================================\n\n");
      } else {
        // Keep active session alive while cloud command persists
        rfidWriteArmTime = millis();
      }
    }
  } else {
    // Cloud disarmed / timed out / cancelled
    if (rfidWritePending) {
      rfidWritePending = false;
      rfidWriteStatus = "IDLE";
      rfidWriteMessage = "Disarmed by cloud";
      Serial.println(F("[CLOUD-RFID] Reader disarmed by cloud"));
    }
  }
}

bool blockCard(const String& uid) {
  String cleanUid = uid;
  cleanUid.trim();
  cleanUid.toUpperCase();
  if (cleanUid.length() == 0 || cleanUid.length() > 15) return false;

  if (isCardBlocked(cleanUid)) return true; // Already blocked

  if (blockedCardsCount >= MAX_BLOCKED_CARDS) {
    return false; // Storage full
  }

  strncpy(blockedCards[blockedCardsCount], cleanUid.c_str(), 15);
  blockedCards[blockedCardsCount][15] = '\0';
  blockedCardsCount++;
  saveBlockedCards();
  Serial.printf("[SECURITY] Card UID %s added to BLOCKED list\n", cleanUid.c_str());
  return true;
}

bool unblockCard(const String& uid) {
  String cleanUid = uid;
  cleanUid.trim();
  cleanUid.toUpperCase();

  int foundIdx = -1;
  for (int i = 0; i < blockedCardsCount; i++) {
    if (cleanUid.equalsIgnoreCase(blockedCards[i])) {
      foundIdx = i;
      break;
    }
  }

  if (foundIdx >= 0) {
    for (int j = foundIdx; j < blockedCardsCount - 1; j++) {
      strcpy(blockedCards[j], blockedCards[j + 1]);
    }
    blockedCardsCount--;
    saveBlockedCards();
    Serial.printf("[SECURITY] Card UID %s REMOVED from BLOCKED list\n", cleanUid.c_str());
    return true;
  }
  return false;
}

void loadBlockedCards() {
  byte flag = EEPROM.read(ADDR_BLOCKED_FLAG);
  if (flag == BLOCKED_FLAG_VAL) {
    blockedCardsCount = EEPROM.read(ADDR_BLOCKED_COUNT);
    if (blockedCardsCount > MAX_BLOCKED_CARDS) blockedCardsCount = 0;
    for (int i = 0; i < blockedCardsCount; i++) {
      int base = ADDR_BLOCKED_START + (i * 16);
      for (int k = 0; k < 16; k++) {
        blockedCards[i][k] = EEPROM.read(base + k);
      }
      blockedCards[i][15] = '\0';
    }
    Serial.printf("[SECURITY] Loaded %d blocked cards from EEPROM\n", blockedCardsCount);
  } else {
    blockedCardsCount = 0;
    EEPROM.write(ADDR_BLOCKED_FLAG, BLOCKED_FLAG_VAL);
    EEPROM.write(ADDR_BLOCKED_COUNT, 0);
    EEPROM.commit();
    Serial.println(F("[SECURITY] Initialized fresh blocked cards registry in EEPROM"));
  }
}

void saveBlockedCards() {
  EEPROM.write(ADDR_BLOCKED_FLAG, BLOCKED_FLAG_VAL);
  EEPROM.write(ADDR_BLOCKED_COUNT, blockedCardsCount);
  for (int i = 0; i < blockedCardsCount; i++) {
    int base = ADDR_BLOCKED_START + (i * 16);
    for (int k = 0; k < 16; k++) {
      EEPROM.write(base + k, blockedCards[i][k]);
    }
  }
  EEPROM.commit();
}

void handleBlockedCardsList() {
  String json = "{";
  json += "\"count\":" + String(blockedCardsCount) + ",";
  json += "\"max\":" + String(MAX_BLOCKED_CARDS) + ",";
  json += "\"cards\":[";
  for (int i = 0; i < blockedCardsCount; i++) {
    if (i > 0) json += ",";
    json += "{\"uid\":\"" + escapeJson(String(blockedCards[i])) + "\"}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleCardBlock() {
  String uid = server.arg("uid");
  uid.trim();
  if (uid.length() == 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Card UID cannot be empty\"}");
    return;
  }
  if (blockCard(uid)) {
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"card_blocked\"}");
  } else {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Failed to block card (list may be full)\"}");
  }
}

void handleCardUnblock() {
  String uid = server.arg("uid");
  uid.trim();
  if (uid.length() == 0) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Card UID cannot be empty\"}");
    return;
  }
  if (unblockCard(uid)) {
    server.send(200, "application/json", "{\"status\":\"ok\",\"message\":\"card_unblocked\"}");
  } else {
    server.send(404, "application/json", "{\"status\":\"error\",\"message\":\"Card UID not found in blocked list\"}");
  }
}





void handleConfig() {
  String json = "{";
  json.reserve(320);
  json += "\"ssid\":\"" + escapeJson(String(routerSSID)) + "\",";
  json += "\"pass\":\"" + escapeJson(String(routerPassword)) + "\",";
  json += "\"user\":\"" + escapeJson(String(guiUsername)) + "\",";
  json += "\"auth\":\"" + escapeJson(String(guiPassword)) + "\",";
  json += "\"device_id\":\"" + escapeJson(String(deviceId)) + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleScan() {
  // Cancel any in-progress background scan or STA association that blocks esp_wifi_scan_start() on ESP32
  if (wifiScanActive) {
    WiFi.scanDelete();
    wifiScanActive = false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect(false);
    delay(120);
  }
  WiFi.mode(WIFI_AP_STA);
  int n = WiFi.scanNetworks(false, false, false, 300);
  Serial.printf("[WIFI-SCAN] Scan complete: %d networks found\n", n);
  String json = "[";
  json.reserve(512);

  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() == 0) continue; // Skip hidden/blank SSIDs
      if (json.length() > 1) json += ",";
      json += "{";
      json += "\"ssid\":\"" + escapeJson(ssid) + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
      json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
      json += "}";
    }
  }
  json += "]";
  WiFi.scanDelete();

  server.send(200, "application/json", json);
}

void handleSave() {
  String newSSID  = server.arg("ssid");
  String newPass  = server.arg("pass");
  String newUser  = server.arg("user");
  String newAuth  = server.arg("auth");
  String newDevId = server.arg("device_id");

  // Input length guards
  if (newSSID.length() > 32 || newPass.length() > 64 || newUser.length() > 16 || newAuth.length() > 16 || newDevId.length() > 31) {
    server.send(400, "text/plain", "Error: Input exceeds maximum allowed length.");
    return;
  }
  if (newUser.length() == 0 || newAuth.length() == 0) {
    server.send(400, "text/plain", "Error: Username and Password cannot be empty.");
    return;
  }

  strncpy(guiUsername,    newUser.c_str(), 16); guiUsername[16]    = '\0';
  strncpy(guiPassword,    newAuth.c_str(), 16); guiPassword[16]    = '\0';

  newDevId.trim();
  if (newDevId.length() > 0) {
    strncpy(deviceId, newDevId.c_str(), sizeof(deviceId) - 1);
    deviceId[sizeof(deviceId) - 1] = '\0';
    for (int i = 0; i < 32; i++) {
      EEPROM.write(ADDR_DEVICE_ID + i, i < (int)newDevId.length() ? newDevId[i] : 0);
    }
  }

  saveCredentials();

  // Connect to new WiFi without rebooting! SoftAP remains active throughout.
  newSSID.trim();
  newPass.trim();
  if (newSSID.length() > 0) {
    connectWiFiWithoutReboot(newSSID.c_str(), newPass.c_str());
  } else {
    // If empty SSID submitted, disable router station mode without reboot
    WiFi.disconnect(false, false);
    routerSSID[0] = '\0';
    routerPassword[0] = '\0';
    saveCredentials();
  }

  bool isAjax = server.hasArg("ajax") || 
                (server.hasHeader("Accept") && server.header("Accept").indexOf("application/json") >= 0);

  if (isAjax) {
    server.send(200, "application/json", "{\"ok\":true,\"status\":\"connecting\",\"ssid\":\"" + escapeJson(routerSSID) + "\"}");
  } else {
    server.send_P(200, "text/html", PAGE_SAVED);
  }

  Serial.println(F("[HTTP] Configuration updated and live WiFi switch initiated without reboot."));
}

void handleWifiConnect() {
  if (!requireAuth()) return;
  String reqSSID = server.arg("ssid");
  String reqPass = server.arg("pass");
  reqSSID.trim();
  reqPass.trim();

  if (reqSSID.length() == 0 || reqSSID.length() > 32) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"Invalid SSID\"}");
    return;
  }

  // If password was omitted in the request, try to pull it from savedNetworks memory
  if (reqPass.length() == 0) {
    for (uint8_t i = 0; i < savedNetworkCount; i++) {
      if (reqSSID == savedNetworks[i].ssid) {
        reqPass = savedNetworks[i].pass;
        break;
      }
    }
  }

  connectWiFiWithoutReboot(reqSSID.c_str(), reqPass.c_str());
  server.send(200, "application/json", "{\"ok\":true,\"status\":\"connecting\",\"ssid\":\"" + escapeJson(String(routerSSID)) + "\"}");
}

void handleWifiForget() {
  if (!requireAuth()) return;
  String reqSSID = server.arg("ssid");
  reqSSID.trim();

  if (reqSSID.length() == 0) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"SSID required\"}");
    return;
  }

  bool removed = removeSavedWiFi(reqSSID.c_str());
  if (removed) {
    if (String(routerSSID) == reqSSID) {
      routerSSID[0] = '\0';
      routerPassword[0] = '\0';
      saveCredentials();
      WiFi.disconnect(false, false);
    }
    server.send(200, "application/json", "{\"ok\":true,\"message\":\"Network forgotten successfully\"}");
  } else {
    server.send(404, "application/json", "{\"ok\":false,\"error\":\"Network not found in saved list\"}");
  }
}

void handleSavedWifiList() {
  if (!requireAuth()) return;
  String json = "{\"ok\":true,\"count\":" + String(savedNetworkCount) + ",\"networks\":[";
  for (uint8_t s = 0; s < savedNetworkCount; s++) {
    if (s > 0) json += ",";
    json += "{\"ssid\":\"" + escapeJson(String(savedNetworks[s].ssid)) + "\",";
    json += "\"is_active\":" + String((WiFi.status() == WL_CONNECTED && String(savedNetworks[s].ssid) == WiFi.SSID()) ? "true" : "false") + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void handleRestart() {
  server.send(200, "application/json", "{\"status\":\"restarting\"}");
  Serial.println(F("[SYSTEM] Soft restart initiated via API..."));
  delay(800);
  ESP.restart();
}

void handleFactoryReset() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0xFF);
  }
  EEPROM.commit();
  server.send(200, "application/json", "{\"status\":\"resetting\"}");
  Serial.println(F("[SYSTEM] Factory reset executed. EEPROM cleared. Restarting..."));
  delay(1000);
  ESP.restart();
}

// ==================== RC522 RFID / NFC LOGIC ====================
bool isCardPresent() {
  // Use official, rock-solid MFRC522 card detection
  return mfrc522.PICC_IsNewCardPresent();
}

void updateNFC() {
  // Real-time RC522 Connection & Hot-Plug Monitor (Every 1 second)
  static unsigned long lastRc522Check = 0;
  if (millis() - lastRc522Check >= 1000) {
    lastRc522Check = millis();
    byte currentVer = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    bool wasConnected = (rc522FirmwareVersion != 0x00 && rc522FirmwareVersion != 0xFF);
    bool isConnected = (currentVer != 0x00 && currentVer != 0xFF);

    if (isConnected && !wasConnected) {
      rc522FirmwareVersion = currentVer;
      mfrc522.PCD_Init(activeCsPin, RC522_RST_PIN); // Re-initialize registers & RF antenna
      mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
      delay(30);
      Serial.print(F("[RC522] RFID reader CONNECTED / DETECTED! Version: 0x"));
      Serial.println(rc522FirmwareVersion, HEX);
    } else if (!isConnected) {
      if (wasConnected) {
        Serial.println(F("[RC522] RFID reader DISCONNECTED / LOST! Attempting hardware reset recovery..."));
      }
      // Active hardware reset pulse on RST pin to recover from SPI/brownout lockups
      pinMode(RC522_RST_PIN, OUTPUT);
      digitalWrite(RC522_RST_PIN, LOW);
      delay(15);
      digitalWrite(RC522_RST_PIN, HIGH);
      delay(25);
      mfrc522.PCD_Init(activeCsPin, RC522_RST_PIN);
      mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
      byte recoveredVer = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
      rc522FirmwareVersion = recoveredVer;
      if (recoveredVer != 0x00 && recoveredVer != 0xFF) {
        Serial.printf("[RC522] Hardware recovery SUCCESS! Version: 0x%02X\n", recoveredVer);
      }
    } else {
      rc522FirmwareVersion = currentVer;
    }
  }

  // If reader is disconnected, do not attempt to scan
  if (rc522FirmwareVersion == 0x00 || rc522FirmwareVersion == 0xFF) {
    return;
  }

  static unsigned long lastNfcPoll = 0;
  if (millis() - lastNfcPoll < 50) return; // 20Hz polling interval
  lastNfcPoll = millis();

  // 1. Check if an RFID/NFC card is in the antenna field
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // 2. Select card and read UID
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // 3. Rate-limiting cooldown to prevent rapid multiple reads from a single tap
  if (millis() - lastNfcScanTime < NFC_COOLDOWN_MS) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    return;
  }
  lastNfcScanTime = millis();

  // 4. Format Card UID as Hex string (e.g. "04:A2:3B:5C")
  String cardUid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) cardUid += "0";
    cardUid += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) cardUid += ":";
  }
  cardUid.toUpperCase();

  Serial.println(F("\n[RC522] >>> RFID / NFC Card Tap Detected! <<<"));
  Serial.printf("        Card UID: %s\n", cardUid.c_str());

  // SECURITY CHECK: Is this card stolen or blacklisted?
  if (isCardBlocked(cardUid)) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    lastScannedUid = cardUid;
    lastStaffName = "BLOCKED CARD";
    lastStaffId = "REVOKED";
    lastTriggerSource = "BLOCKED CARD REJECTED (" + cardUid + ")";

    Serial.println(F("\n[SECURITY ALERT] >>> BLOCKED / STOLEN CARD DETECTED! <<<"));
    Serial.printf("                  Card UID: %s (Access: DENIED)\n", cardUid.c_str());

    for (int b = 0; b < 6; b++) {
      setRGB(true, false);
      delay(40);
      setRGB(false, false);
      delay(40);
    }

    beepFeedback(1, 350, 0);
    queueOfflineEvent("blocked_card_attempt", cardUid.c_str());
    return; // Completely stop: door lock remains CLOSED, count does NOT increment!
  }

  // Check if system is armed in Card Programming (Write) mode
  if (rfidWritePending) {
    Serial.printf("[RC522-WRITE] Card detected for programming! UID: %s\n", cardUid.c_str());
    String writeErr = "";
    bool ok = writeStaffDataToCard(pendingStaffName, pendingStaffId, writeErr);

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    if (ok) {
      rfidWriteStatus = "SUCCESS";
      rfidWriteMessage = "Written & authorized! Name: " + String(pendingStaffName) + " | ID: " + String(pendingStaffId);
      Serial.printf("[RC522-WRITE] Sector write SUCCESS: Name=%s, ID=%s, UID=%s\n", pendingStaffName, pendingStaffId, cardUid.c_str());
    } else {
      // Physical card memory is read-only or protected, but its hardware UID is enrolled & authorized!
      rfidWriteStatus = "SUCCESS";
      rfidWriteMessage = "Card UID authorized for " + String(pendingStaffName) + " (" + String(pendingStaffId) + ") [Memory read-only]";
      Serial.printf("[RC522-WRITE] Memory write skipped (%s). Hardware UID %s enrolled & authorized!\n", writeErr.c_str(), cardUid.c_str());
    }

    lastWrittenUid = cardUid;
    lastStaffName = String(pendingStaffName);
    lastStaffId = String(pendingStaffId);

    // Rapid 3-pulse Green visual confirmation
    for (int b = 0; b < 3; b++) {
      setRGB(false, true);
      delay(70);
      setRGB(false, false);
      delay(70);
    }

    // Notify cloud so the card is registered and auto-authorized immediately
    sendCloudEvent("card_written_success", cardUid.c_str());
    rfidWritePending = false;
    return; // Don't trigger door servo during programming
  }

  // Normal read mode: extract Staff Name and ID if present
  String cardStaffName = "";
  String cardStaffId = "";
  readStaffDataFromCard(cardStaffName, cardStaffId);

  // Cleanly conclude card session
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  // SECURITY CHECK: AUTHORIZED CARDS ONLY!
  if (!isCardAuthorized(cardUid, cardStaffName, cardStaffId)) {
    Serial.println(F("\n[SECURITY ALERT] >>> UNAUTHORIZED / RANDOM CARD REJECTED! <<<"));
    Serial.printf("                  Card UID: %s (Access: REJECTED - Table stays LOCKED)\n", cardUid.c_str());

    lastScannedUid = cardUid;
    lastStaffName = "Unauthorized Card";
    lastStaffId = "DENIED";
    lastTriggerSource = "REJECTED (" + cardUid + ")";

    // Blink Red LED 3 rapid warning pulses
    for (int b = 0; b < 3; b++) {
      setRGB(true, false);
      delay(70);
      setRGB(false, false);
      delay(70);
    }

    beepFeedback(3, 85, 65);
    appendDeviceLog("WARN", "RFID", "DENIED " + cardUid);
    queueOfflineEvent("unauthorized_card_attempt", cardUid.c_str());
    return; // Completely stop: door lock remains CLOSED, count does NOT increment!
  }

  if (cloudMaintenanceLocked) {
    Serial.println(F("[MAINTENANCE] Table is locked for maintenance by cloud dashboard."));
    lastTriggerSource = "MAINTENANCE LOCKED";
    beepFeedback(1, 300, 0);
    for (int b = 0; b < 3; b++) {
      setRGB(true, false);
      delay(60);
      setRGB(false, false);
      delay(60);
    }
    return;
  }

  if (lastRackTriggerTime != 0 && (millis() - lastRackTriggerTime < RACK_LOCKOUT_MS)) {
    for (int b = 0; b < 2; b++) { setRGB(true, true); digitalWrite(BUZZER_PIN, HIGH); delay(55); setRGB(false, false); digitalWrite(BUZZER_PIN, LOW); delay(55); }
    Serial.printf("[RC522] Anti-jiggle lockout active (%lums remaining). Ignored card tap.\n",
                  RACK_LOCKOUT_MS - (millis() - lastRackTriggerTime));
    for (int b = 0; b < 2; b++) {
      setRGB(true, true);
      delay(60);
      setRGB(false, false);
      delay(60);
    }
    return;
  }
  lastRackTriggerTime = millis();

  lastScannedUid = cardUid;
  if (cardStaffName.length() > 0) {
    lastStaffName = cardStaffName;
    lastStaffId = (cardStaffId.length() > 0) ? cardStaffId : "N/A";
    lastTriggerSource = cardStaffName + " (" + lastStaffId + ")";
  } else {
    lastStaffName = "Unregistered";
    lastStaffId = "N/A";
    lastTriggerSource = "RFID (" + cardUid + ")";
  }

  sessionRackCount++;
  saveRackCounter();
  recordRackCount();
  appendDeviceLog("INFO", "RFID", "GRANTED " + cardUid + " (" + cardStaffName + ") RSSI=" + String(WiFi.RSSI()) + "dBm");
  queueOfflineEvent("button_press", cardUid.c_str());

  Serial.printf("[SNOOKER] Authorized Card Unlocked! Session total: %lu (Queued for sync: %d)\n",
                sessionRackCount, offlineQueueCount);

  // Turn RGB LED Solid Green for Authorized Game Unlock
  setRGB(false, true);

  // Trigger Servo Door Lock to Open (Cloud sync will fire automatically after servo detaches)
  triggerServoDoorLock();

  // Confirm reader is still healthy and ready for next tap
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (v == 0x00 || v == 0xFF) {
    Serial.println(F("[RC522] WARNING: Reader communication lost after servo/cloud! (Likely brownout/power drop)"));
  } else {
    Serial.printf("[RC522] Ready for next card tap (Reader status: OK, Version: 0x%02X)\n", v);
    Serial.println(F("        Tip: Lift card off reader and tap again."));
  }
}

// ==================== MG996R SERVO CONTROLLER (SMOOTH SOFT-START) ====================
// Micro-stepped non-blocking sweep eliminates the 2.5A locked-rotor inrush spike,
// preventing power rail dips, voltage drops, and ESP brownout reboots.
// Smooth Servo Soft-Start & Anti-Brownout Parameters
#define SERVO_STEP_DEG          2   // Smaller 2-deg steps reduce MG996R acceleration inrush current
#define SERVO_STEP_INTERVAL_MS  18  // 18ms between steps (~110 deg/s smooth, low-current sweep)
#define SERVO_PRE_ATTACH_MS     120 // Let Wi-Fi/HTTP TX finish before energizing servo motor
#define SERVO_POST_DETACH_MS    800 // Let 5V capacitors recharge before TLS cloud sync fires

unsigned long servoSweepStartMs = 0;
bool servoBrownoutDetectedAtBoot = false;
#define MAX_CLOUD_LOG_BUF 6
char cloudLogBuf[MAX_CLOUD_LOG_BUF][112];
int cloudLogCount = 0;

void setServoInFlight(bool active) {
  EEPROM.write(ADDR_SERVO_IN_FLIGHT, active ? SERVO_IN_FLIGHT_VAL : 0x00);
  EEPROM.commit();
}

void appendDeviceLog(const char* level, const char* tag, const String& msg) {
  char curDate[11], curTime[9];
  getCurrentDateTime(curDate, sizeof(curDate), curTime, sizeof(curTime));
  String lineNoNl = "[" + String(curDate) + " " + String(curTime) + "] [B#" + String(bootCount) + "] [" + String(level) + "] [" + String(tag) + "] " + msg;
  // Queue compact copy for remote Adrenaline Cloud Diagnostics stream
  if (cloudLogCount >= MAX_CLOUD_LOG_BUF) {
    for (int i = 0; i < MAX_CLOUD_LOG_BUF - 1; i++) {
      memcpy(cloudLogBuf[i], cloudLogBuf[i + 1], sizeof(cloudLogBuf[i]));
    }
    cloudLogCount = MAX_CLOUD_LOG_BUF - 1;
  }
  strncpy(cloudLogBuf[cloudLogCount], lineNoNl.c_str(), sizeof(cloudLogBuf[cloudLogCount]) - 1);
  cloudLogBuf[cloudLogCount][sizeof(cloudLogBuf[cloudLogCount]) - 1] = '\0';
  cloudLogCount++;

  if (!littleFsReady) return;
  String line = lineNoNl + "\n";

  // Auto-rotate log file if it exceeds 8 KB (keep latest ~5 KB)
  if (LittleFS.exists("/device_log.txt")) {
    File rf = LittleFS.open("/device_log.txt", "r");
    if (rf && rf.size() > 8192) {
      size_t skip = rf.size() - 5120;
      rf.seek(skip);
      rf.readStringUntil('\n'); // Align to next full line
      String tail = rf.readString();
      rf.close();
      File wf = LittleFS.open("/device_log.txt", "w");
      if (wf) {
        wf.print(tail);
        wf.close();
      }
    } else if (rf) {
      rf.close();
    }
  }

  File f = LittleFS.open("/device_log.txt", "a");
  if (f) {
    f.print(line);
    f.close();
  }
}

void checkServoBrownoutOnBoot() {
  String reason = getResetReasonStr();
  bool rcOk = (rc522FirmwareVersion != 0x00 && rc522FirmwareVersion != 0xFF);
  appendDeviceLog("BOOT", "SYS", "Reason=" + reason + " Heap=" + String(ESP.getFreeHeap()) + "B RC522=" + (rcOk ? "OK" : "FAIL") + " Angles=" + String(servoStartAngle) + "->" + String(servoEndAngle));

  if (EEPROM.read(ADDR_SERVO_IN_FLIGHT) == SERVO_IN_FLIGHT_VAL) {
    servoBrownoutDetectedAtBoot = true;
    setServoInFlight(false);
    appendDeviceLog("CRIT", "SERVO_ABORT", "Previous unlock interrupted mid-sweep by reboot/brownout! ResetReason=" + reason);
    Serial.println("[CRIT] Previous servo unlock was interrupted by a reboot/brownout!");
    queueOfflineEvent("servo_brownout_alert", "");
  }
}

void handleDeviceLogs() {
  if (!littleFsReady || !LittleFS.exists("/device_log.txt")) {
    server.send(200, "text/plain", "(No diagnostic logs recorded yet)\n");
    return;
  }
  File f = LittleFS.open("/device_log.txt", "r");
  if (!f) {
    server.send(500, "text/plain", "Failed to open log file\n");
    return;
  }
  server.streamFile(f, "text/plain");
  f.close();
}

void handleDeviceLogsClear() {
  if (littleFsReady && LittleFS.exists("/device_log.txt")) {
    LittleFS.remove("/device_log.txt");
  }
  appendDeviceLog("INFO", "SYS", "Logs cleared by admin");
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void loadServoAngles() {
  byte flag = EEPROM.read(ADDR_SERVO_FLAG);
  if (flag == SERVO_FLAG_VAL) {
    uint8_t s = EEPROM.read(ADDR_SERVO_START);
    uint8_t e = EEPROM.read(ADDR_SERVO_END);
    servoStartAngle = constrain((int)s, 0, 180);
    servoEndAngle   = constrain((int)e, 0, 180);
    Serial.printf("[SERVO] Loaded saved angles from EEPROM: Start(Locked)=%d deg, End(Unlocked)=%d deg\n",
                    servoStartAngle, servoEndAngle);
  } else {
    servoStartAngle = SERVO_LOCKED_ANGLE;
    servoEndAngle   = SERVO_UNLOCKED_ANGLE;
    Serial.printf("[SERVO] Using default angles: Start(Locked)=%d deg, End(Unlocked)=%d deg\n",
                    servoStartAngle, servoEndAngle);
  }
}

void loadServoHoldTime() {
  if (EEPROM.read(ADDR_SERVO_HOLD_FLAG) == SERVO_HOLD_FLAG_VAL) {
    servoHoldSec = constrain((int)EEPROM.read(ADDR_SERVO_HOLD_SEC), 2, 10);
  } else {
    servoHoldSec = 4;
  }
}

void saveServoAngles() {
  servoStartAngle = constrain(servoStartAngle, 0, 180);
  servoEndAngle   = constrain(servoEndAngle, 0, 180);
  EEPROM.write(ADDR_SERVO_FLAG, SERVO_FLAG_VAL);
  EEPROM.write(ADDR_SERVO_START, (uint8_t)servoStartAngle);
  EEPROM.write(ADDR_SERVO_END, (uint8_t)servoEndAngle);
  servoHoldSec = constrain(servoHoldSec, 2, 10);
  EEPROM.write(ADDR_SERVO_HOLD_FLAG, SERVO_HOLD_FLAG_VAL);
  EEPROM.write(ADDR_SERVO_HOLD_SEC, (uint8_t)servoHoldSec);
  EEPROM.commit();
  Serial.printf("[SERVO] Saved angles to EEPROM: Start(Locked)=%d deg, End(Unlocked)=%d deg\n",
                  servoStartAngle, servoEndAngle);
}

// Glitch-free MG996R attach/detach helpers (prevents 90° attach kick & floating-pin detach jump-back)
void attachServoClean(int targetAngle) {
  lockServo.setPeriodHertz(50);
  lockServo.write(targetAngle);           // Pre-load pulse width BEFORE attach so LEDC never flashes 90°
  lockServo.attach(SERVO_PIN, 550, 2350); // Safe MG996R pulse range (avoids internal mechanical end-stop bounce)
  lockServo.write(targetAngle);
  servoDetached = false;
}

void detachServoClean() {
  lockServo.detach();
  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);           // Pull signal wire cleanly to 0V so MG996R never jumps on floating pin
  servoDetached = true;
}

void handleServoConfig() {
  if (!server.hasArg("start") || !server.hasArg("end")) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Missing start or end angle\"}");
    return;
  }
  int newStart = server.arg("start").toInt();
  int newEnd   = server.arg("end").toInt();
  if (newStart < 0 || newStart > 180 || newEnd < 0 || newEnd > 180) {
    server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Angles must be between 0 and 180\"}");
    return;
  }

  if (server.hasArg("hold")) {
    int newHold = server.arg("hold").toInt();
    if (newHold < 2 || newHold > 10) {
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Hold time must be between 2 and 10 seconds\"}");
      return;
    }
    servoHoldSec = newHold;
  }
  servoStartAngle = newStart;
  servoEndAngle   = newEnd;
  saveServoAngles();

  // Gently reposition to the new starting (locked) angle if currently idle
  if (servoState == SERVO_IDLE_DETACHED) {
    currentServoAngle = servoStartAngle;
    attachServoClean(currentServoAngle);
    servoState = SERVO_SETTLING_CLOSE;
    servoCloseSettleStartTime = millis();
  }

  String resp = "{\"status\":\"ok\",\"servo_start_angle\":" + String(servoStartAngle) +
                ",\"servo_end_angle\":" + String(servoEndAngle) + ",\"servo_hold_sec\":" + String(servoHoldSec) + "}";
  server.send(200, "application/json", resp);
}

void triggerServoDoorLock() {
  unsigned long now = millis();
  servoUnlocked = true;
  servoSweepStartMs = now;
  setServoInFlight(true);
  if (servoState == SERVO_IDLE_DETACHED) {
    currentServoAngle = servoStartAngle;
    servoPreAttachStartTime = now;
    servoState = SERVO_PRE_ATTACH_DELAY; // Defer attach 120ms so HTTP/Wi-Fi TX finishes first
  } else {
    servoState = SERVO_SWEEPING_OPEN;
    lastServoStepTime = now;
  }
  appendDeviceLog("INFO", "SERVO", "Start sweep " + String(currentServoAngle) + "->" + String(servoEndAngle) + " deg Heap=" + String(ESP.getFreeHeap()) + "B");
  Serial.printf("[SERVO] Smooth soft-start: sweeping from %d deg to %d deg (Door Unlocking)\n", currentServoAngle, servoEndAngle);
}

void updateServo() {
  unsigned long now = millis();

  switch (servoState) {
    case SERVO_IDLE_DETACHED:
      // Motor detached - 0mA idle current draw
      break;

    case SERVO_PRE_ATTACH_DELAY:
      // Wait for HTTP response / Wi-Fi TX burst to complete before drawing servo inrush current
      if (now - servoPreAttachStartTime >= SERVO_PRE_ATTACH_MS) {
        attachServoClean(currentServoAngle);
        lastServoStepTime = now;
        servoState = SERVO_SWEEPING_OPEN;
      }
      break;

    case SERVO_SWEEPING_OPEN:
      if (now - lastServoStepTime >= SERVO_STEP_INTERVAL_MS) {
        lastServoStepTime = now;
        if (currentServoAngle != servoEndAngle) {
          if (currentServoAngle > servoEndAngle) {
            currentServoAngle -= SERVO_STEP_DEG;
            if (currentServoAngle < servoEndAngle) currentServoAngle = servoEndAngle;
          } else {
            currentServoAngle += SERVO_STEP_DEG;
            if (currentServoAngle > servoEndAngle) currentServoAngle = servoEndAngle;
          }
          lockServo.write(currentServoAngle);
        } else {
          // Reached full open
          servoState = SERVO_HOLDING_OPEN;
          servoHoldStartTime = now;
          appendDeviceLog("INFO", "SERVO", "Reached open (" + String(currentServoAngle) + " deg) in " + String(now - servoSweepStartMs) + "ms");
          Serial.printf("[SERVO] Full open reached (%d deg). Holding for %ds\n", currentServoAngle, servoHoldSec);
        }
      }
      break;

    case SERVO_HOLDING_OPEN:
      if (now - servoHoldStartTime >= ((unsigned long)servoHoldSec * 1000UL)) {
        servoState = SERVO_SWEEPING_CLOSE;
        servoUnlocked = false;
        lastServoStepTime = now;
        Serial.printf("[SERVO] Unlock hold expired. Smooth sweeping back to locked (%d deg)\n", servoStartAngle);
      }
      break;

    case SERVO_SWEEPING_CLOSE:
      if (now - lastServoStepTime >= SERVO_STEP_INTERVAL_MS) {
        lastServoStepTime = now;
        if (currentServoAngle != servoStartAngle) {
          if (currentServoAngle < servoStartAngle) {
            currentServoAngle += SERVO_STEP_DEG;
            if (currentServoAngle > servoStartAngle) currentServoAngle = servoStartAngle;
          } else {
            currentServoAngle -= SERVO_STEP_DEG;
            if (currentServoAngle < servoStartAngle) currentServoAngle = servoStartAngle;
          }
          lockServo.write(currentServoAngle);
        } else {
          // Reached full lock
          servoState = SERVO_SETTLING_CLOSE;
          servoCloseSettleStartTime = now;
        }
      }
      break;

    case SERVO_SETTLING_CLOSE:
      // Allow 400ms for final mechanical gear train settling under load before cutting PWM cleanly
      if (now - servoCloseSettleStartTime >= 400) {
        detachServoClean();
        lastServoDetachMs = now;
        servoState = SERVO_IDLE_DETACHED;
        setServoInFlight(false);
        if (stressTestActive) {
          if (stressTestDone >= stressTestTotal) {
            stressTestActive = false;
            appendDeviceLog("INFO", "STRESS", "Stress test PASS: " + String(stressTestDone) + "/" + String(stressTestTotal) + " racks completed (0 reboots) Heap=" + String(ESP.getFreeHeap()) + "B");
            Serial.printf("[STRESS] Completed %d/%d racks cleanly!\n", stressTestDone, stressTestTotal);
          } else {
            stressTestNextMs = now + (unsigned long)stressTestGapSec * 1000UL;
          }
        }
        appendDeviceLog("INFO", "SERVO", "Cycle done & detached (" + String(now - servoSweepStartMs) + "ms) Heap=" + String(ESP.getFreeHeap()) + "B");
        Serial.println(F("[SERVO] Motor detached & signal pulled LOW at locked position (0mA idle, no kickback)"));
      }
      break;
  }

  // Autonomous Multi-Rack Stress Test Scheduler
  if (stressTestActive && servoState == SERVO_IDLE_DETACHED && now >= stressTestNextMs) {
    if (stressTestDone < stressTestTotal) {
      stressTestDone++;
      Serial.printf("[STRESS] Triggering rack %d/%d...\n", stressTestDone, stressTestTotal);
      triggerServoDoorLock();
    } else {
      stressTestActive = false;
    }
  }
}

void handleServoStress() {
  String action = server.arg("action");
  if (action == "stop") {
    stressTestActive = false;
    appendDeviceLog("INFO", "STRESS", "Stress test stopped by user at " + String(stressTestDone) + "/" + String(stressTestTotal));
    server.send(200, "application/json", "{\"status\":\"ok\",\"stress_active\":false,\"stress_done\":" + String(stressTestDone) + ",\"stress_total\":" + String(stressTestTotal) + "}");
    return;
  }
  int cycles = server.hasArg("cycles") ? server.arg("cycles").toInt() : 10;
  int gap = server.hasArg("gap") ? server.arg("gap").toInt() : 3;
  stressTestTotal = constrain(cycles, 2, 30);
  stressTestGapSec = constrain(gap, 2, 15);
  stressTestDone = 0;
  stressTestActive = true;
  stressTestNextMs = millis() + 250; // Brief 250ms delay so HTTP response transmits first
  appendDeviceLog("INFO", "STRESS", "Started " + String(stressTestTotal) + "-rack stress test (gap=" + String(stressTestGapSec) + "s)");
  server.send(200, "application/json", "{\"status\":\"ok\",\"stress_active\":true,\"stress_done\":0,\"stress_total\":" + String(stressTestTotal) + ",\"gap\":" + String(stressTestGapSec) + "}");
}

void loadRackCounter() {
  byte flag = EEPROM.read(ADDR_RACK_COUNTER_FLAG);
  if (flag == RACK_COUNTER_FLAG_VAL) {
    uint32_t savedVal = 0;
    EEPROM.get(ADDR_RACK_COUNTER_VAL, savedVal);
    if (savedVal != 0xFFFFFFFF) {
      sessionRackCount = savedVal;
    }
    Serial.printf("[COUNTER] Restored persisted rack count from EEPROM: %lu\n", sessionRackCount);
  } else {
    sessionRackCount = 0;
    saveRackCounter();
  }
}

void saveRackCounter() {
  uint32_t val = (uint32_t)sessionRackCount;
  EEPROM.write(ADDR_RACK_COUNTER_FLAG, RACK_COUNTER_FLAG_VAL);
  EEPROM.put(ADDR_RACK_COUNTER_VAL, val);
  EEPROM.commit();
}

void handleCounterReset() {
  sessionRackCount = 0;
  saveRackCounter();
  lastTriggerSource = "Reset via Web";
  lastScannedUid = "None";
  Serial.println(F("[COUNTER] Session count reset to 0 via Web UI"));
  server.send(200, "application/json", "{\"status\":\"ok\",\"game_count\":0,\"button_count\":0}");
}

// ==================== POWER BANK KEEP-ALIVE LOGIC ====================
void handleKeepAlive() {
  if (server.hasArg("enable")) {
    String argVal = server.arg("enable");
    keepAliveEnabled = (argVal == "1" || argVal == "true");
    EEPROM.write(ADDR_KEEPALIVE, keepAliveEnabled ? 1 : 0);
    EEPROM.commit();
    Serial.printf("[KEEPALIVE] Power bank keep-alive set to: %s\n", keepAliveEnabled ? "ENABLED" : "DISABLED");
  }
  String json = "{\"status\":\"ok\",\"keepalive_enabled\":" + String(keepAliveEnabled ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void updateKeepAlive() {
  if (!keepAliveEnabled) return;

  static unsigned long lastKeepAlivePulse = 0;
  if (millis() - lastKeepAlivePulse < 6500) return; // Pulse every 6.5s
  lastKeepAlivePulse = millis();

  // Draw a momentary ~150mA current pulse to reset power bank auto-shutoff timer
  digitalWrite(LED_BUILTIN, LOW); // LED pulse on (active-low)

  // Fast WiFi broadcast packet triggers RF power amplifier burst
  WiFiUDP udp;
  udp.beginPacket("255.255.255.255", 9);
  udp.write((const uint8_t*)"HB", 2);
  udp.endPacket();

  delay(40);
  digitalWrite(LED_BUILTIN, HIGH); // LED pulse off
}

// ==================== OFFLINE EVENT SPOOLING QUEUE ====================
void loadBootCount() {
  if (EEPROM.read(ADDR_BOOT_COUNT_FLAG) == BOOT_COUNT_FLAG_VAL) {
    EEPROM.get(ADDR_BOOT_COUNT_VAL, bootCount);
    bootCount++;
  } else {
    bootCount = 1;
    EEPROM.write(ADDR_BOOT_COUNT_FLAG, BOOT_COUNT_FLAG_VAL);
  }
  EEPROM.put(ADDR_BOOT_COUNT_VAL, bootCount);
  EEPROM.commit();
  Serial.printf("[BOOT] Boot count: %lu (Reason: %s)\n", (unsigned long)bootCount, getResetReasonStr().c_str());
}

void saveOfflineQueue() {
  if (littleFsReady) {
    File f = LittleFS.open("/offline_q.bin", "w");
    if (f) {
      uint16_t cnt = (uint16_t)offlineQueueCount;
      f.write((const uint8_t*)&cnt, sizeof(cnt));
      if (cnt > 0) {
        f.write((const uint8_t*)offlineQueue, cnt * sizeof(QueuedRackEvent));
      }
      f.close();
      return;
    }
  }
  EEPROM.write(ADDR_OFFLINE_Q_FLAG, OFFLINE_Q_FLAG_VAL);
  uint8_t persistCount = (offlineQueueCount > MAX_PERSISTED_OFFLINE_Q) ? MAX_PERSISTED_OFFLINE_Q : (uint8_t)offlineQueueCount;
  EEPROM.write(ADDR_OFFLINE_Q_COUNT, persistCount);
  int startIdx = (offlineQueueCount > MAX_PERSISTED_OFFLINE_Q) ? (offlineQueueCount - MAX_PERSISTED_OFFLINE_Q) : 0;
  for (uint8_t i = 0; i < persistCount; i++) {
    EEPROM.put(ADDR_OFFLINE_Q_START + (i * sizeof(QueuedRackEvent)), offlineQueue[startIdx + i]);
  }
  EEPROM.commit();
}

void loadOfflineQueue() {
  if (littleFsReady && LittleFS.exists("/offline_q.bin")) {
    File f = LittleFS.open("/offline_q.bin", "r");
    if (f) {
      uint16_t cnt = 0;
      if (f.read((uint8_t*)&cnt, sizeof(cnt)) == sizeof(cnt) && cnt <= MAX_OFFLINE_QUEUE) {
        offlineQueueCount = (int)cnt;
        if (cnt > 0) {
          f.read((uint8_t*)offlineQueue, cnt * sizeof(QueuedRackEvent));
        }
        f.close();
        for (int i = 0; i < offlineQueueCount; i++) {
          offlineQueue[i].event[sizeof(offlineQueue[i].event) - 1] = '\0';
          offlineQueue[i].trigger[sizeof(offlineQueue[i].trigger) - 1] = '\0';
          offlineQueue[i].cardUid[sizeof(offlineQueue[i].cardUid) - 1] = '\0';
          offlineQueue[i].staffName[sizeof(offlineQueue[i].staffName) - 1] = '\0';
          offlineQueue[i].staffId[sizeof(offlineQueue[i].staffId) - 1] = '\0';
          offlineQueue[i].date[sizeof(offlineQueue[i].date) - 1] = '\0';
          offlineQueue[i].time[sizeof(offlineQueue[i].time) - 1] = '\0';
          offlineQueue[i].clientEventId[sizeof(offlineQueue[i].clientEventId) - 1] = '\0';
        }
        Serial.printf("[OFFLINE-QUEUE] Restored %d pending offline games from LittleFS\n", offlineQueueCount);
        return;
      }
      f.close();
    }
  }

  if (EEPROM.read(ADDR_OFFLINE_Q_FLAG) == OFFLINE_Q_FLAG_VAL) {
    uint8_t c = EEPROM.read(ADDR_OFFLINE_Q_COUNT);
    if (c > MAX_PERSISTED_OFFLINE_Q) c = 0;
    offlineQueueCount = c;
    for (uint8_t i = 0; i < c; i++) {
      EEPROM.get(ADDR_OFFLINE_Q_START + (i * sizeof(QueuedRackEvent)), offlineQueue[i]);
      offlineQueue[i].event[sizeof(offlineQueue[i].event) - 1] = '\0';
      offlineQueue[i].trigger[sizeof(offlineQueue[i].trigger) - 1] = '\0';
      offlineQueue[i].cardUid[sizeof(offlineQueue[i].cardUid) - 1] = '\0';
      offlineQueue[i].staffName[sizeof(offlineQueue[i].staffName) - 1] = '\0';
      offlineQueue[i].staffId[sizeof(offlineQueue[i].staffId) - 1] = '\0';
      offlineQueue[i].date[sizeof(offlineQueue[i].date) - 1] = '\0';
      offlineQueue[i].time[sizeof(offlineQueue[i].time) - 1] = '\0';
      offlineQueue[i].clientEventId[sizeof(offlineQueue[i].clientEventId) - 1] = '\0';
    }
    Serial.printf("[OFFLINE-QUEUE] Restored %d pending offline games from EEPROM\n", offlineQueueCount);
  } else {
    offlineQueueCount = 0;
  }
}

void queueOfflineEvent(const char* eventType, const char* cardUid) {
  if (offlineQueueCount >= MAX_OFFLINE_QUEUE) {
    for (int i = 0; i < MAX_OFFLINE_QUEUE - 1; i++) {
      offlineQueue[i] = offlineQueue[i + 1];
    }
    offlineQueueCount = MAX_OFFLINE_QUEUE - 1;
  }

  char curDate[11], curTime[9];
  getCurrentDateTime(curDate, sizeof(curDate), curTime, sizeof(curTime));

  memset(&offlineQueue[offlineQueueCount], 0, sizeof(QueuedRackEvent));
  strncpy(offlineQueue[offlineQueueCount].event, eventType, sizeof(offlineQueue[offlineQueueCount].event) - 1);
  if (cardUid != NULL && strlen(cardUid) > 0) {
    strncpy(offlineQueue[offlineQueueCount].trigger, "rfid", sizeof(offlineQueue[offlineQueueCount].trigger) - 1);
    strncpy(offlineQueue[offlineQueueCount].cardUid, cardUid, sizeof(offlineQueue[offlineQueueCount].cardUid) - 1);
    if (lastStaffName != "None" && lastStaffName != "Unregistered") {
      strncpy(offlineQueue[offlineQueueCount].staffName, lastStaffName.c_str(), sizeof(offlineQueue[offlineQueueCount].staffName) - 1);
      strncpy(offlineQueue[offlineQueueCount].staffId, lastStaffId.c_str(), sizeof(offlineQueue[offlineQueueCount].staffId) - 1);
    }
  } else {
    strncpy(offlineQueue[offlineQueueCount].trigger, "web_portal", sizeof(offlineQueue[offlineQueueCount].trigger) - 1);
  }
  strncpy(offlineQueue[offlineQueueCount].date, curDate, sizeof(offlineQueue[offlineQueueCount].date) - 1);
  strncpy(offlineQueue[offlineQueueCount].time, curTime, sizeof(offlineQueue[offlineQueueCount].time) - 1);
  eventSeq++;
  snprintf(offlineQueue[offlineQueueCount].clientEventId, sizeof(offlineQueue[offlineQueueCount].clientEventId),
           "%x-%lu-%lu", getChipId(), (unsigned long)bootCount, (unsigned long)eventSeq);
  offlineQueue[offlineQueueCount].queuedUptimeSec = millis() / 1000;
  offlineQueueCount++;
  saveOfflineQueue();

  Serial.printf("[OFFLINE-QUEUE] Rack event buffered & saved to EEPROM (%d in queue)\n", offlineQueueCount);
}

void flushOfflineQueue() {
  if (offlineQueueCount == 0 || WiFi.status() != WL_CONNECTED) return;

  String url = String(adrenalineCloudUrl);
  url.trim();
  if (url.length() < 8) return;

  lastCloudSyncAttempt = millis();
  Serial.printf("[OFFLINE-QUEUE] Wi-Fi connected! Flushing %d buffered games to cloud...\n", offlineQueueCount);

  HTTPClient http;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  bool isHttps = url.startsWith("https://");
  bool beginSuccess = false;

  if (isHttps) {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    if (freeHeap < 10500 || maxBlock < 4500) {
      Serial.printf("[HEAP-GUARD] Low heap (%u B, max block %u B) before batch TLS; deferring flush.\n", freeHeap, maxBlock);
      lastCloudStatus = "LOW_HEAP";
      return;
    }
    secureClient.setInsecure();
    secureClient.setTimeout(15000);
    beginSuccess = http.begin((WiFiClient&)secureClient, url);
  } else {
    beginSuccess = http.begin(plainClient, url);
  }

  if (beginSuccess) {
    http.useHTTP10(true);
    http.setReuse(false);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-device-token", adrenalineDeviceToken);
    http.setTimeout(15000); // 15s timeout to accommodate Render cold starts / TLS handshake

    int batchLimit = (offlineQueueCount > 4) ? 4 : offlineQueueCount;
    String payload;
    payload.reserve(896);
    payload = "{\"device_id\":\"" + String(deviceId) + "\",\"serial_number\":\"" + String(deviceSerial) + "\",\"claim_pin\":\"" + String(claimPinStr) + "\",\"batch\":[";
    for (int i = 0; i < batchLimit; i++) {
      if (i > 0) payload += ",";
      payload += "{";
      payload += "\"event\":\"" + String(offlineQueue[i].event) + "\",";
      payload += "\"trigger\":\"" + String(offlineQueue[i].trigger) + "\",";
      if (strlen(offlineQueue[i].cardUid) > 0) {
        payload += "\"card_uid\":\"" + String(offlineQueue[i].cardUid) + "\",";
        if (strlen(offlineQueue[i].staffName) > 0) {
          payload += "\"staff_name\":\"" + escapeJson(offlineQueue[i].staffName) + "\",";
          payload += "\"staff_id\":\"" + escapeJson(offlineQueue[i].staffId) + "\",";
        }
      }
      payload += "\"date\":\"" + String(offlineQueue[i].date) + "\",";
      payload += "\"time\":\"" + String(offlineQueue[i].time) + "\"";
      if (strlen(offlineQueue[i].clientEventId) > 0) {
        payload += ",\"client_event_id\":\"" + String(offlineQueue[i].clientEventId) + "\"";
      }
      uint32_t nowSec = millis() / 1000;
      if (nowSec >= offlineQueue[i].queuedUptimeSec && offlineQueue[i].queuedUptimeSec > 0) {
        payload += ",\"age_seconds\":" + String(nowSec - offlineQueue[i].queuedUptimeSec);
      }
      payload += "}";
    }
    payload += "],";
    payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    payload += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    payload += "\"chip_id\":\"" + String(getChipId(), HEX) + "\",";
    payload += "\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\",";
    payload += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    payload += "\"reset_reason\":\"" + escapeJson(getResetReasonStr()) + "\",";
    payload += "\"rc522_ok\":" + String((rc522FirmwareVersion != 0x00 && rc522FirmwareVersion != 0xFF) ? "true" : "false") + ",";
    payload += "\"offline_queue_count\":" + String(offlineQueueCount) + ",";
    payload += "\"boot_count\":" + String(bootCount);
    if (cloudLogCount > 0) {
      payload += ",\"device_logs\":[";
      for (int i = 0; i < cloudLogCount; i++) {
        if (i > 0) payload += ",";
        payload += "\"" + escapeJson(String(cloudLogBuf[i])) + "\"";
      }
      payload += "]";
    }
    payload += "}";

    int code = http.POST(payload);
    lastCloudHttpCode = code;
    if (code >= 200 && code < 300) {
      lastCloudStatus = "OK";
      cloudLogCount = 0;
      Serial.printf("[OFFLINE-QUEUE] Batch sync accepted by cloud (%d/%d games recovered)\n", batchLimit, offlineQueueCount);
      int remaining = offlineQueueCount - batchLimit;
      for (int i = 0; i < remaining; i++) {
        offlineQueue[i] = offlineQueue[i + batchLimit];
      }
      offlineQueueCount = remaining;
      saveOfflineQueue();
      if (offlineQueueCount == 0) {
        adrenalineSyncPending = false;
      }
      String response = http.getString();
      processCloudSyncResponse(response);
    } else {
      lastCloudStatus = "ERR_" + String(code);
      appendDeviceLog("ERR", "CLOUD", "Batch flush HTTP " + String(code) + " (" + http.errorToString(code) + ") Q=" + String(offlineQueueCount) + " Heap=" + String(ESP.getFreeHeap()) + "B");
      Serial.printf("[OFFLINE-QUEUE] Batch flush failed (HTTP %d: %s). Will retry.\n", code, http.errorToString(code).c_str());
    }
    http.end();
    if (isHttps) secureClient.stop();
  }
  lastCloudSyncAttempt = millis();
}

// ==================== PURE STATELESS TRANSCEIVER LOGIC ====================
void sendCloudEvent(const char* eventType, const char* cardUid) {
  if (WiFi.status() != WL_CONNECTED) {
    if (strcmp(eventType, "button_press") == 0) {
      queueOfflineEvent(eventType, cardUid);
      adrenalineSyncPending = true;
    }
    return;
  }

  String url = String(adrenalineCloudUrl);
  url.trim();
  if (url.length() < 8) return;

  Serial.printf("[CLOUD] Transmitting %s event -> %s\n", eventType, url.c_str());

  bool isHttps = url.startsWith("https://");
  HTTPClient http;
  bool beginSuccess = false;

  WiFiClient plainClient;
  WiFiClientSecure secureClient;

  if (isHttps) {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t maxBlock = ESP.getMaxAllocHeap();
    if (freeHeap < 10500 || maxBlock < 4500) {
      Serial.printf("[HEAP-GUARD] Low heap (%u B, max block %u B) before '%s' TLS; skipping.\n", freeHeap, maxBlock, eventType);
      lastCloudStatus = "LOW_HEAP";
      lastCloudSyncAttempt = millis();
      return;
    }
    secureClient.setInsecure(); // Skip SSL thumbprint validation
    secureClient.setTimeout(15000);
    beginSuccess = http.begin((WiFiClient&)secureClient, url);
  } else {
    beginSuccess = http.begin(plainClient, url);
  }

  if (beginSuccess) {
    http.useHTTP10(true);
    http.setReuse(false);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-device-token", adrenalineDeviceToken);
    http.setTimeout(15000); // 15s timeout for Render cold start / TLS handshake

    // Pure stateless payload: sends event, trigger source, optional card uid, device id, rssi, and ip
    String payload;
    payload.reserve(640);
    payload = "{";
    payload += "\"device_id\":\"" + String(deviceId) + "\",";
    payload += "\"serial_number\":\"" + String(deviceSerial) + "\",";
    payload += "\"claim_pin\":\"" + String(claimPinStr) + "\",";
    payload += "\"event\":\"" + String(eventType) + "\",";
    if (cardUid != NULL && strlen(cardUid) > 0) {
      payload += "\"trigger\":\"rfid\",";
      payload += "\"card_uid\":\"" + String(cardUid) + "\",";
      if (lastStaffName != "None" && lastStaffName != "Unregistered") {
        payload += "\"staff_name\":\"" + escapeJson(lastStaffName) + "\",";
        payload += "\"staff_id\":\"" + escapeJson(lastStaffId) + "\",";
      }
    } else {
      payload += "\"trigger\":\"web_portal\",";
    }
    payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    payload += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    payload += "\"chip_id\":\"" + String(getChipId(), HEX) + "\",";
    payload += "\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\",";
    payload += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    payload += "\"reset_reason\":\"" + escapeJson(getResetReasonStr()) + "\",";
    payload += "\"rc522_ok\":" + String((rc522FirmwareVersion != 0x00 && rc522FirmwareVersion != 0xFF) ? "true" : "false") + ",";
    payload += "\"offline_queue_count\":" + String(offlineQueueCount) + ",";
    payload += "\"boot_count\":" + String(bootCount);
    if (cloudLogCount > 0) {
      payload += ",\"device_logs\":[";
      for (int i = 0; i < cloudLogCount; i++) {
        if (i > 0) payload += ",";
        payload += "\"" + escapeJson(String(cloudLogBuf[i])) + "\"";
      }
      payload += "]";
    }
    payload += "}";

    int httpCode = http.POST(payload);
    lastCloudHttpCode = httpCode;
    if (httpCode >= 200 && httpCode < 300) {
      lastCloudStatus = "OK";
      cloudLogCount = 0;
      Serial.printf("[CLOUD] Event '%s' accepted by Cloud (HTTP %d)\n", eventType, httpCode);
      if (strcmp(eventType, "button_press") == 0 || strcmp(eventType, "rack_unlock") == 0) {
        adrenalineSyncPending = false;
      }
      String response = http.getString();
      Serial.printf("[CLOUD-RESPONSE] (%d bytes): %s\n", response.length(), response.c_str());
      processCloudSyncResponse(response);
    } else {
      lastCloudStatus = "ERR_" + String(httpCode);
      appendDeviceLog("ERR", "CLOUD", String(eventType) + " HTTP " + String(httpCode) + " (" + http.errorToString(httpCode) + ") Heap=" + String(ESP.getFreeHeap()) + "B");
      Serial.printf("[CLOUD] Event failed (HTTP %d): %s\n", httpCode, http.errorToString(httpCode).c_str());
      String errBody = http.getString();
      if (errBody.length() > 0) {
        Serial.printf("[CLOUD-ERR-BODY] %s\n", errBody.c_str());
      }
    }
    http.end();
    if (isHttps) secureClient.stop();
  } else {
    Serial.println(F("[CLOUD] Failed to begin HTTP connection"));
  }

  lastCloudSyncAttempt = millis();
}

// ==================== SETUP ====================
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  // Quick R -> G -> B power-on self-test flash (confirms all 3 RGB channels are wired)
  setRGB(true, false, false);  delay(120);
  setRGB(false, true, false);  delay(120);
  setRGB(false, false, true);  delay(120);
  setRGB(false, false, false); // Start with LEDs OFF

  Serial.begin(115200);
  delay(500);

  // Initialize hardware identity derived from silicon Chip ID
  initHardwareIdentity();

  Serial.println(F("\n\n========================================"));
  Serial.println(F("   Smarthive Pool Controller Booting"));
  Serial.printf ("   Serial:     %s\n", deviceSerial);
  Serial.printf ("   Claim PIN:  %s\n", claimPinStr);
  Serial.printf ("   Device ID:  %s\n", deviceId);
  Serial.printf ("   SoftAP:     %s\n", apSSID);
  Serial.println(F("========================================"));

  // 1. Initialize VSPI bus and RC522 RFID / NFC Reader on GPIO 5
  SPI.begin();
  delay(30);

  activeCsPin = RC522_SS_PIN; // GPIO 5
  mfrc522.PCD_Init(activeCsPin, RC522_RST_PIN);
  delay(40);
  rc522FirmwareVersion = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

  // Maximize antenna gain (48dB) for reliable card detection & ensure antenna is ON
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  mfrc522.PCD_AntennaOn();

  Serial.print(F("[RC522] RFID Reader Version: 0x"));
  Serial.print(rc522FirmwareVersion, HEX);
  if (rc522FirmwareVersion == 0x91 || rc522FirmwareVersion == 0x92) {
    Serial.println(F(" (Genuine RC522 connected on SDA -> GPIO 5)"));
  } else if (rc522FirmwareVersion == 0x12) {
    Serial.println(F(" (Clone RC522 connected on SDA -> GPIO 5, ready)"));
  } else if (rc522FirmwareVersion == 0x00 || rc522FirmwareVersion == 0xFF) {
    Serial.println(F(" (WARNING: Reader NOT detected! Check 3.3V, GND, MOSI=23, MISO=19, SCK=18, SDA=5, RST=22)"));
  } else {
    Serial.println(F(" (Custom/Unknown Version)"));
  }

  // Load EEPROM credentials & saved servo angles before positioning servo
  loadCredentials();
  loadServoAngles();
  loadServoHoldTime();

  // 2. Initialize MG996R Servo on GPIO 13 (Set saved starting/locked angle gently then detach cleanly)
  attachServoClean(servoStartAngle);
  delay(500);
  detachServoClean();
  servoState = SERVO_IDLE_DETACHED;
  currentServoAngle = servoStartAngle;
  Serial.printf("[SERVO] Initialized on Pin %s - Locked at %d deg (End=%d deg) & Detached (0mA idle current)\n", "GPIO 13", servoStartAngle, servoEndAngle);

  loadBootCount();
  littleFsReady = LittleFS.begin(true);
  Serial.printf("[FS] LittleFS mount: %s\n", littleFsReady ? "OK" : "Unavailable (using EEPROM fallback)");
  loadDailyRecords();
  loadRackCounter();
  loadOfflineQueue();
  loadAuthorizedCards();
  loadBlockedCards();
  checkServoBrownoutOnBoot();
  configTime(3600, 0, "pool.ntp.org", "time.google.com"); // UTC+1 (WAT / Africa/Lagos)

  WiFi.mode(WIFI_AP_STA);
  WiFi.setTxPower(WIFI_POWER_15dBm); // Trim peak Wi-Fi TX current (~40% lower spikes, anti-brownout)
  WiFi.setSleep(false); // Disable modem sleep for fast, reliable TLS
  delay(100);

  // Explicitly configure SoftAP IP, Gateway, and Subnet to ensure phone DHCP succeeds reliably
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  bool apStarted = WiFi.softAP(apSSID, apPassword);
  if (apStarted) {
    Serial.println(F("[AP] Access Point ACTIVE"));
    Serial.print(F("     SSID: "));     Serial.println(apSSID);
    Serial.print(F("     Password: ")); Serial.println(apPassword);
    Serial.print(F("     AP IP: "));    Serial.println(WiFi.softAPIP());

    // Start DNS server redirecting all domain requests to AP IP for captive portal
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    Serial.println(F("[DNS] Captive Portal DNS Server started"));
  } else {
    Serial.println(F("[AP] Access Point FAILED to start"));
  }

  // Attempt router connection only if configured
  if (strlen(routerSSID) > 0) {
    Serial.print(F("\n[STA] Connecting to router: "));
    Serial.println(routerSSID);
    WiFi.begin(routerSSID, routerPassword);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(300);
      Serial.print(F("."));
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("\n[STA] Router Connected!"));
      Serial.print(F("      IP: "));   Serial.println(WiFi.localIP());
      Serial.print(F("      RSSI: ")); Serial.print(WiFi.RSSI()); Serial.println(F(" dBm"));
      lastWiFiConnectedStatus = true;
    } else {
      Serial.println(F("\n[STA] Router connection timed out — keeping SoftAP active"));
      WiFi.disconnect(false); // Stop background scanning loop to preserve SoftAP bandwidth
      lastWiFiConnectedStatus = false;
    }
  } else {
    Serial.println(F("\n[STA] No router SSID configured — connect to AP to configure network"));
    WiFi.disconnect(false);
  }

  // Web OTA Firmware Updater (accessible via /update)
  server.on("/update", HTTP_GET, []() {
    if (!requireAuth()) return;
    server.send(200, "text/html", "<html><body style=\"font-family:sans-serif;background:#0f172a;color:#e2e8f0;padding:24px\"><h3>Smarthive Pool ESP32 OTA Update</h3><form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"update\" accept=\".bin\"><br><br><button type=\"submit\">Upload Firmware</button></form></body></html>");
  });
  server.on("/update", HTTP_POST, []() {
    if (!requireAuth()) return;
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "UPDATE FAIL" : "UPDATE OK - REBOOTING");
    delay(300);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("[OTA] Receiving firmware: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) Update.printError(Serial);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) Serial.printf("[OTA] Update Success: %u bytes\n", upload.totalSize);
      else Update.printError(Serial);
    }
  });
  Serial.println("[OTA] Web Updater registered on /update");

  // Captive portal probes & web routes
  server.on("/", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleDashboard();
  });
  server.on("/index.html", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleDashboard();
  });
  server.on("/index.htm", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleDashboard();
  });
  server.on("/api/status", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleAPI();
  });
  server.on("/api/config", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleConfig();
  });
  server.on("/api/scan", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleScan();
  });
  server.on("/api/restart", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleRestart();
  });
  server.on("/api/reset", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleFactoryReset();
  });
  server.on("/settings", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleSettings();
  });
  server.on("/connection", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleDashboard();
  });
  server.on("/reports", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleDashboard();
  });
  server.on("/api/reports", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleReportsAPI();
  });
  server.on("/api/reports/clear", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleReportsClear();
  });
  server.on("/api/card/write", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleCardWrite();
  });
  server.on("/api/card/write/status", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleCardWriteStatus();
  });
  server.on("/api/card/write/cancel", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleCardWriteCancel();
  });
  server.on("/api/card/blocked", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleBlockedCardsList();
  });
  server.on("/api/card/block", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleCardBlock();
  });
  server.on("/api/card/unblock", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleCardUnblock();
  });
  server.on("/save", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleSave();
  });
  server.on("/api/wifi/connect", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleWifiConnect();
  });
  server.on("/api/wifi/forget", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleWifiForget();
  });
  server.on("/api/saved-wifi", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleSavedWifiList();
  });
  server.on("/api/counter/reset", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleCounterReset();
  });
  server.on("/api/counter/reset", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleCounterReset();
  });
  server.on("/api/button/reset", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleCounterReset();
  });
  server.on("/api/button/reset", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleCounterReset();
  });
  server.on("/api/servo/open", HTTP_POST, []() {
    if (!requireAuth()) return;
    triggerServoDoorLock();
    server.send(200, "application/json", "{\"status\":\"ok\",\"servo\":\"unlocked\"}");
  });
  server.on("/api/servo/open", HTTP_GET, []() {
    if (!requireAuth()) return;
    triggerServoDoorLock();
    server.send(200, "application/json", "{\"status\":\"ok\",\"servo\":\"unlocked\"}");
  });
  server.on("/api/servo/config", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleServoConfig();
  });
  server.on("/api/servo/config", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleServoConfig();
  });
  server.on("/api/servo/stress", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleServoStress();
  });
  server.on("/api/servo/stress", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleServoStress();
  });
  server.on("/api/logs", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleDeviceLogs();
  });
  server.on("/api/logs/clear", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleDeviceLogsClear();
  });
  server.on("/api/keepalive", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleKeepAlive();
  });
  server.on("/api/keepalive", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleKeepAlive();
  });

  // Login / Logout routes (public)
  server.on("/login", HTTP_GET, handleLoginPage);
  server.on("/api/login", HTTP_POST, handleLoginPost);
  server.on("/logout", HTTP_GET, handleLogout);
  server.on("/api/logout", HTTP_POST, handleLogout);

  // OS Captive portal detection endpoints
  server.on("/generate_204", HTTP_GET, []() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
  });
  server.on("/gen_204", HTTP_GET, []() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
  });
  server.on("/hotspot-detect.html", HTTP_GET, []() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
  });
  server.on("/connecttest.txt", HTTP_GET, []() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/", true);
    server.send(302, "text/plain", "");
  });
  server.on("/ncsi.txt", HTTP_GET, []() {
    server.send(200, "text/plain", "Microsoft NCSI");
  });
  server.on("/success.txt", HTTP_GET, []() {
    server.send(200, "text/plain", "success\n");
  });

  server.onNotFound(handleNotFound);

  const char* authHeaderKeys[] = {"Cookie", "Authorization"};
  server.collectHeaders(authHeaderKeys, 2);

  server.begin();
  Serial.println(F("[HTTP] Web server started on port 80"));
  Serial.printf("[MEM] Boot Free Heap: %u B | Max Block: %u B\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  Serial.println(F("========================================\n"));
}

// ==================== LOOP ====================
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  updateLED();
  updateNFC();
  updateServo();
  updateKeepAlive();

  static unsigned long lastPruneTime = 0;
  if (millis() - lastPruneTime > 60000) {
    lastPruneTime = millis();
    pruneOldReports();
  }

  // Multi-Network Automatic Reconnect & Background Beacon Detection (Zero Reboots)
  updateWiFiReconnect();

  // Periodic heartbeat sync with Adrenaline Cloud (or batch flush if offline events were queued)
  // Guarded by servoState == SERVO_IDLE_DETACHED + 800ms post-detach cap recharge window
  if (WiFi.status() == WL_CONNECTED && lastWiFiConnectedStatus && servoState == SERVO_IDLE_DETACHED && (millis() - lastServoDetachMs >= SERVO_POST_DETACH_MS)) {
    if (offlineQueueCount > 0) {
      if (millis() - lastCloudSyncAttempt > 5000) {
        flushOfflineQueue();
      }
    } else if (adrenalineSyncPending) {
      if (millis() - lastCloudSyncAttempt > 5000) {
        sendCloudEvent("button_press", lastScannedUid == "None" ? "" : lastScannedUid.c_str());
      }
    } else if (millis() - lastCloudSyncAttempt > CLOUD_HEARTBEAT_INTERVAL) {
      sendCloudEvent("heartbeat");
    }
    performCloudOtaIfPending();
  }

  // Idle low-heap recovery (all state is persisted in EEPROM/LittleFS)
  if (servoState == SERVO_IDLE_DETACHED && ESP.getFreeHeap() < 5500) {
    Serial.printf("[HEAP-GUARD] Critical low heap (%u B) while idle; rebooting cleanly...\n", ESP.getFreeHeap());
    delay(200);
    ESP.restart();
  }
}