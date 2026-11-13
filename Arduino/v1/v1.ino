#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <Update.h>
#include <HTTPUpdate.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>
#include <time.h>
#include <sys/time.h>
#include <WiFiUdp.h>
#include "web_pages.h"

#define CURRENT_FIRMWARE_VERSION "v2.0.0-esp32"

// ==================== FORWARD DECLARATIONS ====================
uint32_t getChipId();
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
void setRGB(bool red, bool green);
void updateLED();
void updateNFC();
bool isCardPresent();
void updateServo();
void triggerServoDoorLock();
void sendCloudEvent(const char* eventType, const char* cardUid = "");
void parseCloudCommands(const String& json);
void checkForCloudOta(const String& json);
void executeCloudOta(const String& firmwareUrl, const String& targetVersion);
void handleCounterReset();
void handleKeepAlive();
void updateKeepAlive();
void loadBootCount();
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
#define INIT_FLAG         0xB2 // V1.0 General Product Schema
#define ADDR_KEEPALIVE    470  // 1 byte (1 = enabled, 0 = disabled)
#define ADDR_CLOUD_URL    480  // 160 bytes (max 159 chars + null)
#define ADDR_CLOUD_TOKEN  640  // 64 bytes (max 63 chars + null)

// Multi-WiFi Memory Block (710..1105) - Remembers previously connected networks
#define ADDR_SAVED_WIFI_FLAG  710 // 1 byte
#define SAVED_WIFI_FLAG_VAL   0xE5
#define ADDR_SAVED_WIFI_COUNT 711 // 1 byte uint8_t (0..MAX_SAVED_WIFI)
#define ADDR_SAVED_WIFI_START 712 // 4 entries * 98 bytes = 392 bytes (712..1103)
#define MAX_SAVED_WIFI        4

// Persisted Session Rack Counter (1110..1114)
#define ADDR_RACK_COUNTER_FLAG  1110 // 1 byte
#define RACK_COUNTER_FLAG_VAL   0xA3
#define ADDR_RACK_COUNTER_VAL   1111 // 4 bytes (uint32_t)

// Persisted Boot Counter (1115..1119) - Used for unique idempotent client_event_id
#define ADDR_BOOT_COUNT_FLAG    1115 // 1 byte
#define BOOT_COUNT_FLAG_VAL     0xB7
#define ADDR_BOOT_COUNT_VAL     1116 // 4 bytes (uint32_t)

// Persisted Offline Event Spooling Queue (1120..3041) - Up to 12 offline games * 160 bytes
#define ADDR_OFFLINE_Q_FLAG     1120 // 1 byte
#define OFFLINE_Q_FLAG_VAL      0xD4
#define ADDR_OFFLINE_Q_COUNT    1121 // 1 byte
#define ADDR_OFFLINE_Q_START    1122 // 12 * 160 = 1920 bytes (1122..3041)
#define MAX_PERSISTED_OFFLINE_Q 12

// Persisted Authorized Cards Cache (3080..4033) - Up to 14 cards * 68 bytes
#define ADDR_AUTH_CARDS_FLAG    3080 // 1 byte
#define AUTH_CARDS_FLAG_VAL     0xF2
#define ADDR_AUTH_CARDS_COUNT   3081 // 1 byte
#define ADDR_AUTH_CARDS_START   3082 // 14 * 68 = 952 bytes (3082..4033)
#define MAX_PERSISTED_AUTH_CARDS 14

struct SavedWiFiNetwork {
  char ssid[33];
  char pass[65];
};

SavedWiFiNetwork savedNetworks[MAX_SAVED_WIFI];
uint8_t savedNetworkCount = 0;

// Forward declarations for multi-wifi & reconnect
void loadSavedWiFi();
void saveSavedWiFi();
bool addSavedWiFi(const char* ssid, const char* pass);
bool removeSavedWiFi(const char* ssid);
void connectWiFiWithoutReboot(const char* ssid, const char* pass);
void updateWiFiReconnect();

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
char deviceId[32]     = "snooker_000000";
char apSSID[32]       = "Smarthive-000000";
const char* apPassword = "setup1234";

uint32_t getChipId() {
  uint64_t mac = ESP.getEfuseMac();
  return (uint32_t)(mac & 0xFFFFFF); // Bottom 24-bit unique silicon ID
}

uint32_t computeClaimPin(uint32_t chipId) {
  uint32_t hash = chipId ^ 0x5a3c9e71;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
  hash = (hash >> 16) ^ hash;
  return 100000 + (hash % 900000);
}

