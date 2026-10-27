#pragma once

#include <HardwareSerial.h>
#include <BluetoothSerial.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>

// Bluetooth Serial object
BluetoothSerial SerialBT;

// Pin configuration for ESP32 SIM Card T-SIM7000G V1.0 with Quectel EG25-G module
// #define STATUS_PIN 5
#define STATUS_PIN 34     // GPIO34 for machine status monitoring
const int baud = 115200;  // Baud rate for EG25-G communication


const char *NODE_ID = "10b50893b65c4c2187194c4f937bd481";

const char *mqttUser = "";
const char *mqttPass = "";
const char *mqttHost = "";
const int mqttPort = 32100;

// MQTT Topics - These will be built dynamically using NODE_ID
String mqttTopic;
String mqttIntervalTopic;
String mqtt_pub_notif;
String mqttSensorsTopic;
const char *clientID = "clientESP32";
const char *node_id = NODE_ID;  // Keep for backward compatibility

// Function to build MQTT topics dynamically
void buildMqttTopics() {
  mqttTopic = "node/" + String(NODE_ID) + "/data";
  mqttIntervalTopic = "node/" + String(NODE_ID);
  mqtt_pub_notif = "notifications/node/" + String(NODE_ID);
  mqttSensorsTopic = "node/" + String(NODE_ID) + "/sensors";
  
  // Debug output to show built topics
  Serial.println("🔧 ======= DYNAMIC MQTT TOPICS BUILT =======");
  Serial.println("🆔 Node ID: " + String(NODE_ID));
  Serial.println("📡 Data Topic: " + mqttTopic);
  Serial.println("⏱️ Interval Topic: " + mqttIntervalTopic);
  Serial.println("🔔 Notification Topic: " + mqtt_pub_notif);
  Serial.println("📊 Sensors Topic: " + mqttSensorsTopic);
  Serial.println("==========================================");
}

HardwareSerial SerialAT(2);

unsigned long lastSend = 0;
// unsigned long sendInterval = 10000;
unsigned long sendInterval = 5000;  // 5 detik untuk GPS fix dan save ke offline_data.json
bool mqttConnected = false;
bool internetActive = false;
bool lastStatusMesin = true;

// MQTT monitoring variables
bool subscriptionActive = false;
unsigned long lastMqttCheck = 0;
const unsigned long MQTT_CHECK_INTERVAL = 30000;  // Cek MQTT setiap 30 detik

// Timer system untuk offline data (menggantikan counter)
unsigned long offlineTimer = 0;
// unsigned long offlineInterval = 60000; // Default 60 detik (akan diupdate dari MQTT)
unsigned long offlineInterval = 10000;  // Default 30 detik untuk offline timer (akan diupdate dari MQTT)
bool offlineTimerActive = false;
unsigned long lastOfflineSave = 0;
bool timerDebugEnabled = false;  // Flag untuk enable/disable debug timer
// Timer system untuk online data (status mesin = 1)
unsigned long onlineTimer = 0;
unsigned long onlineInterval = 10000;  // Default 30 detik untuk online timer (akan diupdate dari MQTT)
bool onlineTimerActive = false;
unsigned long lastOnlinePublish = 0;



// Debouncing untuk status mesin
unsigned long lastStatusChange = 0;
const unsigned long STATUS_DEBOUNCE_TIME = 2000; // 2 detik debounce
bool lastStableStatus = false; // Status terakhir yang stabil
int statusReadCount = 0; // Counter untuk validasi pembacaan
const int STATUS_VALIDATION_COUNT = 3; // Minimal 3x pembacaan sama untuk validasi

// Power supply monitoring
unsigned long lastPowerCheck = 0;
const unsigned long powerCheckInterval = 5000; // Check power every 5 seconds
bool powerStable = true;
size_t lastFreeHeap = 0;

// GPS related variables
bool gpsEnabled = false;
bool gpsConnected = false;
unsigned long lastGPSCheck = 0;
const unsigned long gpsCheckInterval = 10000;  // Check GPS status every 10 seconds

