// mqtt_function.ino
#include "configure.h"
#include <ArduinoJson.h>

// Konfigurasi interval publish
const unsigned long PUBLISH_INTERVAL = 3000;  // 3 detik antara setiap publish
const unsigned long BATCH_DELAY = 5000;       // 5 detik setiap batch (setiap 5 data)
const int BATCH_SIZE = 5;                     // Jumlah data per batch



// Fungsi untuk mengecek status koneksi MQTT
bool checkMqttConnection() {
  if (!internetActive) {
    Serial.println("❌ Internet tidak aktif untuk cek MQTT");
    sendToBluetooth("❌ Internet tidak aktif untuk cek MQTT");
    return false;
  }

  // Hanya cek jika tidak sedang dalam proses koneksi
  static bool isChecking = false;
  if (isChecking) {
    Serial.println("ℹ️ MQTT status check sedang berlangsung, skip...");
    sendToBluetooth("ℹ️ MQTT status check sedang berlangsung, skip...");
    return mqttConnected;  // Return status terakhir
  }

  isChecking = true;
  Serial.println("🔍 Checking MQTT connection status...");
  sendToBluetooth("🔍 Checking MQTT connection status...");

  // Clear buffer
  while (SerialAT.available()) {
    SerialAT.read();
  }

  SerialAT.println("AT+QMTCONN?");
  delay(500);  // Reduced delay

  String response = "";
  unsigned long start = millis();

  while (millis() - start < 2000) {  // Reduced timeout
    if (SerialAT.available()) {
      response += SerialAT.readString();
      if (response.indexOf("OK") >= 0) {
        break;
      }
    }
    delay(10);
  }

  // Log response ke Bluetooth untuk debugging
  Serial.println("📡 MQTT Status Response: " + response);
  sendToBluetooth("📡 MQTT Status Response: " + response);

  bool isConnected = false;
  if (response.indexOf("+QMTCONN: 0,1") >= 0) {
    Serial.println("✅ MQTT Connected");
    sendToBluetooth("✅ MQTT Connected");
    mqttConnected = true;
    isConnected = true;
  } else if (response.indexOf("+QMTCONN: 0,0") >= 0) {
    Serial.println("🔴 MQTT Disconnected");
    sendToBluetooth("🔴 MQTT Disconnected");
    mqttConnected = false;
    isConnected = false;
  } else {
    Serial.println("⚠️ MQTT Status Unknown, assuming connected");
    sendToBluetooth("⚠️ MQTT Status Unknown, assuming connected");
    // Jika tidak bisa cek status, asumsikan masih terkoneksi
    isConnected = mqttConnected;
  }

  isChecking = false;
  return isConnected;
}

// Variabel global untuk alarm statusMesin