void initHardwareIdentity() {
  uint32_t chipId = getChipId();
  snprintf(deviceSerial, sizeof(deviceSerial), "SMP-%06X", chipId);
  snprintf(deviceId, sizeof(deviceId), "snooker_%06x", chipId);
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

// ==================== SNOOKER HARDWARE CONFIGURATION (ESP32 DEVKIT V1) ====================
// 1. RC522 RFID / NFC Reader (Hardware VSPI Bus):
// -------------------------------------------------------------------------
// Wire Connection:
//   3.3V (VCC) -> 3V3 Pin on ESP32 (CAUTION: NEVER 5V!)
//   GND        -> Common GND
//   RST        -> ESP32 GPIO 22
//   SDA (SS)   -> ESP32 GPIO 5  (Hardware VSPI SS)
//   SCK        -> ESP32 GPIO 18 (Hardware VSPI SCK)
//   MOSI       -> ESP32 GPIO 23 (Hardware VSPI MOSI)
//   MISO       -> ESP32 GPIO 19 (Hardware VSPI MISO)
// -------------------------------------------------------------------------
#define RC522_SS_PIN   5  // GPIO 5 (VSPI SS)
#define RC522_RST_PIN  22 // GPIO 22 (Reset)

MFRC522 mfrc522(RC522_SS_PIN, RC522_RST_PIN);
byte rc522FirmwareVersion = 0x00;
bool rfidWritePending = false;
unsigned long rfidWriteArmTime = 0;
const unsigned long RFID_WRITE_TIMEOUT_MS = 25000; // 25s window to present card
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
//   Orange / Yellow (PWM Signal) -> ESP32 GPIO 13 (Hardware LEDC PWM)
//   Red (+5V Power)              -> Direct 5V Power Rail (+ 470uF Capacitor [+])
//   Brown / Black (Ground)       -> Common GND          (- 470uF Capacitor [-])
// -------------------------------------------------------------------------
#define SERVO_PIN            13   // GPIO 13 (Hardware PWM)
#define SERVO_LOCKED_ANGLE   180  // Full 180 deg travel for ball gate release
#define SERVO_UNLOCKED_ANGLE 0    // Full open angle (0 deg)
#define UNLOCK_HOLD_MS       3000 // How long to stay open (milliseconds)

Servo lockServo;
bool servoUnlocked = false;
bool servoDetached = true;

// Non-blocking smooth soft-start motion state machine
enum ServoMotionState {
  SERVO_IDLE_DETACHED,
  SERVO_SWEEPING_OPEN,
  SERVO_HOLDING_OPEN,
  SERVO_SWEEPING_CLOSE,
  SERVO_SETTLING_CLOSE
};

ServoMotionState servoState = SERVO_IDLE_DETACHED;
int currentServoAngle = SERVO_LOCKED_ANGLE;
unsigned long lastServoStepTime = 0;
unsigned long servoHoldStartTime = 0;
unsigned long servoCloseSettleStartTime = 0;

// 3. Status Indicator (RGB LED):
// -------------------------------------------------------------------------
// Wire Connection (Common Cathode):
//   RGB Pin R (Red)   -> ESP32 GPIO 26
//   RGB Pin G (Green) -> ESP32 GPIO 27
//   RGB Pin - (GND)   -> ESP32 GND
// -------------------------------------------------------------------------
#define LED_RED_PIN        26 // GPIO 26 (Red Status Channel)
#define LED_GREEN_PIN      27 // GPIO 27 (Green Status Channel)
#define RGB_COMMON_CATHODE 1  // 1 = Common Cathode (GND pin, HIGH = ON); 0 = Common Anode (VCC pin, LOW = ON)

#ifndef LED_BUILTIN
#define LED_BUILTIN        2  // Standard onboard blue LED on ESP32 DevKit (GPIO 2)
#endif

// 4. Onboard Bench Test / Manual Ball Release Pushbutton (SW1):
#define TEST_BUTTON_PIN    32 // GPIO 32 (Tactile Switch with Internal Pull-Up to GND)

volatile unsigned long sessionRackCount = 0;
bool keepAliveEnabled = false;

// ==================== SMARTHIVE UNIFIED CLOUD BACKEND ====================
// Production domain pre-configured from factory. Customers NEVER need to type API URLs.
#define PRODUCTION_CLOUD_SYNC_URL "https://pool.smarthivetech.com/api/snooker/sync"

// Set to true while developing locally; set to false for production hardware release
#define USE_LOCAL_DEV             true
#define DEV_MACHINE_IP            "192.168.5.3"
#define DEV_CLOUD_SYNC_URL        "http://" DEV_MACHINE_IP ":3000/api/snooker/sync"

#if USE_LOCAL_DEV
char cloudApiUrl[160] = DEV_CLOUD_SYNC_URL;
#else
char cloudApiUrl[160] = PRODUCTION_CLOUD_SYNC_URL;
#endif

char cloudDeviceToken[64] = "smarthive_table_secret_2026";
bool cloudSyncPending = false;
bool isMaintenanceLocked = false;
unsigned long lastCloudSyncAttempt = 0;
const unsigned long CLOUD_HEARTBEAT_INTERVAL = 15000; // 15s heartbeat interval (balanced for TLS RAM stability & rapid cloud response)

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

#define MAX_OFFLINE_QUEUE 100
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

    // Load Optional Cloud Sync URL (preserves hardcoded default if EEPROM slot is blank)
    char eepromCloud[160];
    for (int i = 0; i < 159; i++) eepromCloud[i] = EEPROM.read(ADDR_CLOUD_URL + i);
    eepromCloud[159] = '\0';
    if ((uint8_t)eepromCloud[0] != 0xFF && strlen(eepromCloud) >= 8) {
      strncpy(cloudApiUrl, eepromCloud, sizeof(cloudApiUrl) - 1);
      cloudApiUrl[159] = '\0';
    }

    // Load Optional Cloud Device Token
    for (int i = 0; i < 63; i++) cloudDeviceToken[i] = EEPROM.read(ADDR_CLOUD_TOKEN + i);
    cloudDeviceToken[63] = '\0';
    if ((uint8_t)cloudDeviceToken[0] == 0xFF || strlen(cloudDeviceToken) == 0) {
      strcpy(cloudDeviceToken, "smarthive_table_secret_2026");
    }

    Serial.println("[EEPROM] Credentials loaded successfully");
  } else {
    // Initial configuration: prompt setup via AP without personal defaults
    routerSSID[0] = '\0';
    routerPassword[0] = '\0';
    strcpy(guiUsername, "admin");
    strcpy(guiPassword, "admin");
    cloudApiUrl[0] = '\0'; // Empty by default = pure standalone operation
    strcpy(cloudDeviceToken, "smarthive_table_secret_2026");
    saveCredentials();
    Serial.println("[EEPROM] First boot: initialized default standalone V1.0 configuration");
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
  if (strlen(cloudApiUrl) >= 8) {
    Serial.printf("[CLOUD] Configured sync endpoint: %s\n", cloudApiUrl);
  } else {
    Serial.println("[CLOUD] Mode: STANDALONE (Local operation, no external cloud sync)");
  }
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

  for (int i = 0; i < 159; i++) EEPROM.write(ADDR_CLOUD_URL + i, cloudApiUrl[i]);
  EEPROM.write(ADDR_CLOUD_URL + 159, 0);

  for (int i = 0; i < 63; i++) EEPROM.write(ADDR_CLOUD_TOKEN + i, cloudDeviceToken[i]);
  EEPROM.write(ADDR_CLOUD_TOKEN + 63, 0);

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
    Serial.println("\n[WIFI] Connection dropped! Auto-reconnecting without reboot...");
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
      Serial.println("[WIFI] Initial attempt timed out. Scanning for known saved networks in the air...");
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

// ==================== AUTH GUARD ====================
bool isSoftAPClient() {
  IPAddress clientIP = server.client().remoteIP();
  IPAddress apIP = WiFi.softAPIP();
  // Check if client is on the SoftAP subnet (192.168.4.x)
  return (clientIP[0] == apIP[0] && clientIP[1] == apIP[1] && clientIP[2] == apIP[2]);
}

bool requireAuth() {
  // If the client is connected directly to the hardware's WPA2-protected Access Point (192.168.4.x),
  // allow access so mobile captive portals (Android CaptivePortalLogin & iOS CNA) can display the dashboard
  // without failing on HTTP 401 Basic Auth prompts (which break mobile captive network detection).
  if (isSoftAPClient()) {
    return true;
  }
  if (!server.authenticate(guiUsername, guiPassword)) {
    server.requestAuthentication(BASIC_AUTH, "Smarthive_Pool_Hub", "Authentication Required");
    return false;
  }
  return true;
}

// ==================== HARDWARE RGB LED STATUS FEEDBACK ====================
void setRGB(bool red, bool green) {
#if RGB_COMMON_CATHODE
  digitalWrite(LED_RED_PIN, red ? HIGH : LOW);
  digitalWrite(LED_GREEN_PIN, green ? HIGH : LOW);
#else
  digitalWrite(LED_RED_PIN, red ? LOW : HIGH);
  digitalWrite(LED_GREEN_PIN, green ? LOW : HIGH);
#endif
}

void updateLED() {
  unsigned long now = millis();

  // 0. Maintenance Locked: Slow Warning Red Pulse
  if (isMaintenanceLocked) {
    bool redPulse = ((now / 600) % 2 == 0);
    setRGB(redPulse, false);
    return;
  }

  // 1. Table Unlocked / Authorized Card Detected: SOLID GREEN!
  // Stays bright solid green while the servo unlocks, holds the ball gate open, and returns
  if (servoUnlocked || servoState != SERVO_IDLE_DETACHED) {
    setRGB(false, true); // Solid Green ON, Red OFF
    return;
  }

  // 2. Reader Armed in Programming Mode: Amber / Yellow pulse (Red + Green both active)
  if (rfidWritePending) {
    bool armedPulse = ((now / 250) % 2 == 0); // 4Hz amber pulse
    setRGB(armedPulse, armedPulse);
    return;
  }
  
  // 3. Online Heartbeat (Connected to Router): Green pulse every 2.5s, Red OFF
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long phase = now % 2500;
    setRGB(false, phase < 60);
    return;
  }
  
  // 4. Offline / SoftAP / Connecting Mode
  unsigned long interval = (strlen(routerSSID) > 0) ? 500 : 800;
  if (now - lastLedToggle >= interval) {
    lastLedToggle = now;
    ledState = !ledState;
  }
  setRGB(ledState, false); // Gentle Red blink, Green OFF
}