// GPS Boot Time Tracking
unsigned long bootTime = 0;          // Waktu saat boot (millis)
unsigned long firstGPSFixTime = 0;   // Waktu pertama kali GPS fix (millis)
bool gpsFixAchieved = false;         // Flag apakah GPS sudah pernah fix
unsigned long bootToGPSFixTime = 0;  // Waktu dari boot sampai GPS fix (dalam detik)

const char *lastGPSFile = "/last_data.json";
const char *offlineDataFile = "/offline_data.json";

// GSM Functions
bool activateInternet();
bool deactivateInternet();
void sendAT(String cmd, unsigned long delayTime = 1000);
bool sendAndWait(String cmd, String expected, unsigned long timeout = 5000);

// MQTT Functions
bool mqttConnect();
bool mqttConnectForPublish();  // New: connect MQTT untuk publish only (tanpa subscribe)
bool checkMqttConnection();    // New: check MQTT connection status
bool mqttPublish(String payload);
bool mqttPublishStatusMesin(String timestamp, String statusMesinStr);
bool mqttPublishNotification(String payload);  // New: publish notification with specific topic
String buildStatusGPSPayload(String timestamp, String statusGPS);  // New: build statusGPS payload
bool publishStatusGPS(String timestamp, String statusGPS);         // New: publish statusGPS
bool publishOfflineData();
bool publishSingleOfflineData();
bool publishLastGPSData();  // New: publish last GPS data
bool mqttSubscribe(String topic);
bool checkSubscriptionStatus();  // Updated: return boolean
bool ensureMqttSubscription();   // New: ensure subscription is active
void monitorMqttConnection();    // New: periodic MQTT monitoring
void testTopicMessage();         // New: test topic message reception
void checkRetainMessage();       // New: check retain message
void handleMqttMessage(String topic, String message);
void processIntervalMessage(String message);
void checkMqttMessages();
void processMqttReceive(String line);

// Message Handler Functions
void handleSerialATMQTT();
void checkIncomingMQTT();
void handleMQTTMessage(String topic, String payload);
bool isValidJson(const String& str);
bool processMqttReceiveWithResult(String line);

// Interval Manager Functions
void saveIntervalToSD(int interval);
void loadIntervalFromSD();
void parseIntervalFromSerialAdvanced();

// GPS Functions
String parseGPSTimestamp(String timeUTC, String date);
bool isValidGPSTimestamp(String timestamp);
String getGPSPayload(bool &gpsSuccess, String &timestampRef);
String buildGPSPayload(String timestamp, String lat, String lon);
String buildGPSPayloadForStorage(String timestamp, String lat, String lon);  // New: untuk GPS yang disimpan di SPIFFS
String buildStatusPayload(String timestamp, String statusMesinStr);
void initGPS();
bool checkGPSStatus();
void monitorGPSStatus();
void displayGPSStatusSummary();
bool getGPSDataImmediate(String &timestamp, String &lat, String &lon, String &alt);  // New: untuk data GPS segera
String getFallbackTimestamp();  // New: fallback timestamp jika GSM tidak valid
String getRTCTimeFromGSM();     // New: get RTC time from GSM module
String getFallbackPayload(String &timestampRef);  // New: get fallback GPS payload
bool getGPSData(String &timestamp, String &lat, String &lon, String &alt);  // New: get GPS data
String determineAlarmStatus(int status, int alarmType, int alarmHigh, int alarmLow);  // New: determine alarm status

// Power Supply Functions
void monitorPowerSupply();
bool isPowerSupplyStable();
void logPowerStatus();

// File Manager Functions
void initSDCard();
void displaySDCardInfo();  // New: tampilkan info SD Card
void saveLastGPS(String fullPayload);
bool loadLastGPS(String &payload);
String getLastGPSWithRealtimeTimestamp();  // New: helper function untuk data GPS terakhir dengan timestamp GSM realtime
bool isNewGPS(String newLat, String newLon, String newTimestamp);
void saveOfflineGPSData(String gpsPayload);  // Updated: hanya menyimpan GPS data
int getOfflineGPSDataCount();                // New: untuk mendapatkan jumlah data GPS offline
bool isNewGPSData(String gpsPayload);        // New: untuk validasi data GPS baru
bool publishOfflineDataChunked(int chunkSize);  // New: chunked loading untuk file besar
bool processOfflineDataInChunks(int chunkSize);      // New: process data dalam chunks
int loadAndPublishChunk(int chunkSize);              // New: load dan publish satu chunk
bool publishOfflineDataNormal();                     // New: normal loading untuk file kecil