// Tambahkan subscribe ke topic sensors setelah connect MQTT
bool mqttConnect() {
  if (!internetActive) {
    Serial.println("❌ Internet tidak aktif, tidak bisa connect MQTT");
    sendToBluetooth("❌ Internet tidak aktif, tidak bisa connect MQTT");
    return false;
  }

  // Check if modem is responding
  Serial.println("🔍 Checking modem response...");
  sendToBluetooth("🔍 Checking modem response...");
  
  // Send AT command to check if modem is responding
  SerialAT.println("AT");
  delay(1000);
  
  bool modemResponding = false;
  unsigned long startCheck = millis();
  while (millis() - startCheck < 3000) {
    if (SerialAT.available()) {
      String resp = SerialAT.readString();
      if (resp.indexOf("OK") >= 0) {
        modemResponding = true;
        break;
      }
    }
    delay(100);
  }
  
  if (!modemResponding) {
    Serial.println("❌ Modem tidak merespons, mungkin sedang restart");
    sendToBluetooth("❌ Modem tidak merespons, mungkin sedang restart");
    return false;
  }
  
  Serial.println("✅ Modem merespons, lanjut ke MQTT connect");
  sendToBluetooth("✅ Modem merespons, lanjut ke MQTT connect");

  // Monitor RAM sebelum MQTT connect
  Serial.println("📊 ======= RAM MONITORING MQTT CONNECT =======");
  sendToBluetooth("📊 ======= RAM MONITORING MQTT CONNECT =======");
  Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
  sendToBluetooth("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
  Serial.println("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
  Serial.println("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
  sendToBluetooth("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");

  // Validasi RAM sebelum connect
  if (ESP.getFreeHeap() < 30000) {  // Jika RAM < 30KB
    Serial.println("⚠️ RAM rendah sebelum MQTT connect: " + String(ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("⚠️ RAM rendah sebelum MQTT connect: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("⚠️ MQTT connection mungkin gagal karena kekurangan RAM");
    sendToBluetooth("⚠️ MQTT connection mungkin gagal karena kekurangan RAM");
  }
  Serial.println("=============================================");
  sendToBluetooth("=============================================");

  // Cek apakah sudah terkoneksi
  if (mqttConnected) {
    Serial.println("ℹ️ MQTT sudah terkoneksi, skip connect");
    return true;
  }

  Serial.println("🔄 Memulai koneksi MQTT...");
  sendToBluetooth("🔄 Memulai koneksi MQTT...");

  // Konfigurasi send mode (sesuai Python)
  Serial.println("⚙️ Konfigurasi MQTT send mode...");
  sendAT("AT+QMTCFG=\"send/mode\",0,1");
  delay(500);

  // Buka koneksi MQTT (sesuai Python)
  Serial.println("🔄 Membuka koneksi MQTT...");
  if (!sendAndWait("AT+QMTOPEN=0,\"" + String(mqttHost) + "\"," + String(mqttPort), "+QMTOPEN: 0,0", 15000)) {
    Serial.println("❌ Gagal membuka koneksi MQTT");
    return false;
  }

  Serial.println("🔄 Melakukan authentikasi MQTT...");
  String connCmd = String("AT+QMTCONN=0,\"") + String(mqttHost) + "\",\"" + String(mqttUser) + "\",\"" + String(mqttPass) + "\"";
  if (!sendAndWait(connCmd, "+QMTCONN: 0,0,0", 10000)) {
    Serial.println("❌ Gagal authentikasi MQTT");
    return false;
  }

  Serial.println("✅ MQTT CONNECT sukses");
  sendToBluetooth("✅ MQTT CONNECT sukses");

  // Set flag mqttConnected
  mqttConnected = true;

  // Subscribe ke topic interval setelah connect
  if (mqttSubscribe(mqttIntervalTopic)) {
    Serial.println("✅ Berhasil subscribe ke topic interval");
    // Subscribe ke topic sensors
    mqttSubscribe(mqttSensorsTopic);
    // Tunggu sebentar untuk stabilisasi
    delay(1000);
    // Cek message yang masuk
    checkMqttMessages();
    // Cek apakah ada retain message yang masuk
    Serial.println("🔍 Checking for retain message after subscribe...");
    checkRetainMessage();
  } else {
    Serial.println("❌ Gagal subscribe ke topic interval");
  }
  return true;
}

// Fungsi untuk connect MQTT khusus untuk publish (tanpa subscribe)
bool mqttConnectForPublish() {
  if (!internetActive) {
    Serial.println("❌ Internet tidak aktif, tidak bisa connect MQTT");
    return false;
  }

  Serial.println("🔄 Memulai koneksi MQTT untuk publish...");

  // Konfigurasi send mode (sesuai Python)
  Serial.println("⚙️ Konfigurasi MQTT send mode...");
  sendAT("AT+QMTCFG=\"send/mode\",0,1");
  delay(500);

  // Buka koneksi MQTT (sesuai Python)
  Serial.println("🔄 Membuka koneksi MQTT...");
  if (!sendAndWait("AT+QMTOPEN=0,\"" + String(mqttHost) + "\"," + String(mqttPort), "+QMTOPEN: 0,0", 15000)) {
    Serial.println("❌ Gagal membuka koneksi MQTT");
    return false;
  }

  Serial.println("🔄 Melakukan authentikasi MQTT...");
  String connCmd = String("AT+QMTCONN=0,\"") + String(mqttHost) + "\",\"" + String(mqttUser) + "\",\"" + String(mqttPass) + "\"";
  if (!sendAndWait(connCmd, "+QMTCONN: 0,0,0", 10000)) {
    Serial.println("❌ Gagal authentikasi MQTT");
    return false;
  }

  Serial.println("✅ MQTT CONNECT sukses (untuk publish)");

  // Set flag mqttConnected
  mqttConnected = true;

  // TIDAK subscribe, hanya untuk publish
  Serial.println("📡 Mode publish only - tidak subscribe");

  return true;
}



// Handler pesan masuk untuk topic sensors
void handleMqttMessage(String topic, String message) {
  Serial.println("📨 ======= MQTT MESSAGE HANDLER =======");
  Serial.println("📋 Processing Topic: " + topic);
  Serial.println("📄 Processing Message: " + message);
  Serial.println("🔍 Expected Topic: " + String(mqttIntervalTopic));
  Serial.println("🔍 Topic Match: " + String(topic == mqttIntervalTopic ? "YES" : "NO"));

  if (topic == mqttIntervalTopic) {
    Serial.println("⏱️ ======= INTERVAL MESSAGE DETECTED =======");
    Serial.println("🔄 Processing interval message...");
    processIntervalMessage(message);
    Serial.println("⏱️ ========================================");
  } else if (topic == mqttSensorsTopic) {
    Serial.println("🔔 Processing sensors topic...");
    StaticJsonDocument<2048> doc;
    DeserializationError err = deserializeJson(doc, message);
    if (!err && doc.is<JsonArray>()) {
      for (JsonObject sensor : doc.as<JsonArray>()) {
        if (sensor["costume_id"] == "statusMesin") {
          statusMesinAlarmType = String(sensor["alarm_type"] | "0").toInt();
          statusMesinAlarmHigh = sensor["highest_value"] | 0;
          statusMesinAlarmLow = sensor["lowest_value"] | 0;
          Serial.println("[ALARM] statusMesinAlarmType: " + String(statusMesinAlarmType) + ", High: " + String(statusMesinAlarmHigh) + ", Low: " + String(statusMesinAlarmLow));
        }
      }
    } else {
      Serial.println("❌ Gagal parsing JSON sensors");
    }
  } else {
    Serial.println("⚠️ Unknown topic: " + topic);
    Serial.println("📄 Message content: " + message);
  }
  Serial.println("📨 ==================================");
}

// Fungsi rekursif untuk mencari interval di seluruh JSON
int findInterval(JsonVariant v) {
  if (v.is<int>() && v.as<int>() > 0) return v.as<int>();
  if (v.is<JsonObject>()) {
    for (JsonPair kv : v.as<JsonObject>()) {
      if (String(kv.key().c_str()) == "interval") return kv.value().as<int>();
      int found = findInterval(kv.value());
      if (found > 0) return found;
    }
  }
  if (v.is<JsonArray>()) {
    for (JsonVariant val : v.as<JsonArray>()) {
      int found = findInterval(val);
      if (found > 0) return found;
    }
  }
  return -1;
}

void processIntervalMessage(String message) {
  Serial.println("🔄 ======= PROCESSING INTERVAL MESSAGE =======");
  Serial.println("📄 Raw Message: " + message);
  sendToBluetooth("🔄 ======= PROCESSING INTERVAL MESSAGE =======");
  sendToBluetooth("📄 Raw Message: " + message);

  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.println("❌ JSON PARSE ERROR: " + String(error.c_str()));
    sendToBluetooth("❌ JSON PARSE ERROR: " + String(error.c_str()));
    sendToBluetooth("❌ Gagal parsing data interval dari MQTT!");
    return;
  }

  if (doc.containsKey("interval")) {
    int newInterval = doc["interval"];
    int originalInterval = newInterval;
    // Kurangi 40 detik agar publish lebih awal
    if (newInterval > 50) {
      newInterval -= 50;
      Serial.println("🔄 Interval dikurangi 50 detik: " + String(originalInterval) + "s -> " + String(newInterval) + "s");
      sendToBluetooth("🔄 Interval dikurangi 50 detik: " + String(originalInterval) + "s -> " + String(newInterval) + "s");
    } else {
      newInterval = 1;
      Serial.println("⚠️ Interval terlalu kecil, diset ke minimum 1 detik");
      sendToBluetooth("⚠️ Interval terlalu kecil, diset ke minimum 1 detik");
    }
    Serial.println("✅ Key 'interval' ditemukan: " + String(newInterval));
    sendToBluetooth("✅ Key 'interval' ditemukan: " + String(newInterval));

    // Update offlineInterval dan onlineInterval dengan interval yang sudah dikurangi
    if (newInterval > 0 && newInterval <= 86400) {
      unsigned long newIntervalMs = newInterval * 1000UL;
      if (newIntervalMs != offlineInterval) {
        unsigned long oldInterval = offlineInterval / 1000;
        offlineInterval = newIntervalMs;
        onlineInterval = newIntervalMs; // Update online interval juga
        Serial.println("🔄 Interval updated: " + String(oldInterval) + "s -> " + String(newInterval) + "s");
        sendToBluetooth("🔄 Interval updated: " + String(oldInterval) + "s -> " + String(newInterval) + "s");
        if (offlineTimerActive) {
          resetOfflineTimer();
          Serial.println("🔄 Offline timer reset with new interval");
          sendToBluetooth("🔄 Offline timer reset with new interval");
        }
        if (onlineTimerActive) {
          resetOnlineTimer();
          Serial.println("🔄 Online timer reset with new interval");
          sendToBluetooth("🔄 Online timer reset with new interval");
        }
      }
    }
    saveIntervalToSD(newInterval);  // Simpan interval yang sudah dikurangi ke SD Card
    Serial.println("💾 Interval berhasil disimpan ke SD Card");
    sendToBluetooth("💾 Interval berhasil disimpan ke SD Card");
    sendToBluetooth("✅ BERHASIL update interval dari MQTT!");
  } else {
    Serial.println("❌ Key 'interval' TIDAK ditemukan di root JSON");
    sendToBluetooth("❌ Key 'interval' TIDAK ditemukan di root JSON");
    // Coba pakai findInterval jika nested
    int newInterval = findInterval(doc);
    int originalInterval = newInterval;
    if (newInterval > 50) {
      newInterval -= 50;
      Serial.println("🔄 Interval dikurangi 50 detik: " + String(originalInterval) + "s -> " + String(newInterval) + "s");
      sendToBluetooth("🔄 Interval dikurangi 50 detik: " + String(originalInterval) + "s -> " + String(newInterval) + "s");
    } else if (newInterval > 0) {
      newInterval = 1;
      Serial.println("⚠️ Interval terlalu kecil, diset ke minimum 1 detik");
      sendToBluetooth("⚠️ Interval terlalu kecil, diset ke minimum 1 detik");
    }
    Serial.println("🔍 Hasil findInterval: " + String(newInterval));
    sendToBluetooth("🔍 Hasil findInterval: " + String(newInterval));
    if (newInterval > 0) {
      // Update offlineInterval dengan interval yang sudah dikurangi
      if (newInterval <= 86400) {
        unsigned long newIntervalMs = newInterval * 1000UL;
        if (newIntervalMs != offlineInterval) {
          unsigned long oldInterval = offlineInterval / 1000;
          offlineInterval = newIntervalMs;
          Serial.println("🔄 Interval updated: " + String(oldInterval) + "s -> " + String(newInterval) + "s");
          sendToBluetooth("🔄 Interval updated: " + String(oldInterval) + "s -> " + String(newInterval) + "s");
          if (offlineTimerActive) {
            resetOfflineTimer();
            Serial.println("🔄 Offline timer reset with new interval");
            sendToBluetooth("🔄 Offline timer reset with new interval");
          }
        }
      }
      saveIntervalToSD(newInterval);  // Simpan interval yang sudah dikurangi ke SD Card
      Serial.println("💾 Interval berhasil disimpan ke SD Card");
      sendToBluetooth("💾 Interval berhasil disimpan ke SD Card");
      sendToBluetooth("✅ BERHASIL update interval dari MQTT!");
    } else {
      Serial.println("❌ Tetap tidak ditemukan key 'interval'");
      sendToBluetooth("❌ Tetap tidak ditemukan key 'interval'");
      sendToBluetooth("❌ Gagal update interval dari MQTT!");
    }
  }
}

bool mqttPublish(String payload) {
  if (!mqttConnected) {
    Serial.println("❌ MQTT tidak terkoneksi");
    sendToBluetooth("❌ MQTT tidak terkoneksi");
    return false;
  }

  // Check power supply stability before publish
  if (!isPowerSupplyStable()) {
    Serial.println("⚠️ Power supply tidak stabil, delaying publish...");
    sendToBluetooth("⚠️ Power supply tidak stabil, delaying publish...");
    delay(1000); // Wait for power to stabilize
  }

  if (payload.length() == 0) {
    Serial.println("❌ Payload kosong, tidak bisa publish");
    sendToBluetooth("❌ Payload kosong, tidak bisa publish");
    return false;
  }

  // Validasi payload tidak null atau kosong
  if (payload == "null" || payload == "" || payload.length() < 10) {
    Serial.println("❌ Payload tidak valid: " + payload);
    Serial.println("📏 Payload length: " + String(payload.length()));
    sendToBluetooth("❌ Payload tidak valid, length: " + String(payload.length()));
    return false;
  }

  // Debug: tampilkan payload yang akan dikirim (hanya jika debug enabled)
  if (timerDebugEnabled) {
    Serial.println("🔍 Debug - Payload yang akan dikirim:");
    Serial.println("📄 Content: " + payload);
    Serial.println("📏 Length: " + String(payload.length()) + " bytes");
    sendToBluetooth("🔍 Debug - Payload length: " + String(payload.length()) + " bytes");
  }

  int len = payload.length();
  Serial.println("🔄 Memulai publish payload (Length: " + String(len) + ")...");
  sendToBluetooth("🔄 Memulai publish payload (Length: " + String(len) + ")...");

  String publishCmd = "AT+QMTPUBEX=0,0,0,0,\"" + String(mqttTopic) + "\"," + String(len);
  Serial.println("📡 Command: " + publishCmd);
  sendToBluetooth("📡 Command: " + publishCmd);
  SerialAT.println(publishCmd);
  delay(100);

  unsigned long start = millis();
  bool promptReceived = false;
  String response = "";

  while (millis() - start < 5000) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      response += line;
      if (line.indexOf(">") >= 0) {
        promptReceived = true;
        break;
      }
    }
  }

  if (!promptReceived) {
    Serial.println("❌ Tidak menerima prompt '>' dari modem");
    Serial.println("Response: " + response);
    sendToBluetooth("❌ Tidak menerima prompt '>' dari modem");
    sendToBluetooth("Response: " + response);

    // Check if modem is still responding
    Serial.println("🔍 Checking if modem is still responding...");
    sendToBluetooth("🔍 Checking if modem is still responding...");
    
    SerialAT.println("AT");
    delay(1000);
    bool modemOk = false;
    unsigned long startCheck = millis();
    while (millis() - startCheck < 3000) {
      if (SerialAT.available()) {
        String resp = SerialAT.readString();
        if (resp.indexOf("OK") >= 0) {
          modemOk = true;
          break;
        }
      }
      delay(100);
    }
    
    if (!modemOk) {
      Serial.println("❌ Modem tidak merespons, marking MQTT as disconnected");
      sendToBluetooth("❌ Modem tidak merespons, marking MQTT as disconnected");
      mqttConnected = false;
      internetActive = false;
      return false;
    } else {
      Serial.println("⚠️ Modem masih merespons tapi MQTT publish gagal");
      sendToBluetooth("⚠️ Modem masih merespons tapi MQTT publish gagal");
      mqttConnected = false;
      return false;
    }
  }

  delay(20);
  Serial.println("✅ Kirim payload...");
  sendToBluetooth("✅ Kirim payload...");
  Serial.println(payload);
  SerialAT.print(payload);
  SerialAT.write(0x1A);

  // Wait for publish confirmation
  start = millis();
  response = "";
  while (millis() - start < 10000) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      response += line;
      Serial.println("<< " + line);
      sendToBluetooth("<< " + line);
      if (line.indexOf("+QMTPUBEX: 0,0,0") >= 0) {
        Serial.println("✅ Publish berhasil dikonfirmasi");
        sendToBluetooth("✅ Publish berhasil dikonfirmasi");
        return true;
      }
      if (line.indexOf("ERROR") >= 0) {
        Serial.println("❌ Publish gagal: " + line);
        sendToBluetooth("❌ Publish gagal: " + line);
        return false;
      }
    }
  }

  Serial.println("⚠️ Timeout menunggu konfirmasi publish");
  Serial.println("Response: " + response);
  sendToBluetooth("⚠️ Timeout menunggu konfirmasi publish");
  sendToBluetooth("Response: " + response);
  return false;
}

bool mqttPublishStatusMesin(String timestamp, String statusMesinStr) {
  StaticJsonDocument<256> doc;
  doc["node_id"] = node_id;
  doc["timestamp"] = timestamp;
  doc["status"] = "9";
  JsonObject data = doc.createNestedObject("data");
  data["statusMesin"] = statusMesinStr;
  String json;
  serializeJson(doc, json);
  return mqttPublish(json);
}



bool publishOfflineData() {
  if (!SD.exists(offlineDataFile) || !mqttConnected) {
    Serial.println("❌ File offline tidak ada di SD Card atau MQTT tidak terkoneksi");
    return false;
  }

  // Monitor RAM sebelum memulai
  Serial.println("📊 ======= RAM MONITORING =======");
  sendToBluetooth("📊 ======= RAM MONITORING =======");
  Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
  sendToBluetooth("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
  Serial.println("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
  Serial.println("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
  sendToBluetooth("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
  Serial.println("=================================");
  sendToBluetooth("=================================");

  // Cek ukuran file sebelum loading
  File sizeFile = SD.open(offlineDataFile, FILE_READ);
  if (!sizeFile) {
    Serial.println("❌ Gagal membuka file offline untuk cek ukuran");
    return false;
  }

  size_t fileSize = sizeFile.size();
  sizeFile.close();

  Serial.println("📁 File size: " + String(fileSize) + " bytes");
  sendToBluetooth("📁 File size: " + String(fileSize) + " bytes");

  // Validasi ukuran file dan RAM
  if (fileSize > 30000) {  // Jika file > 30KB
    Serial.println("⚠️ File terlalu besar (" + String(fileSize) + " bytes), mungkin menyebabkan masalah RAM");
    sendToBluetooth("⚠️ File terlalu besar (" + String(fileSize) + " bytes), mungkin menyebabkan masalah RAM");
    Serial.println("💾 Free RAM: " + String(ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("💾 Free RAM: " + String(ESP.getFreeHeap()) + " bytes");

    if (ESP.getFreeHeap() < 50000) {  // Jika RAM < 50KB
      Serial.println("❌ RAM tidak cukup untuk memuat file besar");
      sendToBluetooth("❌ RAM tidak cukup untuk memuat file besar");
      Serial.println("❌ MQTT connection mungkin gagal karena kekurangan RAM");
      sendToBluetooth("❌ MQTT connection mungkin gagal karena kekurangan RAM");
      return false;
    }
  }

  // Simpan data GPS terbaru SEBELUM publish batch untuk memastikan tidak ada data yang hilang
  Serial.println("💾 Menyimpan data GPS terbaru sebelum publish batch offline data...");
  bool gpsFix = false;
  String timestamp;
  String gpsPayload = getGPSPayload(gpsFix, timestamp);

  // Hanya simpan jika ada data GPS valid DAN mesin mati
  if (gpsPayload != "" && !digitalRead(STATUS_PIN)) {
    saveOfflineGPSData(gpsPayload);
    Serial.println("✅ Data GPS terbaru tersimpan sebelum publish batch offline data (mesin mati)");
    Serial.println("⏰ Sumber: Timer offlineInterval (" + String(offlineInterval/1000) + " detik)");
    sendToBluetooth("⏰ Sumber: Timer offlineInterval (" + String(offlineInterval/1000) + " detik)");
  } else if (gpsPayload != "" && digitalRead(STATUS_PIN)) {
    Serial.println("⚠️ Data GPS valid tersedia tapi mesin hidup, tidak disimpan");
  } else {
    Serial.println("⚠️ Tidak ada data GPS valid untuk disimpan sebelum publish batch");
  }

  // Load file dengan validasi RAM
  File file = SD.open(offlineDataFile, FILE_READ);
  if (!file) {
    Serial.println("❌ Gagal membuka file offline di SD Card");
    return false;
  }

  // Cek RAM sebelum readString
  size_t freeHeapBefore = ESP.getFreeHeap();
  Serial.println("💾 Free RAM sebelum load file: " + String(freeHeapBefore) + " bytes");
  sendToBluetooth("💾 Free RAM sebelum load file: " + String(freeHeapBefore) + " bytes");

  String content = file.readString();
  file.close();

  size_t freeHeapAfter = ESP.getFreeHeap();
  Serial.println("💾 Free RAM setelah load file: " + String(freeHeapAfter) + " bytes");
  sendToBluetooth("💾 Free RAM setelah load file: " + String(freeHeapAfter) + " bytes");
  Serial.println("💾 RAM yang digunakan: " + String(freeHeapBefore - freeHeapAfter) + " bytes");
  sendToBluetooth("💾 RAM yang digunakan: " + String(freeHeapBefore - freeHeapAfter) + " bytes");

  if (freeHeapAfter < 20000) {  // Jika RAM tersisa < 20KB
    Serial.println("❌ RAM tidak cukup setelah load file, MQTT mungkin gagal");
    sendToBluetooth("❌ RAM tidak cukup setelah load file, MQTT mungkin gagal");
    Serial.println("❌ Free RAM: " + String(freeHeapAfter) + " bytes (minimum: 20000 bytes)");
    sendToBluetooth("❌ Free RAM: " + String(freeHeapAfter) + " bytes (minimum: 20000 bytes)");
    return false;
  }

  // Cek RAM sebelum JSON parsing
  size_t freeHeapBeforeJson = ESP.getFreeHeap();
  Serial.println("💾 Free RAM sebelum JSON parsing: " + String(freeHeapBeforeJson) + " bytes");
  sendToBluetooth("💾 Free RAM sebelum JSON parsing: " + String(freeHeapBeforeJson) + " bytes");

  // Gunakan DynamicJsonDocument untuk file besar
  DynamicJsonDocument doc(fileSize * 2);  // 2x ukuran file untuk JSON overhead
  DeserializationError err = deserializeJson(doc, content);

  size_t freeHeapAfterJson = ESP.getFreeHeap();
  Serial.println("💾 Free RAM setelah JSON parsing: " + String(freeHeapAfterJson) + " bytes");
  sendToBluetooth("💾 Free RAM setelah JSON parsing: " + String(freeHeapAfterJson) + " bytes");
  Serial.println("💾 RAM yang digunakan JSON: " + String(freeHeapBeforeJson - freeHeapAfterJson) + " bytes");
  sendToBluetooth("💾 RAM yang digunakan JSON: " + String(freeHeapBeforeJson - freeHeapAfterJson) + " bytes");

  if (err) {
    Serial.println("❌ Gagal parsing JSON offline data: " + String(err.c_str()));
    sendToBluetooth("❌ Gagal parsing JSON offline data: " + String(err.c_str()));
    Serial.println("❌ Kemungkinan file corrupt atau format tidak valid");
    sendToBluetooth("❌ Kemungkinan file corrupt atau format tidak valid");
    return false;
  }

  if (freeHeapAfterJson < 15000) {  // Jika RAM tersisa < 15KB
    Serial.println("❌ RAM tidak cukup setelah JSON parsing, MQTT mungkin gagal");
    sendToBluetooth("❌ RAM tidak cukup setelah JSON parsing, MQTT mungkin gagal");
    Serial.println("❌ Free RAM: " + String(freeHeapAfterJson) + " bytes (minimum: 15000 bytes)");
    sendToBluetooth("❌ Free RAM: " + String(freeHeapAfterJson) + " bytes (minimum: 15000 bytes)");
    return false;
  }

  if (!doc.containsKey("offline_gps_data")) {
    Serial.println("❌ Tidak ada offline_gps_data dalam file");
    return false;
  }

  JsonArray dataArray = doc["offline_gps_data"];
  int totalData = dataArray.size();
  int published = 0;
  int failed = 0;

  Serial.println("📤 ======= OFFLINE GPS DATA PUBLISH =======");
  Serial.println("📊 Total GPS data offline: " + String(totalData));
  Serial.println("⏱️ Interval per publish: " + String(PUBLISH_INTERVAL) + "ms");
  Serial.println("📦 Batch size: " + String(BATCH_SIZE));
  Serial.println("⏱️ Batch delay: " + String(BATCH_DELAY) + "ms");
  Serial.println("🗑️ Auto delete: Data yang berhasil dipublish langsung dihapus dari SD Card");
  Serial.println("==========================================");

  unsigned long startTime = millis();

  for (int i = 0; i < totalData; i++) {
    JsonVariant item = dataArray[i];
    String payload;
    serializeJson(item, payload);

    Serial.println("📤 Publishing GPS data " + String(i + 1) + "/" + String(totalData) + "...");

    if (mqttPublish(payload)) {
      published++;
      Serial.println("✅ Success [" + String(published) + " published] - Data dihapus dari SD Card");

      // Langsung hapus data yang berhasil dipublish dari array
      dataArray.remove(0);  // Hapus data pertama (yang baru saja dipublish)

      // Update file SD Card secara real-time
      File writeFile = SD.open(offlineDataFile, FILE_WRITE);
      if (writeFile) {
        serializeJson(doc, writeFile);
        writeFile.close();
        Serial.println("🗑️ Data berhasil dihapus dari SD Card, tersisa: " + String(dataArray.size()) + " entries");
      } else {
        Serial.println("❌ Gagal update file SD Card");
      }

    } else {
      failed++;
      Serial.println("❌ Failed [" + String(failed) + " failures] - Data tetap disimpan di SD Card");
      break;  // Stop jika gagal
    }

    // Jeda antar publish
    if (i < totalData - 1) {  // Jangan delay di data terakhir
      Serial.println("⏱️ Menunggu " + String(PUBLISH_INTERVAL) + "ms sebelum publish berikutnya...");
      delay(200);  // Lebih singkat antar publish
    }

    // Jeda batch (setiap BATCH_SIZE data)
    if ((i + 1) % BATCH_SIZE == 0 && i < totalData - 1) {
      Serial.println("📦 Batch " + String((i + 1) / BATCH_SIZE) + " selesai");
      Serial.println("⏱️ Batch delay " + String(BATCH_DELAY) + "ms...");
      delay(500);  // Lebih singkat antar batch
    }

    // Cek koneksi MQTT setiap 10 data
    if ((i + 1) % 10 == 0) {
      Serial.println("🔍 Checking MQTT connection...");
      if (!mqttConnected) {
        Serial.println("❌ MQTT connection lost, stopping publish");
        break;
      }
    }
  }

  unsigned long elapsedTime = millis() - startTime;
  Serial.println("📊 ======= PUBLISH SUMMARY =======");
  Serial.println("✅ Berhasil publish: " + String(published) + "/" + String(totalData));
  Serial.println("❌ Gagal publish: " + String(failed) + "/" + String(totalData));
  Serial.println("🗑️ Data dihapus dari SD Card: " + String(published) + " entries");
  Serial.println("💾 Data tersisa di SD Card: " + String(dataArray.size()) + " entries");
  Serial.println("⏱️ Total waktu: " + String(elapsedTime / 1000) + " detik");
  Serial.println("⚡ Rate: " + String((float)published / (elapsedTime / 1000.0), 2) + " data/detik");
  Serial.println("=================================");

  // Simpan data GPS terbaru SETELAH publish batch untuk memastikan data tidak hilang
  Serial.println("💾 Menyimpan data GPS terbaru setelah publish batch offline data...");
  gpsFix = false;
  timestamp = "";
  gpsPayload = getGPSPayload(gpsFix, timestamp);

  // Hanya simpan jika ada data GPS valid DAN mesin mati
  if (gpsPayload != "" && !digitalRead(STATUS_PIN)) {
    saveOfflineGPSData(gpsPayload);
    Serial.println("✅ Data GPS terbaru tersimpan setelah publish batch offline data (mesin mati)");
  } else if (gpsPayload != "" && digitalRead(STATUS_PIN)) {
    Serial.println("⚠️ Data GPS valid tersedia tapi mesin hidup, tidak disimpan");
  } else {
    Serial.println("⚠️ Tidak ada data GPS valid untuk disimpan setelah publish batch");
  }

  return published > 0;
}

bool publishSingleOfflineData() {
  if (!SD.exists(offlineDataFile) || !mqttConnected) {
    Serial.println("❌ File offline tidak ada atau MQTT tidak terkoneksi");
    return false;
  }

  // Ambil data PERTAMA dari SD Card (FIFO)
  File file = SD.open(offlineDataFile, FILE_READ);
  if (!file) {
    Serial.println("❌ Gagal membuka file offline data");
    return false;
  }

  String content = file.readString();
  file.close();

  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, content);
  if (err) {
    Serial.println("❌ Gagal parsing JSON offline data");
    return false;
  }

  if (!doc.containsKey("offline_gps_data")) {
    Serial.println("❌ Tidak ada offline_gps_data dalam file");
    return false;
  }

  JsonArray dataArray = doc["offline_gps_data"];
  if (dataArray.size() == 0) {
    Serial.println("ℹ️ Tidak ada data offline untuk dipublish");
    return false;
  }

  // Ambil data pertama dari array
  JsonVariant firstItem = dataArray[0];
  String payload;
  serializeJson(firstItem, payload);

  // Validasi payload
  if (payload == "null" || payload == "" || payload.length() < 10) {
    Serial.println("❌ Payload data offline tidak valid: " + payload);
    return false;
  }

  // Publish data dari SD Card dengan timestamp asli dan status 0
  Serial.println("📤 Publishing GPS data offline dari SD Card...");
  Serial.println("📄 Payload: " + payload);
  Serial.println("📅 Timestamp asli: " + firstItem["timestamp"].as<String>());
  Serial.println("📊 Status: 0 (stored data)");

  if (mqttPublish(payload)) {
    Serial.println("✅ Berhasil publish GPS data offline dari SD Card");

    // Hapus item PERTAMA dari array offline (FIFO)
    dataArray.remove(0);

    // Update file SD Card
    if (dataArray.size() > 0) {
      File writeFile = SD.open(offlineDataFile, FILE_WRITE);
      if (writeFile) {
        serializeJson(doc, writeFile);
        writeFile.close();
        Serial.println("🗑️ Data berhasil dihapus dari SD Card, tersisa: " + String(dataArray.size()) + " entries");
      } else {
        Serial.println("❌ Gagal update file SD Card");
      }
    } else {
      // Jika tidak ada data tersisa, hapus file
      SD.remove(offlineDataFile);
      Serial.println("🗑️ File offline data dihapus (tidak ada data tersisa)");
    }
    return true;
  } else {
    Serial.println("❌ Gagal publish GPS data offline dari SD Card");
    return false;
  }
}

// Fungsi untuk publish data GPS terakhir (bukan data offline)
bool publishLastGPSData() {
  if (!mqttConnected) {
    Serial.println("❌ MQTT tidak terkoneksi");
    return false;
  }

  Serial.println("📡 Mengambil data GPS terakhir untuk publish...");

  // Coba ambil data GPS terakhir dari file
  String lastGpsPayload;
  if (loadLastGPS(lastGpsPayload)) {
    Serial.println("✅ Data GPS terakhir ditemukan");
    Serial.println("Payload lama: " + lastGpsPayload);

    // Parse data GPS terakhir untuk mendapatkan koordinat
    StaticJsonDocument<512> doc;
    DeserializationError err = deserializeJson(doc, lastGpsPayload);
    if (!err && doc.containsKey("data")) {
      String lat = doc["data"]["latitude"].as<String>();
      String lon = doc["data"]["longitude"].as<String>();

      // Validasi koordinat tidak 0.0,0.0
      if (lat == "0.0" && lon == "0.0") {
        Serial.println("❌ GPS data terakhir adalah koordinat default (0.0,0.0), skip publish");
        return false;
      }

      // Gunakan timestamp GSM yang real-time dan status 9 (realtime)
      String timestamp = getRTCTimeFromGSM();

      // Validasi timestamp GSM
      if (timestamp == "") {
        Serial.println("❌ GSM timestamp tidak valid, skip publish last GPS data");
        return false;
      }

      // Buat payload dengan status 9 (realtime) bukan status 0 (stored)
      StaticJsonDocument<256> newDoc;
      newDoc["node_id"] = node_id;
      newDoc["timestamp"] = timestamp;
      newDoc["status"] = "9";  // Status realtime
      JsonObject data = newDoc.createNestedObject("data");
      data["latitude"] = lat;
      data["longitude"] = lon;

      String updatedPayload;
      serializeJson(newDoc, updatedPayload);

      Serial.println("📡 Menggunakan timestamp GSM real-time: " + timestamp);
      Serial.println("📊 Status: 9 (realtime fallback)");
      Serial.println("📍 Latitude: " + lat);
      Serial.println("📍 Longitude: " + lon);
      Serial.println("Payload baru: " + updatedPayload);
      Serial.println("📏 Payload size: " + String(updatedPayload.length()) + " bytes");

      if (mqttPublish(updatedPayload)) {
        Serial.println("✅ Berhasil publish data GPS terakhir dengan timestamp GSM realtime");
        return true;
      } else {
        Serial.println("❌ Gagal publish data GPS terakhir");
        return false;
      }
    } else {
      Serial.println("⚠️ Data GPS terakhir rusak atau tidak lengkap, skip publish");
      return false;
    }
  } else {
    Serial.println("⚠️ Tidak ada data GPS terakhir, skip publish");
    return false;
  }
}

void handleOfflineDataPublish() {
  // Notification system sudah diubah ke direct send tanpa flag
  Serial.println("ℹ️ Notification system: Direct send on status change (no flags)");
  sendToBluetooth("ℹ️ Notification system: Direct send on status change (no flags)");
  sendToBluetooth("🔍 DEBUG: Notification disabled in handleOfflineDataPublish - sent in status_machine_function.ino");

  Serial.println("🔄 ======= FORCE PUBLISH GPS DATA START =======");
  Serial.println("🔄 Mencoba publish 1 GPS data (mesin masih mati)...");
  sendToBluetooth("🔄 ======= FORCE PUBLISH GPS DATA START =======");
  sendToBluetooth("🔄 Mencoba publish 1 GPS data (mesin masih mati)...");

  Serial.println("🎯 Mendapatkan data GPS terbaru untuk publish...");
  bool gpsFix = false;
  String timestamp;
  String lat, lon, alt;

  // Gunakan getGPSData langsung untuk menghindari override timestamp
  gpsFix = getGPSData(timestamp, lat, lon, alt);

  String gpsPayload = "";
  if (gpsFix) {
    // Buat payload dengan timestamp asli dari GPS
    StaticJsonDocument<256> doc;
    doc["node_id"] = node_id;
    doc["timestamp"] = timestamp;  // Timestamp asli dari GPS
    doc["status"] = "9";
    JsonObject data = doc.createNestedObject("data");
    data["latitude"] = lat;
    data["longitude"] = lon;
    data["altitude"] = alt;
    serializeJson(doc, gpsPayload);
    Serial.println("✅ Data GPS terbaru dengan timestamp asli: " + timestamp);
  } else {
    // Jika GPS tidak fix, gunakan data terakhir dari SD Card
    Serial.println("⚠️ GPS tidak fix, mencoba menggunakan data terakhir dari SD Card...");
    gpsPayload = getFallbackPayload(timestamp);
  }

  // ✅ Simpan last GPS data (force publish mode)
  if (gpsPayload != "") {
    saveLastGPS(gpsPayload);
    Serial.println("💾 Last GPS data tersimpan (force publish mode)");
  }

  bool publishSuccess = false;

  // Publish data GPS terbaru langsung dengan status "9" (real-time)
  if (gpsPayload != "") {
    Serial.println("📤 Publishing data GPS terbaru dengan status '9' (real-time)...");
    Serial.println("📄 Payload: " + gpsPayload);

    Serial.println("🌐 Mengaktifkan internet sementara...");
    if (activateInternet()) {
      Serial.println("✅ Internet aktif sementara");
      delay(200);
      Serial.println("📡 Connecting to MQTT (publish + subscribe)...");
      bool tempMqttConnected = mqttConnect();

      if (tempMqttConnected) {
        Serial.println("✅ MQTT connected sementara (publish + subscribe)");
        bool originalMqttState = mqttConnected;
        mqttConnected = true;
        String gsmTimestamp = getRTCTimeFromGSM();

        // Validasi timestamp GSM
        if (gsmTimestamp == "") {
          Serial.println("❌ GSM timestamp tidak valid, skip publish force data");
          mqttConnected = originalMqttState;
          return;
        }

        timestamp = gsmTimestamp;
        bool currentStatus = digitalRead(STATUS_PIN);
        String statusMesinStr = currentStatus ? "1" : "0";

        // Publish data GPS terbaru langsung
        publishSuccess = mqttPublish(gpsPayload);
        if (publishSuccess) {
          Serial.println("✅ Berhasil publish data GPS terbaru dengan status '9'");

          // HAPUS: Notifikasi sudah dikirim di status_machine_function.ino saat status mesin berubah
          // Kirim notification hanya jika belum dikirim sebelumnya
          // if (!offlineNotificationSent) {
          //   Serial.println("📢 Mengirim notification offline data published...");
          //   sendStatusNotifications(0); // Force status mesin = 0 untuk offline notification
          //   offlineNotificationSent = true;
          // } else {
          //   Serial.println("ℹ️ Notification offline sudah dikirim sebelumnya, skip...");
          // }
        } else {
          Serial.println("❌ Gagal publish data GPS terbaru");
        }

        // Simpan data GPS terbaru ke SD card untuk backup (setelah publish, hanya jika mesin mati)
        if (!digitalRead(STATUS_PIN)) {
          Serial.println("💾 Menyimpan data GPS terbaru ke SD card untuk backup...");
          saveOfflineGPSData(gpsPayload);
          Serial.println("✅ Data GPS terbaru tersimpan untuk backup (mesin mati)");
        } else {
          Serial.println("⚠️ Mesin hidup, data GPS tidak disimpan untuk backup");
        }

        Serial.println("🔔 Subscribe ke topic alarm status mesin (sensors)...");
        if (mqttSubscribe(mqttSensorsTopic)) {
          Serial.println("✅ Subscribe sensors sukses, cek retain message...");
          checkMqttMessages();
        } else {
          Serial.println("❌ Subscribe sensors gagal");
        }
        Serial.println("⏱️ Subscribe ke topic timer offline (interval)...");
        if (mqttSubscribe(mqttIntervalTopic)) {
          Serial.println("✅ Subscribe interval sukses, cek retain message...");
          checkMqttMessages();
        } else {
          Serial.println("❌ Subscribe interval gagal");
        }
        bool gpsFixStatus = false;
        String tsStatus;
        getGPSPayload(gpsFixStatus, tsStatus);
        String statusGPS = gpsFixStatus ? "1" : "0";
        Serial.println("📡 Publishing statusGPS saat force publish...");
        Serial.println("🛰️ Status GPS: " + String(gpsFixStatus ? "CONNECTED" : "DISCONNECTED"));
        bool statusGPSPublishSuccess = publishStatusGPS(tsStatus, statusGPS);
        Serial.println("📡 Publishing alarm status mesin terbaru...");
        int alarmStatusInt = digitalRead(STATUS_PIN) ? 1 : 0;
        String alarmStatusStr = determineAlarmStatus(alarmStatusInt, statusMesinAlarmType, statusMesinAlarmHigh, statusMesinAlarmLow);
        StaticJsonDocument<256> doc;
        doc["node_id"] = node_id;
        doc["timestamp"] = tsStatus;
        doc["status"] = alarmStatusStr;
        JsonObject data = doc.createNestedObject("data");
        data["statusMesin"] = statusMesinStr;
        String statusPayload;
        serializeJson(doc, statusPayload);
        bool alarmStatusPublishSuccess = mqttPublish(statusPayload);
        if (alarmStatusPublishSuccess) {
          Serial.println("✅ Alarm status mesin berhasil dipublish");
        } else {
          Serial.println("❌ Alarm status mesin gagal dipublish");
        }
        Serial.println("📊 ======= PUBLISH SUMMARY =======");
        Serial.println("📡 Status Mesin: " + String(alarmStatusPublishSuccess ? "✅ SUCCESS" : "❌ FAILED"));
        Serial.println("🛰️ Status GPS: " + String(statusGPSPublishSuccess ? "✅ SUCCESS" : "❌ FAILED"));
        Serial.println("📍 GPS Data: " + String(publishSuccess ? "✅ SUCCESS" : "❌ FAILED"));
        Serial.println("=================================");
        if (publishSuccess || alarmStatusPublishSuccess || statusGPSPublishSuccess) {
          Serial.println("✅ Berhasil publish data (minimal 1 berhasil)");
        } else {
          Serial.println("❌ Semua publish gagal");
        }
        mqttConnected = originalMqttState;
        delay(200);
        Serial.println("🔌 Disconnecting MQTT...");
        sendAT("AT+QMTDISC=0", 2000);
        Serial.println("🔌 MQTT disconnected");
      } else {
        Serial.println("❌ Gagal connect MQTT sementara");
      }
      Serial.println("🔌 Menonaktifkan internet...");
      deactivateInternet();
      mqttConnected = false;
      Serial.println("🔌 Internet dimatikan kembali");
    } else {
      Serial.println("❌ Gagal aktifkan internet sementara");
    }
  } else {
    Serial.println("⚠️ Tidak ada data GPS valid untuk dipublish");
  }

  Serial.println("🔄 ======= FORCE PUBLISH GPS DATA END =======");
}

// Fungsi untuk publish offline data dengan kontrol yang lebih baik
bool publishOfflineDataWithControl(int maxPublish = -1) {
  if (!SD.exists(offlineDataFile) || !mqttConnected) {
    Serial.println("❌ File offline tidak ada di SD Card atau MQTT tidak terkoneksi");
    return false;
  }

  Serial.println("💾 Menyimpan data GPS terbaru sebelum publish dengan kontrol...");
  bool gpsFix = false;
  String timestamp;
  String gpsPayload = getGPSPayload(gpsFix, timestamp);

  // Hanya simpan jika ada data GPS valid DAN mesin mati
  if (gpsPayload != "" && !digitalRead(STATUS_PIN)) {
    saveOfflineGPSData(gpsPayload);
    Serial.println("✅ Data GPS terbaru tersimpan sebelum publish dengan kontrol (mesin mati)");
  } else if (gpsPayload != "" && digitalRead(STATUS_PIN)) {
    Serial.println("⚠️ Data GPS valid tersedia tapi mesin hidup, tidak disimpan");
  } else {
    Serial.println("⚠️ Tidak ada data GPS valid untuk disimpan sebelum publish dengan kontrol");
  }

  File file = SD.open(offlineDataFile, FILE_READ);
  if (!file) return false;

  String content = file.readString();
  file.close();

  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, content);
  if (err) return false;

  if (!doc.containsKey("offline_gps_data")) return false;

  JsonArray dataArray = doc["offline_gps_data"];
  int totalData = dataArray.size();
  int targetPublish = (maxPublish > 0) ? min(maxPublish, totalData) : totalData;
  int published = 0;

  Serial.println("📤 Publishing " + String(targetPublish) + " dari " + String(totalData) + " GPS data offline...");

  for (int i = 0; i < targetPublish; i++) {
    JsonVariant item = dataArray[i];
    String payload;
    serializeJson(item, payload);

    Serial.println(" [" + String(i + 1) + "/" + String(targetPublish) + "] Publishing GPS data...");

    if (mqttPublish(payload)) {
      published++;
      Serial.println("✅ Success");
    } else {
      Serial.println("❌ Failed");
      break;
    }

    if (i < targetPublish - 1) {
      Serial.println("⏱️ Delay " + String(PUBLISH_INTERVAL) + "ms...");
      delay(PUBLISH_INTERVAL);
    }
  }

  Serial.println("📊 Berhasil publish " + String(published) + "/" + String(targetPublish) + " GPS data");

  for (int i = 0; i < published; i++) {
    dataArray.remove(0);
  }

  if (dataArray.size() > 0) {
    File writeFile = SD.open(offlineDataFile, FILE_WRITE);
    serializeJson(doc, writeFile);
    writeFile.close();
    Serial.println("💾 Tersisa " + String(dataArray.size()) + " GPS data offline");
  } else {
    SD.remove(offlineDataFile);
    Serial.println("🗑️ Semua GPS data offline telah dipublish, file dihapus");
  }

  Serial.println("💾 Menyimpan data GPS terbaru setelah publish dengan kontrol...");
  gpsFix = false;
  timestamp = "";
  gpsPayload = getGPSPayload(gpsFix, timestamp);

  // Hanya simpan jika ada data GPS valid DAN mesin mati
  if (gpsPayload != "" && !digitalRead(STATUS_PIN)) {
    saveOfflineGPSData(gpsPayload);
    Serial.println("✅ Data GPS terbaru tersimpan setelah publish dengan kontrol (mesin mati)");
  } else if (gpsPayload != "" && digitalRead(STATUS_PIN)) {
    Serial.println("⚠️ Data GPS valid tersedia tapi mesin hidup, tidak disimpan");
  } else {
    Serial.println("⚠️ Tidak ada data GPS valid untuk disimpan setelah publish dengan kontrol");
  }

  return published > 0;
}

// Fungsi untuk test apakah topic menerima message
void testTopicMessage() {
  if (!mqttConnected) {
    Serial.println("❌ MQTT tidak terkoneksi untuk test");
    return;
  }

  Serial.println("🧪 ======= TESTING TOPIC MESSAGE =======");
  Serial.println("📡 Testing topic: " + String(mqttIntervalTopic));

  // Clear buffer
  while (SerialAT.available()) {
    SerialAT.read();
  }

  // Cek apakah ada data yang masuk dalam 10 detik
  Serial.println("⏱️ Menunggu message selama 10 detik...");
  unsigned long startTime = millis();
  bool messageReceived = false;

  while (millis() - startTime < 10000) {
    if (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();

      if (line.length() > 0) {
        Serial.println("📡 Received: " + line);

        if (line.indexOf("+QMTRECV:") >= 0) {
          Serial.println("✅ MQTT Message detected!");
          messageReceived = true;
          processMqttReceive(line);
        }
      }
    }
    delay(100);
  }

  if (!messageReceived) {
    Serial.println("❌ Tidak ada message yang diterima dalam 10 detik");
  }

  Serial.println("🧪 ======================================");
}

// Fungsi untuk mengecek status subscribe
bool checkSubscriptionStatus() {
  if (!mqttConnected) {
    Serial.println("❌ MQTT tidak terkoneksi untuk cek subscription");
    return false;
  }

  Serial.println("🔍 Checking subscription status...");

  // Clear buffer
  while (SerialAT.available()) {
    SerialAT.read();
  }

  SerialAT.println("AT+QMTSUB?");
  delay(1000);

  String response = "";
  unsigned long start = millis();

  while (millis() - start < 3000) {
    if (SerialAT.available()) {
      response += SerialAT.readString();
      if (response.indexOf("OK") >= 0) {
        break;
      }
    }
    delay(10);
  }

  Serial.println("📡 Subscription Status Response: " + response);

  if (response.indexOf("+QMTSUB:") >= 0) {
    Serial.println("✅ Subscription status found");

    // Parse subscription details
    int startIdx = response.indexOf("+QMTSUB:");
    if (startIdx >= 0) {
      String subInfo = response.substring(startIdx);
      Serial.println("📋 Subscription Info: " + subInfo);

      // Cek apakah topic interval ada dalam subscription
      if (subInfo.indexOf(mqttIntervalTopic) >= 0) {
        Serial.println("✅ Topic interval masih subscribed");
        return true;
      } else {
        Serial.println("❌ Topic interval tidak ditemukan dalam subscription");
        return false;
      }
    }
    return true;  // Jika ada subscription tapi tidak bisa parse detail
  } else {
    Serial.println("⚠️ No subscription status found");
    return false;
  }
}






// Fungsi untuk memastikan MQTT subscribe tetap aktif
bool ensureMqttSubscription() {
  if (!mqttConnected) {
    Serial.println("❌ MQTT tidak terkoneksi untuk ensure subscription");
    return false;
  }

  // Cek apakah subscription masih aktif
  if (checkSubscriptionStatus()) {
    Serial.println("✅ MQTT subscription masih aktif");
    subscriptionActive = true;
    return true;
  }

  Serial.println("⚠️ MQTT subscription tidak aktif, mencoba subscribe ulang...");

  // Coba subscribe ulang
  if (mqttSubscribe(mqttIntervalTopic)) {
    Serial.println("✅ Berhasil subscribe ulang ke topic interval");
    subscriptionActive = true;
    return true;
  } else {
    Serial.println("❌ Gagal subscribe ulang ke topic interval");
    subscriptionActive = false;
    return false;
  }
}

// Fungsi untuk monitoring MQTT connection secara berkala
void monitorMqttConnection() {
  unsigned long currentMillis = millis();
  unsigned long elapsedSinceLastCheck;

  // Handle millis() overflow
  if (currentMillis >= lastMqttCheck) {
    elapsedSinceLastCheck = currentMillis - lastMqttCheck;
  } else {
    elapsedSinceLastCheck = (0xFFFFFFFF - lastMqttCheck) + currentMillis + 1;
  }

  if (elapsedSinceLastCheck > MQTT_CHECK_INTERVAL) {
    lastMqttCheck = currentMillis;

    // Hanya monitor jika mesin hidup dan internet aktif
    if (digitalRead(STATUS_PIN) && internetActive) {
      if (mqttConnected) {
        Serial.println("🔍 Periodic MQTT connection check...");

        // Cek status koneksi MQTT dengan timeout yang lebih pendek
        if (!checkMqttConnection()) {
          Serial.println("⚠️ MQTT connection lost, attempting reconnect...");
          mqttConnected = false;
          subscriptionActive = false;

          // Coba reconnect dengan delay
          delay(2000);
          if (mqttConnect()) {
            Serial.println("✅ MQTT reconnection successful");

            // Pastikan status mesin terpublish setelah reconnect
            ensureStatusMesinPublished();
          } else {
            Serial.println("❌ MQTT reconnection failed");
          }
        } else {
          // Cek subscription status hanya jika perlu
          if (!subscriptionActive) {
            Serial.println("⚠️ MQTT subscription inactive, ensuring subscription...");
            ensureMqttSubscription();
          }
        }
      }
    } else {
      // Jika mesin mati, tidak perlu monitor MQTT
      if (timerDebugEnabled) {
        Serial.println("ℹ️ MQTT monitoring skipped (mesin mati atau internet tidak aktif)");
      }
    }
  }
}

// Fungsi untuk build statusGPS payload
String buildStatusGPSPayload(String timestamp, String statusGPS) {
  if (timerDebugEnabled) {
    Serial.println("🔍 Debug - buildStatusGPSPayload()");
  }
  Serial.println("📅 Timestamp: " + timestamp);
  Serial.println("🛰️ StatusGPS: " + statusGPS);

  StaticJsonDocument<512> doc;  // Increased size untuk menampung data tambahan
  doc["node_id"] = node_id;
  doc["timestamp"] = timestamp;

  // Set status berdasarkan statusGPS
  if (statusGPS == "0") {
    doc["status"] = "4";  // Error (gagal fix)
  } else {
    doc["status"] = "1";  // Normal (berhasil fix)
  }

  JsonObject data = doc.createNestedObject("data");
  data["statusGPS"] = statusGPS;

  // Tambahkan informasi waktu boot sampai GPS fix
  if (gpsFixAchieved) {
    data["gpsFixTime"] = String(bootToGPSFixTime);  // Waktu dalam detik
  } else {
    // Jika GPS belum fix, hitung waktu yang sudah berlalu sejak boot
    unsigned long currentTime = millis();
    unsigned long elapsed;
    if (currentTime >= bootTime) {
      elapsed = currentTime - bootTime;
    } else {
      elapsed = (0xFFFFFFFF - bootTime) + currentTime + 1;
    }
    data["bootToGPSFixTime"] = 0;            // Belum fix
    data["timeSinceBoot"] = elapsed / 1000;  // Waktu sejak boot dalam detik
    data["gpsFixAchieved"] = false;
  }

  String json;
  serializeJson(doc, json);

  Serial.println("📄 Generated JSON: " + json);
  Serial.println("📏 JSON Length: " + String(json.length()) + " bytes");

  return json;
}

// Fungsi untuk publish statusGPS
bool publishStatusGPS(String timestamp, String statusGPS) {
  String statusGPSPayload = buildStatusGPSPayload(timestamp, statusGPS);

  if (!mqttConnected) {
    Serial.println("⚠️ MQTT tidak terkoneksi, statusGPS tidak bisa dipublish");
    return false;
  }

  // Coba publish dengan retry mechanism
  const int maxRetries = 3;
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    Serial.println("📡 Mencoba publish statusGPS... Percobaan ke-" + String(attempt) + "/" + String(maxRetries));

    if (mqttPublish(statusGPSPayload)) {
      Serial.println("✅ StatusGPS berhasil dipublish");
      return true;
    } else {
      Serial.println("❌ StatusGPS gagal dipublish (Percobaan " + String(attempt) + ")");

      if (attempt < maxRetries) {
        Serial.println("⏱️ Menunggu 2 detik sebelum mencoba lagi...");
        delay(2000);

        // Cek ulang koneksi MQTT
        if (!mqttConnected) {
          Serial.println("🔄 MQTT terputus, mencoba reconnect...");
          mqttConnected = mqttConnectForPublish();
          delay(200);
        }
      }
    }
  }

  Serial.println("❌ StatusGPS gagal dipublish setelah " + String(maxRetries) + " percobaan");
  return false;
}

// Implementasi clean untuk mqttSubscribe
bool mqttSubscribe(String topic) {
  if (!mqttConnected) {
    Serial.println("❌ MQTT tidak terkoneksi untuk subscribe");
    return false;
  }

  Serial.println("🔄 Subscribe ke topic: " + topic);

  // Clear buffer sebelum subscribe
  while (SerialAT.available()) {
    SerialAT.read();
  }

  // Subscribe dengan QoS 2
  String subCmd = "AT+QMTSUB=0,1,\"" + topic + "\",2";
  Serial.println("📡 Subscribe command: " + subCmd);
  delay(200);  // Lebih singkat

  if (sendAndWait(subCmd, "+QMTSUB: 0,1,0,2", 15000)) {
    Serial.println("✅ Subscribe berhasil dengan QoS 2");
    subscriptionActive = true;
    // Langsung lanjut ke pengecekan pesan interval dari serial
    parseIntervalFromSerialAdvanced();
    checkMqttMessages();
    return true;
  } else {
    Serial.println("❌ Subscribe gagal dengan QoS 2");
    subscriptionActive = false;
    return false;
  }
}

// Fungsi untuk publish seluruh data tersimpan di SPIFFS saat statusMesin 1
bool publishAllStoredDataWhenMachineOn() {
  Serial.println("🚀 ======= FUNGSI PUBLISH ALL STORED DATA DIPANGGIL =======");

  if (!mqttConnected) {
    Serial.println("❌ MQTT tidak terkoneksi untuk publish data tersimpan");
    Serial.println("📡 MQTT Status: " + String(mqttConnected ? "CONNECTED" : "DISCONNECTED"));
    return false;
  }

  Serial.println("📤 ======= PUBLISH ALL STORED DATA (MACHINE ON) =======");
  Serial.println("✅ MQTT terkoneksi, memulai proses publish...");
  Serial.println("📋 IMPORTANT: Data tersimpan akan dipublish dengan TIMESTAMP ASLI (tidak di-override)");

  // 1. Publish statusMesin terlebih dahulu
  String timestamp = getRTCTimeFromGSM();

  // Validasi timestamp GSM
  if (timestamp == "") {
    Serial.println("❌ GSM timestamp tidak valid, skip publish all stored data");
    return false;
  }

  bool currentStatus = digitalRead(STATUS_PIN);
  String statusMesinStr = currentStatus ? "1" : "0";

  Serial.println("📡 Step 1: Publishing statusMesin...");
  Serial.println("⚙️ Status mesin: " + String(currentStatus ? "HIDUP" : "MATI"));

  // Gunakan alarm status untuk statusMesin
  int status = digitalRead(STATUS_PIN) ? 1 : 0;
  String alarmStatus = determineAlarmStatus(status, statusMesinAlarmType, statusMesinAlarmHigh, statusMesinAlarmLow);

  StaticJsonDocument<256> statusDoc;
  statusDoc["node_id"] = node_id;
  statusDoc["timestamp"] = timestamp;
  statusDoc["status"] = alarmStatus;
  JsonObject statusData = statusDoc.createNestedObject("data");
  statusData["statusMesin"] = statusMesinStr;
  String statusPayload;
  serializeJson(statusDoc, statusPayload);

  bool statusMesinPublished = mqttPublish(statusPayload);
  if (statusMesinPublished) {
    Serial.println("✅ StatusMesin berhasil dipublish");
  } else {
    Serial.println("❌ StatusMesin gagal dipublish");
  }

  // 2. Publish statusGPS
  Serial.println("📡 Step 2: Publishing statusGPS...");
  String statusGPS = gpsConnected ? "1" : "0";
  Serial.println("🛰️ Status GPS: " + String(gpsConnected ? "CONNECTED" : "DISCONNECTED"));

  bool statusGPSPublished = publishStatusGPS(timestamp, statusGPS);
  if (statusGPSPublished) {
    Serial.println("✅ StatusGPS berhasil dipublish");
  } else {
    Serial.println("❌ StatusGPS gagal dipublish");
  }

  // 3. Publish seluruh data tersimpan di SD Card dengan chunked loading
  Serial.println("📡 Step 3: Publishing seluruh data tersimpan di SD Card...");

  bool storedDataPublished = false;

  // SELALU COBA PUBLISH DATA SD CARD, bahkan jika file tidak ada
  if (SD.exists(offlineDataFile)) {
    // Cek status mesin sebelum mulai publish data tersimpan
    bool currentMachineStatus = digitalRead(STATUS_PIN);
    if (!currentMachineStatus) {
      Serial.println("🛑 INTERRUPT: Status mesin MATI sebelum publish data tersimpan");
      Serial.println("🔄 Menghentikan publish dan kembali ke mode offline");
      Serial.println("📤 ======= PUBLISH SUMMARY (INTERRUPTED) =======");
      Serial.println("📡 StatusMesin: " + String(statusMesinPublished ? "✅ SUCCESS" : "❌ FAILED"));
      Serial.println("🛰️ StatusGPS: " + String(statusGPSPublished ? "✅ SUCCESS" : "❌ FAILED"));
      Serial.println("📦 Stored Data: 🛑 INTERRUPTED");
      Serial.println("=============================================");
      return statusMesinPublished || statusGPSPublished;
    }

    // Cek ukuran file untuk menentukan metode loading
    File sizeFile = SD.open(offlineDataFile, FILE_READ);
    if (sizeFile) {
      size_t fileSize = sizeFile.size();
      sizeFile.close();

      Serial.println("📁 File size: " + String(fileSize) + " bytes");

      if (fileSize > 0) { // Pastikan file tidak kosong
        // Gunakan chunked loading untuk file besar
        if (fileSize > 25000) { // Turunkan threshold dari 30KB ke 25KB
          Serial.println("⚠️ File terlalu besar (" + String(fileSize) + " bytes), menggunakan chunked loading");
          Serial.println("🛑 NOTE: Chunked loading dapat diinterrupt jika status mesin berubah menjadi MATI");
          
          // Cek RAM sebelum mulai chunked loading
          size_t freeHeap = ESP.getFreeHeap();
          Serial.println("💾 Free RAM sebelum chunked loading: " + String(freeHeap) + " bytes");
          
          if (freeHeap < 30000) {
            Serial.println("⚠️ RAM rendah, menggunakan chunk size 1 untuk file besar");
            storedDataPublished = publishOfflineDataChunked(1);  // Force chunk size 1
          } else {
            storedDataPublished = publishOfflineDataChunked(1);  // Default chunk size 1 untuk file besar
          }
        } else {
          Serial.println("✅ File size normal, menggunakan normal loading");
          storedDataPublished = publishOfflineDataNormal();
        }
      } else {
        Serial.println("ℹ️ File offline data kosong, tidak ada data untuk dipublish");
      }
    } else {
      Serial.println("⚠️ Gagal membuka file untuk cek ukuran, tetap akan coba publish");
      // Coba publish dengan metode normal sebagai fallback
      storedDataPublished = publishOfflineDataNormal();
    }
  } else {
    Serial.println("ℹ️ File offline data tidak ada, tidak ada data untuk dipublish");
  }

  // Summary hasil publish
  Serial.println("📤 ======= PUBLISH SUMMARY =======");
  Serial.println("📡 StatusMesin: " + String(statusMesinPublished ? "✅ SUCCESS" : "❌ FAILED"));
  Serial.println("🛰️ StatusGPS: " + String(statusGPSPublished ? "✅ SUCCESS" : "❌ FAILED"));
  Serial.println("📦 Stored Data: " + String(storedDataPublished ? "✅ SUCCESS" : "❌ FAILED"));
  Serial.println("=================================");

  return statusMesinPublished || statusGPSPublished || storedDataPublished;
}