// ==================== CAPTIVE PORTAL HANDLER ====================
void handleNotFound() {
  String host = server.hostHeader();
  String apIp = WiFi.softAPIP().toString();

  // If request is not targeted directly at our AP IP or mDNS hostname, redirect to captive portal landing
  if (host != apIp && host != "smarthive.local" && host != (apIp + ":80") && host != "smarthive.local:80") {
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
  json += "\"heap_frag\":0,";
  json += "\"uptime_secs\":" + String(millis() / 1000) + ",";
  json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"chip_id\":\"" + String(getChipId(), HEX) + "\",";
  json += "\"game_count\":" + String(sessionRackCount) + ",";
  json += "\"button_count\":" + String(sessionRackCount) + ",";
  json += "\"last_trigger\":\"" + escapeJson(lastTriggerSource) + "\",";
  json += "\"last_card_uid\":\"" + escapeJson(lastScannedUid) + "\",";
  json += "\"staff_name\":\"" + escapeJson(lastStaffName) + "\",";
  json += "\"staff_id\":\"" + escapeJson(lastStaffId) + "\",";
  json += "\"rfid_write_status\":\"" + escapeJson(rfidWriteStatus) + "\",";
  json += "\"rc522_version\":\"0x" + String(rc522FirmwareVersion, HEX) + "\",";
  json += "\"rc522_connected\":" + String((rc522FirmwareVersion != 0x00 && rc522FirmwareVersion != 0xFF) ? "true" : "false") + ",";
  json += "\"servo_state\":\"" + String(servoUnlocked ? "UNLOCKED" : "LOCKED") + "\",";
  json += "\"keepalive_enabled\":" + String(keepAliveEnabled ? "true" : "false") + ",";
  json += "\"offline_queue_count\":" + String(offlineQueueCount) + ",";
  json += "\"authorized_cards_count\":" + String(authorizedCardsCount) + ",";
  json += "\"boot_count\":" + String(bootCount) + ",";
  json += "\"firmware_version\":\"" CURRENT_FIRMWARE_VERSION "\",";
  json += "\"cloud_configured\":" + String(strlen(cloudApiUrl) >= 8 ? "true" : "false") + ",";
  json += "\"cloud_url\":\"" + escapeJson(String(cloudApiUrl)) + "\"";
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
    Serial.println("[REPORTS] Initialized fresh daily records schema in EEPROM");
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
  Serial.println("[REPORTS] Daily reports history manually cleared via Web UI");
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
    Serial.println("[RC522-WRITE] Session timed out");
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
  Serial.println("[RC522-WRITE] Card programming cancelled");
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
  if (EEPROM.read(ADDR_AUTH_CARDS_FLAG) == AUTH_CARDS_FLAG_VAL) {
    uint8_t c = EEPROM.read(ADDR_AUTH_CARDS_COUNT);
    if (c > MAX_PERSISTED_AUTH_CARDS) c = 0;
    authorizedCardsCount = c;
    for (uint8_t i = 0; i < c; i++) {
      EEPROM.get(ADDR_AUTH_CARDS_START + (i * sizeof(AuthorizedCardCache)), authorizedCardsList[i]);
      authorizedCardsList[i].uid[sizeof(authorizedCardsList[i].uid) - 1] = '\0';
      authorizedCardsList[i].name[sizeof(authorizedCardsList[i].name) - 1] = '\0';
      authorizedCardsList[i].id[sizeof(authorizedCardsList[i].id) - 1] = '\0';
      authorizedCardsList[i].role[sizeof(authorizedCardsList[i].role) - 1] = '\0';
    }
    Serial.printf("[AUTH-CARDS] Loaded %d cached authorized cards from EEPROM\n", authorizedCardsCount);
  }
}

void saveAuthorizedCards() {
  EEPROM.write(ADDR_AUTH_CARDS_FLAG, AUTH_CARDS_FLAG_VAL);
  uint8_t persistCount = (authorizedCardsCount > MAX_PERSISTED_AUTH_CARDS) ? MAX_PERSISTED_AUTH_CARDS : (uint8_t)authorizedCardsCount;
  EEPROM.write(ADDR_AUTH_CARDS_COUNT, persistCount);
  for (uint8_t i = 0; i < persistCount; i++) {
    EEPROM.put(ADDR_AUTH_CARDS_START + (i * sizeof(AuthorizedCardCache)), authorizedCardsList[i]);
  }
  EEPROM.commit();
}

void updateAuthorizedCardsFromJson(const String& json) {
  int arrayStart = json.indexOf("\"authorizedCards\":[");
  if (arrayStart == -1) return;
  int arrayEnd = json.indexOf("]", arrayStart);
  if (arrayEnd == -1) return;

  String cardsBlock = json.substring(arrayStart + 18, arrayEnd);
  AuthorizedCardCache prevCards[MAX_AUTHORIZED_CARDS];
  int prevCount = authorizedCardsCount;
  memcpy(prevCards, authorizedCardsList, sizeof(prevCards));
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
      memset(&authorizedCardsList[count], 0, sizeof(AuthorizedCardCache));
      strncpy(authorizedCardsList[count].uid, uid.c_str(), sizeof(authorizedCardsList[count].uid) - 1);
      strncpy(authorizedCardsList[count].name, name.c_str(), sizeof(authorizedCardsList[count].name) - 1);
      strncpy(authorizedCardsList[count].id, id.c_str(), sizeof(authorizedCardsList[count].id) - 1);
      strncpy(authorizedCardsList[count].role, role.c_str(), sizeof(authorizedCardsList[count].role) - 1);
      count++;
    }

    pos = objEnd + 1;
  }

  if (count > 0 || cardsBlock.indexOf("{") == -1) {
    authorizedCardsCount = count;
    Serial.printf("[AUTH-CARDS] Synced %d authorized cards from Cloud.\n", authorizedCardsCount);
    if (authorizedCardsCount != prevCount || memcmp(prevCards, authorizedCardsList, authorizedCardsCount * sizeof(AuthorizedCardCache)) != 0) {
      saveAuthorizedCards();
    }
  }
}