// Status Machine Functions
void handleMachineStatusChange(bool currentStatus, bool previousStatus);
bool publishStatusMesin(String timestamp, String statusMesinStr);  // New: publish status mesin
void ensureStatusMesinPublished();                                 // New: ensure status mesin published after reconnect
void handleOfflineDataPublish();
void startOfflineTimer();
void stopOfflineTimer();
bool isOfflineTimerExpired();
void resetOfflineTimer();
bool publishAllStoredDataWhenMachineOn();  // New: publish all stored data when machine on

// Online timer functions
void startOnlineTimer();
void stopOnlineTimer();
void resetOnlineTimer();
bool isOnlineTimerExpired();
// Notification Functions
void sendStatusNotifications(int statusMachine);  // Send status machine notifications
bool sendStatusNotificationToChannel(String channel, const char* statusText, const char* subjectText);  // Send status notification to specific channel

// Status validation functions
bool isStatusMachineStable();  // Validasi status mesin yang stabil
bool getStableMachineStatus(); // Dapatkan status mesin yang sudah divalidasi


// Debug Functions
void displaySystemStatus();
void displayOfflineDataInfo();
void testMQTTConnection();
void manualPublishOfflineData();
void testTopicMessage();
void displayGPSStatusSummary();

// Bluetooth Functions
void initBluetooth();
void sendToBluetooth(String message);
void sendSystemStatusToBT();
void sendGPSStatusToBT();
void sendGPSDataToBT();  // New: tampilkan data GPS terbaru
void sendMQTTStatusToBT();
void sendChargingStatusToBT();  // New: tampilkan status charging lengkap
void testSerialConnection();
void sendSerialTestToBT();

// Charger Functions
void setupChargerModule();
void setupChargerModuleMinimal();  // Minimal setup untuk boot cepat
void readChargerStatus();
void publishChargerStatus();
void updateChargerLED();     // Update LED status secara real-time
void updateAdapterStatus();  // Update adapter status secara real-time

// Helper functions untuk charging status
bool isChargingActive();     // Cek apakah charging aktif (mesin hidup)
bool isAdapterConnected();   // Cek apakah adapter terhubung

// Notification control functions
void resetNotificationFlag();  // Reset notification flag

// Global charger status variables
extern int isAdapterOk;
extern int led;
int led = 0;

extern int statusMesinAlarmType;
extern int statusMesinAlarmHigh;
extern int statusMesinAlarmLow;

// Global variables untuk charging status
// CATATAN: Sistem ini menggunakan STATUS_PIN yang sama untuk status mesin dan adapter
// Jika perlu membedakan, tambahkan pin terpisah untuk adapter status
int isAdapterOk = 0;  // Status adapter (0 = tidak terpasang, 1 = terpasang)

// Flag untuk mencegah duplikasi notification
bool notificationSentForCurrentStatus = false;
int lastNotificationStatus = -1; // -1 = belum ada notification


// Variabel global untuk alarm statusMesin
int statusMesinAlarmType = 0;
int statusMesinAlarmHigh = 0;
int statusMesinAlarmLow = 0;

#include <Wire.h>

// --- Konstanta dan Pin Charger ---
#define BQ24725A_ADDR 0x12
#define MANUFACTURER_ID_REG 0xFE
#define INPUT_CURRENT_REG 0x3F
#define CHARGE_VOLTAGE_REG 0x15
#define CHARGE_CURRENT_REG 0x14
#define CHARGE_OPTION_REG 0x12
#define IOUT_PIN 36           // Pin ADC untuk membaca arus
#define CHGEN_PIN 33          // Pin untuk mengaktifkan/menonaktifkan IC charger dari baterai
#define ADAPTER_OK_LED_PIN 5   // Pin Output untuk indikator status adapter
#define INPUT_CURRENT_LIMIT_MA 2048
#define TARGET_CHARGE_VOLTAGE_MV 12600
#define TARGET_CHARGE_CURRENT_MA 768
#define SENSE_RESISTOR_OHMS 0.010
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCLK 18
#define SD_CS 25