void updateRfidWriteCmdFromJson(const String& json) {
  int cmdPos = json.indexOf("\"rfidWriteCmd\":");
  if (cmdPos == -1) return;
  int objStart = json.indexOf("{", cmdPos);
  if (objStart == -1) return;
  int objEnd = json.indexOf("}", objStart);
  if (objEnd == -1) return;

  String cmdObj = json.substring(objStart, objEnd + 1);

  bool arm = (cmdObj.indexOf("\"arm\":true") != -1);
  if (arm) {
    auto extractVal = [&](const char* key) -> String {
      int k = cmdObj.indexOf(String("\"") + key + "\":\"");
      if (k == -1) return "";
      int vStart = k + strlen(key) + 4;
      int vEnd = cmdObj.indexOf("\"", vStart);
      if (vEnd == -1) return "";
      return cmdObj.substring(vStart, vEnd);
    };

    String name = extractVal("staffName");
    String id = extractVal("staffId");

    if (name.length() > 0 && id.length() > 0) {
      if (!rfidWritePending || strcmp(pendingStaffName, name.c_str()) != 0 || strcmp(pendingStaffId, id.c_str()) != 0) {
        strncpy(pendingStaffName, name.c_str(), 15);
        pendingStaffName[15] = '\0';
        strncpy(pendingStaffId, id.c_str(), 15);
        pendingStaffId[15] = '\0';
        rfidWritePending = true;
        rfidWriteArmTime = millis();
        rfidWriteStatus = "WAITING_FOR_CARD";
        rfidWriteMessage = "Cloud armed reader for " + name + " (" + id + ")";
        Serial.printf("[CLOUD-RFID] Reader ARMED by Cloud to program: %s (%s)\n", pendingStaffName, pendingStaffId);
      }
    }
  } else {
    // Cloud disarmed / timed out / cancelled
    if (rfidWritePending) {
      rfidWritePending = false;
      rfidWriteStatus = "IDLE";
      rfidWriteMessage = "Disarmed by cloud";
      Serial.println("[CLOUD-RFID] Reader disarmed by cloud");
    }
  }
}

// ==================== CLOUD COMMAND PARSER (MAINTENANCE LOCK & REMOTE UNLOCK) ====================
void parseCloudCommands(const String& json) {
  // 0. Sync system clock from serverEpoch (works even when UDP NTP port 123 is blocked)
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

  // 1. Maintenance Lock Parser
  if (json.indexOf("\"maintenanceLocked\":true") != -1) {
    if (!isMaintenanceLocked) {
      isMaintenanceLocked = true;
      Serial.println("\n**************************************************");
      Serial.println("[MAINTENANCE] *** LOCK ENGAGED BY CLOUD DASHBOARD ***");
      Serial.println("[MAINTENANCE] Ball gate and RFID unlocks are SUSPENDED.");
      Serial.println("**************************************************\n");
      setRGB(true, false);
    }
  } else if (json.indexOf("\"maintenanceLocked\":false") != -1) {
    if (isMaintenanceLocked) {
      isMaintenanceLocked = false;
      Serial.println("\n**************************************************");
      Serial.println("[MAINTENANCE] *** LOCK RELEASED BY CLOUD DASHBOARD ***");
      Serial.println("[MAINTENANCE] Table operations are RESTORED.");
      Serial.println("**************************************************\n");
      setRGB(false, false);
    }
  }

  // 2. Remote Unlock Solenoid Command Parser
  int unlockIdx = json.indexOf("\"unlockCmd\":");
  if (unlockIdx != -1) {
    int pendingIdx = json.indexOf("\"pending\":true", unlockIdx);
    if (pendingIdx != -1 && pendingIdx < unlockIdx + 80) {
      if (isMaintenanceLocked) {
        Serial.println("\n[REMOTE-UNLOCK] REJECTED: Table is locked under Maintenance Lock.");
      } else {
        Serial.println("\n========================================");
        Serial.println("[REMOTE-UNLOCK] *** DISPATCH RECEIVED FROM CLOUD DASHBOARD ***");
        Serial.println("[REMOTE-UNLOCK] Triggering Ball Gate Solenoid / Servo...");
        Serial.println("========================================\n");
        sessionRackCount++;
        saveRackCounter();
        recordRackCount();
        lastTriggerSource = "Cloud Remote Unlock";
        lastScannedUid = "CLOUD_REMOTE";
        setRGB(false, true);
        triggerServoDoorLock();
        queueOfflineEvent("remote_unlock", "CLOUD_REMOTE");
      }
    }
  }
}

// ==================== CLOUD REMOTE OTA UPDATER ====================
bool otaUpdateInProgress = false;

void executeCloudOta(const String& firmwareUrl, const String& targetVersion) {
  if (otaUpdateInProgress) return;
  otaUpdateInProgress = true;

  Serial.println("\n========================================");
  Serial.println("[CLOUD_OTA] Remote update initiated!");
  Serial.printf ("            Target Version: %s\n", targetVersion.c_str());
  Serial.printf ("            Download URL:   %s\n", firmwareUrl.c_str());
  Serial.println("========================================");

  // Indicate OTA in progress with Amber status LED (Red + Green ON)
  setRGB(true, true);

  httpUpdate.onStart([]() {
    Serial.println("[CLOUD_OTA] Flash partition opened, streaming binary...");
  });
  httpUpdate.onEnd([]() {
    Serial.println("[CLOUD_OTA] Flash complete! Rebooting into new firmware...");
  });
  httpUpdate.onProgress([](int cur, int total) {
    if (total > 0) {
      Serial.printf("[CLOUD_OTA] Flashing: %d%% (%d / %d bytes)\n", (cur * 100) / total, cur, total);
    }
  });
  httpUpdate.onError([](int err) {
    Serial.printf("[CLOUD_OTA] Error code: %d\n", err);
  });

  httpUpdate.rebootOnUpdate(true); // Automatically boot into new partition on completion

  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  t_httpUpdate_return ret;

  if (firmwareUrl.startsWith("https://")) {
    secureClient.setInsecure(); // Support TLS without certificate pinning
    ret = httpUpdate.update(secureClient, firmwareUrl);
  } else {
    ret = httpUpdate.update(plainClient, firmwareUrl);
  }

  otaUpdateInProgress = false;

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("[CLOUD_OTA] FAILED! Error (%d): %s\n",
                    httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      setRGB(true, false); // Red indicator
      sendCloudEvent("ota_failed", httpUpdate.getLastErrorString().c_str());
      delay(2000);
      setRGB(false, false);
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[CLOUD_OTA] Server reported no updates available.");
      setRGB(false, false);
      break;

    case HTTP_UPDATE_OK:
      Serial.println("[CLOUD_OTA] SUCCESS! Restarting device...");
      setRGB(false, true); // Green indicator
      // ESP32 will reboot automatically
      break;
  }
}

void checkForCloudOta(const String& json) {
  int cmdPos = json.indexOf("\"otaCmd\":");
  if (cmdPos == -1) return;
  int objStart = json.indexOf("{", cmdPos);
  if (objStart == -1) return;
  int objEnd = json.indexOf("}", objStart);
  if (objEnd == -1) return;

  String cmdObj = json.substring(objStart, objEnd + 1);

  bool pending = (cmdObj.indexOf("\"pending\":true") != -1);
  if (!pending) return;

  auto extractVal = [&](const char* key) -> String {
    int k = cmdObj.indexOf(String("\"") + key + "\":\"");
    if (k == -1) return "";
    int vStart = k + strlen(key) + 4;
    int vEnd = cmdObj.indexOf("\"", vStart);
    if (vEnd == -1) return "";
    return cmdObj.substring(vStart, vEnd);
  };

  String url = extractVal("firmwareUrl");
  String version = extractVal("targetVersion");

  if (url.length() < 8) return;

  // Safety check: Never flash if servo is actively sweeping open or closed
  if (servoState != SERVO_IDLE_DETACHED) {
    Serial.println("[CLOUD_OTA] Deferring OTA update: servo ball release is currently active");
    return;
  }

  Serial.printf("[CLOUD_OTA] Update request detected: Target=%s, Current=%s\n",
                version.c_str(), CURRENT_FIRMWARE_VERSION);

  executeCloudOta(url, version);
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
    Serial.println("[SECURITY] Initialized fresh blocked cards registry in EEPROM");
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
  json.reserve(384);
  json += "\"ssid\":\"" + escapeJson(String(routerSSID)) + "\",";
  json += "\"pass\":\"" + escapeJson(String(routerPassword)) + "\",";
  json += "\"user\":\"" + escapeJson(String(guiUsername)) + "\",";
  json += "\"auth\":\"" + escapeJson(String(guiPassword)) + "\",";
  json += "\"cloud_url\":\"" + escapeJson(String(cloudApiUrl)) + "\",";
  json += "\"cloud_token\":\"" + escapeJson(String(cloudDeviceToken)) + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleScan() {
  int n = WiFi.scanNetworks(false, true);
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
  if (!requireAuth()) return;

  String newSSID  = server.arg("ssid");
  String newPass  = server.arg("pass");
  String newUser  = server.arg("user");
  String newAuth  = server.arg("auth");
  String newCloud = server.arg("cloud_url");
  String newToken = server.arg("cloud_token");

  // Input length guards
  if (newSSID.length() > 32 || newPass.length() > 64 || newUser.length() > 16 || newAuth.length() > 16 ||
      newCloud.length() > 159 || newToken.length() > 63) {
    server.send(400, "text/plain", "Error: Input exceeds maximum allowed length.");
    return;
  }
  if (newUser.length() == 0 || newAuth.length() == 0) {
    server.send(400, "text/plain", "Error: Username and Password cannot be empty.");
    return;
  }

  strncpy(guiUsername,    newUser.c_str(), 16); guiUsername[16]    = '\0';
  strncpy(guiPassword,    newAuth.c_str(), 16); guiPassword[16]    = '\0';

  newCloud.trim();
  if (newCloud.length() >= 8) {
    strncpy(cloudApiUrl, newCloud.c_str(), 159);
    cloudApiUrl[159] = '\0';
  }

  newToken.trim();
  if (newToken.length() > 0) {
    strncpy(cloudDeviceToken, newToken.c_str(), 63);
    cloudDeviceToken[63] = '\0';
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

  Serial.println("[HTTP] Configuration updated and live WiFi switch initiated without reboot.");
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
  Serial.println("[SYSTEM] Soft restart initiated via API...");
  delay(800);
  ESP.restart();
}

void handleFactoryReset() {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 0xFF);
  }
  EEPROM.commit();
  server.send(200, "application/json", "{\"status\":\"resetting\"}");
  Serial.println("[SYSTEM] Factory reset executed. EEPROM cleared. Restarting...");
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
      mfrc522.PCD_Init(); // Re-initialize registers & RF antenna
      mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
      mfrc522.PCD_AntennaOn();
      delay(50);
      Serial.print("[RC522] RFID reader CONNECTED / DETECTED! Version: 0x");
      Serial.println(rc522FirmwareVersion, HEX);
    } else if (!isConnected) {
      if (wasConnected) {
        Serial.println("[RC522] RFID reader LOST! Attempting active hardware RST recovery...");
      }
      pinMode(RC522_RST_PIN, OUTPUT);
      digitalWrite(RC522_RST_PIN, LOW);
      delay(5);
      digitalWrite(RC522_RST_PIN, HIGH);
      delay(15);
      mfrc522.PCD_Init();
      mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
      mfrc522.PCD_AntennaOn();
      byte recoveredVer = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
      rc522FirmwareVersion = recoveredVer;
      if (recoveredVer != 0x00 && recoveredVer != 0xFF) {
        Serial.printf("[RC522] Hardware RST recovery SUCCEEDED (Version: 0x%02X)\n", recoveredVer);
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

  Serial.println("\n[RC522] >>> RFID / NFC Card Tap Detected! <<<");
  Serial.printf("        Card UID: %s\n", cardUid.c_str());

  // SECURITY CHECK: Is this card stolen or blacklisted?
  if (isCardBlocked(cardUid)) {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    lastScannedUid = cardUid;
    lastStaffName = "BLOCKED CARD";
    lastStaffId = "REVOKED";
    lastTriggerSource = "BLOCKED CARD REJECTED (" + cardUid + ")";

    Serial.println("\n[SECURITY ALERT] >>> BLOCKED / STOLEN CARD DETECTED! <<<");
    Serial.printf("                  Card UID: %s (Access: DENIED)\n", cardUid.c_str());

    for (int b = 0; b < 6; b++) {
      setRGB(true, false);
      delay(40);
      setRGB(false, false);
      delay(40);
    }

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

  // Check Maintenance Lock:
  if (isMaintenanceLocked) {
    Serial.println("\n[MAINTENANCE] >>> ACCESS DENIED: TABLE IS CURRENTLY IN MAINTENANCE LOCK! <<<");
    Serial.printf("               Card UID: %s (Ignored - Table out of service)\n", cardUid.c_str());
    for (int b = 0; b < 3; b++) {
      setRGB(true, false); delay(70);
      setRGB(false, false); delay(70);
    }
    return;
  }

  // SECURITY CHECK: AUTHORIZED CARDS ONLY!
  if (!isCardAuthorized(cardUid, cardStaffName, cardStaffId)) {
    Serial.println("\n[SECURITY ALERT] >>> UNAUTHORIZED / RANDOM CARD REJECTED! <<<");
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

    queueOfflineEvent("unauthorized_card_attempt", cardUid.c_str());
    return; // Completely stop: door lock remains CLOSED, count does NOT increment!
  }

  if (lastRackTriggerTime != 0 && (millis() - lastRackTriggerTime < RACK_LOCKOUT_MS)) {
    Serial.printf("[RC522] Anti-jiggle lockout active (%lums remaining). Ignored card tap.\n",
                  RACK_LOCKOUT_MS - (millis() - lastRackTriggerTime));
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

  Serial.printf("[SNOOKER] Authorized Card Unlocked! Session total: %lu\n", sessionRackCount);

  // Turn RGB LED Solid Green for Authorized Game Unlock
  setRGB(false, true);

  // Queue event and trigger Servo Door Lock (non-blocking; loop() flushes once servo detaches)
  queueOfflineEvent("button_press", cardUid.c_str());
  triggerServoDoorLock();

  // Confirm reader is still healthy and ready for next tap
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  if (v == 0x00 || v == 0xFF) {
    Serial.println("[RC522] WARNING: Reader communication lost after servo/cloud! (Likely brownout/power drop)");
  } else {
    Serial.printf("[RC522] Ready for next card tap (Reader status: OK, Version: 0x%02X)\n", v);
    Serial.println("        Tip: Lift card off reader and tap again.");
  }
}

// ==================== MG996R SERVO CONTROLLER (SMOOTH SOFT-START) ====================
// Micro-stepped non-blocking sweep eliminates the 2.5A locked-rotor inrush spike,
// preventing power rail dips, voltage drops, and ESP brownout reboots.
#define SERVO_STEP_DEG         4   // Degrees per step
#define SERVO_STEP_INTERVAL_MS 12  // Milliseconds between steps (~350ms total travel)

void triggerServoDoorLock() {
  if (servoState == SERVO_IDLE_DETACHED) {
    lockServo.attach(SERVO_PIN);
    currentServoAngle = SERVO_LOCKED_ANGLE;
    lockServo.write(currentServoAngle);
    servoDetached = false;
  }
  servoState = SERVO_SWEEPING_OPEN;
  servoUnlocked = true;
  lastServoStepTime = millis();
  Serial.printf("[SERVO] Smooth soft-start: sweeping from %d deg to %d deg (Door Unlocking)\n", currentServoAngle, SERVO_UNLOCKED_ANGLE);
}

void updateServo() {
  unsigned long now = millis();

  switch (servoState) {
    case SERVO_IDLE_DETACHED:
      // Motor detached - 0mA idle current draw
      break;

    case SERVO_SWEEPING_OPEN:
      if (now - lastServoStepTime >= SERVO_STEP_INTERVAL_MS) {
        lastServoStepTime = now;
        if (currentServoAngle > SERVO_UNLOCKED_ANGLE) {
          currentServoAngle -= SERVO_STEP_DEG;
          if (currentServoAngle < SERVO_UNLOCKED_ANGLE) currentServoAngle = SERVO_UNLOCKED_ANGLE;
          lockServo.write(currentServoAngle);
        } else {
          // Reached full open
          servoState = SERVO_HOLDING_OPEN;
          servoHoldStartTime = now;
          Serial.printf("[SERVO] Full open reached (%d deg). Holding for %ds\n", currentServoAngle, UNLOCK_HOLD_MS / 1000);
        }
      }
      break;

    case SERVO_HOLDING_OPEN:
      if (now - servoHoldStartTime >= UNLOCK_HOLD_MS) {
        servoState = SERVO_SWEEPING_CLOSE;
        servoUnlocked = false;
        lastServoStepTime = now;
        Serial.printf("[SERVO] Unlock hold expired. Smooth sweeping back to locked (%d deg)\n", SERVO_LOCKED_ANGLE);
      }
      break;

    case SERVO_SWEEPING_CLOSE:
      if (now - lastServoStepTime >= SERVO_STEP_INTERVAL_MS) {
        lastServoStepTime = now;
        if (currentServoAngle < SERVO_LOCKED_ANGLE) {
          currentServoAngle += SERVO_STEP_DEG;
          if (currentServoAngle > SERVO_LOCKED_ANGLE) currentServoAngle = SERVO_LOCKED_ANGLE;
          lockServo.write(currentServoAngle);
        } else {
          // Reached full lock
          servoState = SERVO_SETTLING_CLOSE;
          servoCloseSettleStartTime = now;
        }
      }
      break;

    case SERVO_SETTLING_CLOSE:
      // Allow 400ms for final mechanical gear train settling under load before cutting PWM
      if (now - servoCloseSettleStartTime >= 400) {
        lockServo.detach();
        servoDetached = true;
        servoState = SERVO_IDLE_DETACHED;
        Serial.println("[SERVO] Motor detached at locked position (0mA idle current, brownout safe)");
      }
      break;
  }
}

// ==================== MANUAL BENCH TEST / RELEASE BUTTON ====================
void updateTestButton() {
  static int lastBtnReading = HIGH;
  static int debouncedBtnState = HIGH;
  static unsigned long lastBtnDebounce = 0;

  int reading = digitalRead(TEST_BUTTON_PIN);
  if (reading != lastBtnReading) {
    lastBtnDebounce = millis();
  }

  if ((millis() - lastBtnDebounce) > 50) {
    if (reading != debouncedBtnState) {
      debouncedBtnState = reading;
      if (debouncedBtnState == LOW) { // Pressed (pulled to GND)
        if (isMaintenanceLocked) {
          Serial.println("[MAINTENANCE] Manual bench button ignored: Table is in Maintenance Lock.");
          return;
        }
        Serial.println("[BUTTON] Manual Bench Test Button pressed (GPIO 32)");
        lastTriggerSource = "Manual Test Button";
        lastScannedUid = "MANUAL_BTN";
        sessionRackCount++;
        recordRackCount();
        setRGB(false, true); // Solid Green during release
        triggerServoDoorLock();
        sendCloudEvent("button_press", "MANUAL_BTN");
      }
    }
  }
  lastBtnReading = reading;
}

void loadRackCounter() {
  if (EEPROM.read(ADDR_RACK_COUNTER_FLAG) == RACK_COUNTER_FLAG_VAL) {
    EEPROM.get(ADDR_RACK_COUNTER_VAL, sessionRackCount);
    Serial.printf("[COUNTER] Restored persisted session rack count from EEPROM: %lu\n", sessionRackCount);
  } else {
    sessionRackCount = 0;
  }
}

void saveRackCounter() {
  EEPROM.write(ADDR_RACK_COUNTER_FLAG, RACK_COUNTER_FLAG_VAL);
  EEPROM.put(ADDR_RACK_COUNTER_VAL, sessionRackCount);
  EEPROM.commit();
}

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
  Serial.printf("[BOOT] Boot count: %lu (Reason: %d)\n", (unsigned long)bootCount, (int)esp_reset_reason());
}

void handleCounterReset() {
  sessionRackCount = 0;
  saveRackCounter();
  lastTriggerSource = "Reset via Web";
  lastScannedUid = "None";
  Serial.println("[COUNTER] Session count reset to 0 via Web UI");
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
  digitalWrite(LED_BUILTIN, HIGH); // LED pulse on

  // Fast WiFi broadcast packet triggers RF power amplifier burst
  WiFiUDP udp;
  udp.beginPacket("255.255.255.255", 9);
  udp.write((const uint8_t*)"HB", 2);
  udp.endPacket();

  delay(40);
  digitalWrite(LED_BUILTIN, LOW); // LED pulse off
}

// ==================== OFFLINE EVENT SPOOLING QUEUE ====================
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

  String url = String(cloudApiUrl);
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
    secureClient.setInsecure();
    beginSuccess = http.begin(secureClient, url);
  } else {
    beginSuccess = http.begin(plainClient, url);
  }

  if (beginSuccess) {
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-device-token", cloudDeviceToken);
    http.setTimeout(12000); // 12s timeout to accommodate cloud latency / server response

    String payload = "{\"device_id\":\"" + String(deviceId) + "\",\"serial_number\":\"" + String(deviceSerial) + "\",\"claim_pin\":\"" + String(claimPinStr) + "\",\"batch\":[";
    for (int i = 0; i < offlineQueueCount; i++) {
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
    payload += "\"firmware_version\":\"" CURRENT_FIRMWARE_VERSION "\",";
    payload += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    payload += "\"reset_reason\":\"esp32_code_" + String((int)esp_reset_reason()) + "\",";
    payload += "\"rc522_ok\":" + String((rc522FirmwareVersion != 0x00 && rc522FirmwareVersion != 0xFF) ? "true" : "false") + ",";
    payload += "\"offline_queue_count\":" + String(offlineQueueCount) + ",";
    payload += "\"boot_count\":" + String(bootCount) + "}";

    int code = http.POST(payload);
    if (code >= 200 && code < 300) {
      Serial.printf("[OFFLINE-QUEUE] Batch sync accepted by cloud (%d games recovered)\n", offlineQueueCount);
      offlineQueueCount = 0;
      saveOfflineQueue();
      cloudSyncPending = false;
      String response = http.getString();
      updateAuthorizedCardsFromJson(response);
      updateRfidWriteCmdFromJson(response);
      checkForCloudOta(response);
      parseCloudCommands(response);
    } else {
      Serial.printf("[OFFLINE-QUEUE] Batch flush failed (HTTP %d). Will retry.\n", code);
    }
    http.end();
    if (isHttps) secureClient.stop();
  }
  lastCloudSyncAttempt = millis();
}

// ==================== PURE STATELESS TRANSCEIVER LOGIC ====================
void sendCloudEvent(const char* eventType, const char* cardUid) {
  String url = String(cloudApiUrl);
  url.trim();
  if (url.length() < 8) return; // Standalone mode: no external cloud sync configured

  if (WiFi.status() != WL_CONNECTED) {
    if (strcmp(eventType, "button_press") == 0) {
      queueOfflineEvent(eventType, cardUid);
      cloudSyncPending = true;
    }
    return;
  }

  Serial.printf("[CLOUD] Transmitting %s event -> %s\n", eventType, url.c_str());

  bool isHttps = url.startsWith("https://");
  HTTPClient http;
  bool beginSuccess = false;

  WiFiClient plainClient;
  WiFiClientSecure secureClient;

  if (isHttps) {
    secureClient.setInsecure(); // Skip SSL thumbprint validation
    beginSuccess = http.begin(secureClient, url);
  } else {
    beginSuccess = http.begin(plainClient, url);
  }

  if (beginSuccess) {
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("x-device-token", cloudDeviceToken);
    http.setTimeout(12000); // 12s timeout for cloud response / TLS handshake

    // Pure stateless payload: sends event, trigger source, optional card uid, device id, rssi, and ip
    String payload = "{";
    payload += "\"device_id\":\"" + String(deviceId) + "\",";
    payload += "\"serial_number\":\"" + String(deviceSerial) + "\",";
    payload += "\"claim_pin\":\"" + String(claimPinStr) + "\",";
    payload += "\"firmware_version\":\"" CURRENT_FIRMWARE_VERSION "\",";
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
    payload += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    payload += "\"reset_reason\":\"esp32_code_" + String((int)esp_reset_reason()) + "\",";
    payload += "\"rc522_ok\":" + String((rc522FirmwareVersion != 0x00 && rc522FirmwareVersion != 0xFF) ? "true" : "false") + ",";
    payload += "\"offline_queue_count\":" + String(offlineQueueCount) + ",";
    payload += "\"boot_count\":" + String(bootCount);
    payload += "}";

    int httpCode = http.POST(payload);
    if (httpCode >= 200 && httpCode < 300) {
      Serial.printf("[CLOUD] Event '%s' accepted by Cloud (HTTP %d)\n", eventType, httpCode);
      if (strcmp(eventType, "button_press") == 0 || strcmp(eventType, "rack_unlock") == 0) {
        cloudSyncPending = false;
      }
      String response = http.getString();
      updateAuthorizedCardsFromJson(response);
      updateRfidWriteCmdFromJson(response);
      checkForCloudOta(response);
      parseCloudCommands(response);
    } else {
      Serial.printf("[CLOUD] Event failed (HTTP %d): %s\n", httpCode, http.errorToString(httpCode).c_str());
    }
    http.end();
    if (isHttps) secureClient.stop();
  } else {
    Serial.println("[CLOUD] Failed to begin HTTP connection");
  }

  lastCloudSyncAttempt = millis();
}

// ==================== SETUP ====================
void setup() {
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  setRGB(false, false); // Start with LEDs OFF
  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP); // Onboard bench test button on GPIO 32
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(115200);
  delay(500);

  // Initialize hardware identity derived from silicon Chip ID
  initHardwareIdentity();

  Serial.println("\n\n========================================");
  Serial.println("   Smarthive Pool Controller (ESP32)");
  Serial.printf ("   Serial:     %s\n", deviceSerial);
  Serial.printf ("   Claim PIN:  %s\n", claimPinStr);
  Serial.printf ("   Device ID:  %s\n", deviceId);
  Serial.printf ("   SoftAP:     %s\n", apSSID);
  Serial.println("========================================");

  // 1. Initialize SPI bus and RC522 RFID / NFC Reader on VSPI (Pins 18, 19, 23, 5)
  SPI.begin(18, 19, 23, RC522_SS_PIN);
  delay(30);

  activeCsPin = RC522_SS_PIN;
  mfrc522.PCD_Init(activeCsPin, RC522_RST_PIN);
  delay(40);
  rc522FirmwareVersion = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

  // Maximize antenna gain (48dB) for reliable card detection & ensure antenna is ON
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  mfrc522.PCD_AntennaOn();

  Serial.print("[RC522] RFID Reader Version: 0x");
  Serial.print(rc522FirmwareVersion, HEX);
  if (rc522FirmwareVersion == 0x91 || rc522FirmwareVersion == 0x92) {
    Serial.println(" (Genuine RC522 connected on VSPI -> Pin 5, ready)");
  } else if (rc522FirmwareVersion == 0x12) {
    Serial.println(" (Clone RC522 connected on VSPI -> Pin 5, ready)");
  } else if (rc522FirmwareVersion == 0x00 || rc522FirmwareVersion == 0xFF) {
    Serial.println(" (WARNING: Reader NOT detected! Check 3.3V, GND, MOSI=23, MISO=19, SCK=18, SDA=5, RST=22)");
  } else {
    Serial.println(" (Custom/Unknown Version)");
  }

  // 2. Initialize MG996R Servo with ESP32 Hardware PWM Timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  lockServo.setPeriodHertz(50); // Standard 50Hz servo
  lockServo.attach(SERVO_PIN, 500, 2500);
  lockServo.write(SERVO_LOCKED_ANGLE);
  delay(500);
  lockServo.detach();
  servoDetached = true;
  servoState = SERVO_IDLE_DETACHED;
  currentServoAngle = SERVO_LOCKED_ANGLE;
  Serial.printf("[SERVO] Initialized on Pin %d - Locked at %d deg & Detached (0mA idle current)\n", SERVO_PIN, SERVO_LOCKED_ANGLE);

  loadCredentials();
  loadBootCount();
  littleFsReady = LittleFS.begin(true);
  Serial.printf("[FS] LittleFS mount: %s\n", littleFsReady ? "OK" : "Unavailable (using EEPROM fallback)");
  loadDailyRecords();
  loadRackCounter();
  loadOfflineQueue();
  loadAuthorizedCards();
  loadBlockedCards();
  configTime(3600, 0, "pool.ntp.org", "time.google.com"); // UTC+1 (WAT / Africa/Lagos)

  WiFi.mode(WIFI_AP_STA);
  delay(100);

  // Explicitly configure SoftAP IP, Gateway, and Subnet to ensure phone DHCP succeeds reliably
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apIP, netMsk);

  bool apStarted = WiFi.softAP(apSSID, apPassword);
  if (apStarted) {
    Serial.println("[AP] Access Point ACTIVE");
    Serial.print("     SSID: ");     Serial.println(apSSID);
    Serial.print("     Password: "); Serial.println(apPassword);
    Serial.print("     AP IP: ");    Serial.println(WiFi.softAPIP());

    // Start DNS server redirecting all domain requests to AP IP for captive portal
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);
    Serial.println("[DNS] Captive Portal DNS Server started");
  } else {
    Serial.println("[AP] Access Point FAILED to start");
  }

  // Attempt router connection only if configured
  if (strlen(routerSSID) > 0) {
    Serial.print("\n[STA] Connecting to router: ");
    Serial.println(routerSSID);
    WiFi.begin(routerSSID, routerPassword);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(300);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n[STA] Router Connected!");
      Serial.print("      IP: ");   Serial.println(WiFi.localIP());
      Serial.print("      RSSI: "); Serial.print(WiFi.RSSI()); Serial.println(" dBm");
    } else {
      Serial.println("\n[STA] Router connection timed out — keeping SoftAP active");
      WiFi.disconnect(false); // Stop background scanning loop to preserve SoftAP bandwidth
    }
  } else {
    Serial.println("\n[STA] No router SSID configured — connect to AP to configure network");
    WiFi.disconnect(false);
  }

  // Initialize mDNS responder (http://smarthive.local)
  if (MDNS.begin("smarthive")) {
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mDNS] Responder started: http://smarthive.local");
  }

  // Native ESP32 Web OTA Firmware Updater (accessible via /update)
  server.on("/update", HTTP_GET, []() {
    if (!requireAuth()) return;
    server.sendHeader("Connection", "close");
    server.send(200, "text/html",
      "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>ESP32 Firmware Update</title><style>"
      "body{font-family:sans-serif;background:#0f172a;color:#f8fafc;display:flex;align-items:center;justify-content:center;height:100vh;margin:0;}"
      ".card{background:#1e293b;padding:2rem;border-radius:1rem;border:1px solid #334155;text-align:center;max-width:400px;width:90%;}"
      "h2{color:#38bdf8;margin-top:0;}input[type=file]{margin:1.5rem 0;color:#94a3b8;width:100%;}"
      "input[type=submit]{background:#0284c7;color:#fff;border:none;padding:0.75rem 1.5rem;border-radius:0.5rem;font-weight:bold;cursor:pointer;width:100%;}"
      "</style></head><body><div class='card'><h2>ESP32 Web OTA</h2><p>Upload compiled firmware (.bin)</p>"
      "<form method='POST' action='/update' enctype='multipart/form-data'>"
      "<input type='file' name='update' accept='.bin'><br><input type='submit' value='Upload & Flash Firmware'>"
      "</form></div></body></html>");
  });
  server.on("/update", HTTP_POST, []() {
    if (!requireAuth()) return;
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK - Rebooting ESP32...");
    delay(500);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("[OTA] Update Start: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) {
        Serial.printf("[OTA] Update Success: %u bytes\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
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
  server.on("/api/wifi/saved", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleSavedWifiList();
  });
  server.on("/api/wifi/connect", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleWifiConnect();
  });
  server.on("/api/wifi/forget", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleWifiForget();
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
  server.on("/api/keepalive", HTTP_POST, []() {
    if (!requireAuth()) return;
    handleKeepAlive();
  });
  server.on("/api/keepalive", HTTP_GET, []() {
    if (!requireAuth()) return;
    handleKeepAlive();
  });

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

  server.begin();
  Serial.println("[HTTP] Web server started on port 80");
  Serial.println("========================================\n");
}

// ==================== LOOP ====================
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  updateLED();
  updateNFC();
  updateServo();
  updateTestButton();
  updateKeepAlive();

  static unsigned long lastPruneTime = 0;
  if (millis() - lastPruneTime > 60000) {
    lastPruneTime = millis();
    pruneOldReports();
  }

  // Periodic heartbeat sync with Cloud Dashboard (if configured) or batch flush if offline events were queued
  // Guarded by servoState == SERVO_IDLE_DETACHED to prevent simultaneous Servo PWM + Wi-Fi brownouts
  if (strlen(cloudApiUrl) >= 8 && WiFi.status() == WL_CONNECTED && servoState == SERVO_IDLE_DETACHED) {
    if (offlineQueueCount > 0) {
      if (millis() - lastCloudSyncAttempt > 5000) {
        flushOfflineQueue();
      }
    } else if (cloudSyncPending) {
      if (millis() - lastCloudSyncAttempt > 5000) {
        sendCloudEvent("button_press", lastScannedUid == "None" ? "" : lastScannedUid.c_str());
      }
    } else if (millis() - lastCloudSyncAttempt > CLOUD_HEARTBEAT_INTERVAL) {
      sendCloudEvent("heartbeat");
    }
  }

  // Multi-Network Automatic Reconnect & Background Beacon Detection (Zero Reboots)
  updateWiFiReconnect();
}