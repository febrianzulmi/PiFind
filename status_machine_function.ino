// status_machine_function.ino
#include "configure.h"

// Fungsi untuk validasi status mesin yang stabil
bool isStatusMachineStable() {
  unsigned long currentTime = millis();
  
  // Cek apakah sudah cukup waktu sejak perubahan terakhir
  if (currentTime - lastStatusChange < STATUS_DEBOUNCE_TIME) {
    return false; // Masih dalam debounce time
  }
  
  return true;
}

// Fungsi untuk mendapatkan status mesin yang sudah divalidasi
bool getStableMachineStatus() {
  bool currentStatus = digitalRead(STATUS_PIN);
  
  // Jika status sama dengan yang terakhir, increment counter
  if (currentStatus == lastStableStatus) {
    statusReadCount++;
  } else {
    // Status berubah, reset counter dan update last status
    statusReadCount = 1;
    lastStableStatus = currentStatus;
    lastStatusChange = millis();
  }
  
  // Status dianggap stabil jika sudah dibaca minimal 3x sama
  if (statusReadCount >= STATUS_VALIDATION_COUNT) {
    return currentStatus;
  }
  
  // Return status terakhir yang stabil
  return lastStableStatus;
}

String buildStatusPayload(String timestamp, String statusMesinStr) {
  if (timerDebugEnabled) {
    Serial.println("🔍 Debug - buildStatusPayload()");
  }
  Serial.println("📅 Timestamp: " + timestamp);
  Serial.println("⚙️ StatusMesin: " + statusMesinStr);
  
  // Ambil status mesin dari pin
  bool mesinStatus = digitalRead(STATUS_PIN);
  String statusValue = mesinStatus ? "1" : "0";
  
  // Gunakan helper functions untuk konsistensi
  bool adapterStatus = isAdapterConnected();
  bool chargingActive = isChargingActive();
  
  Serial.println("🔋 Status Mesin: " + String(mesinStatus ? "HIDUP" : "MATI"));
  Serial.println("🔌 Status Adapter: " + String(adapterStatus ? "TERPASANG" : "TIDAK TERPASANG"));
  
  // Debug charging status
  sendToBluetooth("🔍 DEBUG: buildStatusPayload - mesinStatus=" + String(mesinStatus) + ", adapterStatus=" + String(adapterStatus) + ", chargingActive=" + String(chargingActive));
  
  StaticJsonDocument<512> doc;
  doc["node_id"] = node_id;
  doc["timestamp"] = timestamp;
  doc["status"] = "9";
  JsonObject data = doc.createNestedObject("data");
  data["statusMesin"] = statusValue;
  data["statusAdapter"] = String(adapterStatus ? "1" : "0");
  data["statusCharging"] = String(chargingActive ? "1" : "0");
  
  // Tambahkan informasi charging detail - SELALU tampilkan lengkap
  if (chargingActive) {
    data["chargingStatus"] = "ACTIVE";
    data["chargingDescription"] = "Mesin hidup + Adapter terpasang";
  } else {
    data["chargingStatus"] = "INACTIVE";
    if (!mesinStatus) {
      data["chargingDescription"] = "Mesin mati";
    } else if (!adapterStatus) {
      data["chargingDescription"] = "Adapter tidak terpasang";
    } else {
      data["chargingDescription"] = "Tidak diketahui";
    }
  }

  String json;
  serializeJson(doc, json);

  // Validasi JSON tidak null atau kosong
  if (json == "null" || json == "" || json.length() < 10) {
    Serial.println("❌ JSON payload tidak valid: " + json);
    Serial.println("📏 JSON length: " + String(json.length()));
    // Return payload default jika error
    json = "{\"node_id\":\"" + String(node_id) + "\",\"timestamp\":\"" + timestamp + "\",\"status\":\"9\",\"data\":{\"statusMesin\":\"" + statusValue + "\",\"statusAdapter\":\"" + String(adapterStatus ? "1" : "0") + "\",\"statusCharging\":\"" + String(chargingActive ? "1" : "0") + "\",\"chargingStatus\":\"" + (chargingActive ? "ACTIVE" : "INACTIVE") + "\",\"chargingDescription\":\"" + (chargingActive ? "Mesin hidup + Adapter terpasang" : "Mesin mati atau adapter tidak terpasang") + "\"}}";
  }

  Serial.println("📄 Generated JSON: " + json);
  Serial.println("📏 JSON Length: " + String(json.length()) + " bytes");
  
  // Debug JSON payload charging status
  sendToBluetooth("🔍 DEBUG: JSON payload charging status:");
  sendToBluetooth("   - statusMesin: " + statusValue);
  sendToBluetooth("   - statusAdapter: " + String(adapterStatus ? "1" : "0"));
  sendToBluetooth("   - statusCharging: " + String(chargingActive ? "1" : "0"));
  sendToBluetooth("   - chargingStatus: " + String(chargingActive ? "ACTIVE" : "INACTIVE"));
  
  return json;
}

// Fungsi untuk publish status mesin ke MQTT dengan retry mechanism
bool publishStatusMesin(String timestamp, String statusMesinStr) {
  String statusPayload = buildStatusPayload(timestamp, statusMesinStr);
  
  if (!mqttConnected) {
    Serial.println("⚠️ MQTT tidak terkoneksi, status mesin tidak bisa dipublish");
    return false;
  }

  // Coba publish dengan retry mechanism
  const int maxRetries = 3;
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    Serial.println("📡 Mencoba publish status mesin... Percobaan ke-" + String(attempt) + "/" + String(maxRetries));
    
    if (mqttPublish(statusPayload)) {
      Serial.println("✅ Status mesin berhasil dipublish");
      return true;
    } else {
      Serial.println("❌ Status mesin gagal dipublish (Percobaan " + String(attempt) + ")");
      
      if (attempt < maxRetries) {
        Serial.println("⏱️ Menunggu 2 detik sebelum mencoba lagi...");
        delay(2000);
        
        // Cek ulang koneksi MQTT
        if (!mqttConnected) {
          Serial.println("🔄 MQTT terputus, mencoba reconnect...");
          mqttConnected = mqttConnect();
          delay(1000);
        }
      }
    }
  }
  
  Serial.println("❌ Status mesin gagal dipublish setelah " + String(maxRetries) + " percobaan");
  return false;
}

// Fungsi untuk memastikan status mesin terkirim saat reconnect
void ensureStatusMesinPublished() {
  if (mqttConnected) {
    String timestamp = getRTCTimeFromGSM();
    
    // Validasi timestamp GSM
    if (timestamp == "") {
      Serial.println("❌ GSM timestamp tidak valid, skip publish status mesin");
      sendToBluetooth("❌ GSM timestamp tidak valid, skip publish status mesin");
      return;
    }
    
    bool currentStatus = digitalRead(STATUS_PIN);
    String statusMesinStr = currentStatus ? "1" : "0";
    
    Serial.println("📡 Memastikan status mesin terpublish setelah reconnect...");
    Serial.println("⚙️ Status mesin saat ini: " + String(currentStatus ? "HIDUP" : "MATI"));
    
    bool publishSuccess = publishStatusMesin(timestamp, statusMesinStr);
    if (publishSuccess) {
      Serial.println("✅ Status mesin berhasil dipublish setelah reconnect");
      
      // Kirim notification sesuai status mesin saat ini
      if (currentStatus) {
        Serial.println("📢 Mengirim notification charging on setelah reconnect...");
        sendToBluetooth("📢 Mengirim notification charging on setelah reconnect...");
        sendStatusNotifications(1);
        Serial.println("✅ Notification charging on berhasil dikirim setelah reconnect");
        sendToBluetooth("✅ Notification charging on berhasil dikirim setelah reconnect");
      } else {
        Serial.println("📢 Mengirim notification charging off setelah reconnect...");
        sendToBluetooth("📢 Mengirim notification charging off setelah reconnect...");
        sendStatusNotifications(0);
        Serial.println("✅ Notification charging off berhasil dikirim setelah reconnect");
        sendToBluetooth("✅ Notification charging off berhasil dikirim setelah reconnect");
      }
    } else {
      Serial.println("❌ Status mesin gagal dipublish setelah reconnect");
    }
  }
}

void handleMachineStatusChange(bool currentStatus, bool previousStatus) {
  Serial.println("🔄 ======= STATUS MACHINE CHANGE =======");
  sendToBluetooth("🔄 ======= STATUS MACHINE CHANGE =======");
  sendToBluetooth("🔍 DEBUG: currentStatus=" + String(currentStatus) + ", previousStatus=" + String(previousStatus));
  
  // Reset notification flag saat status berubah
  notificationSentForCurrentStatus = false;
  lastNotificationStatus = -1;
  Serial.println("🔄 Notification flag reset untuk status baru");
  sendToBluetooth("🔄 Notification flag reset untuk status baru");
  
  // Validasi status mesin yang stabil
  if (!isStatusMachineStable()) {
    Serial.println("⚠️ Status mesin belum stabil, skip processing (debounce: " + String(STATUS_DEBOUNCE_TIME/1000) + "s)");
    sendToBluetooth("⚠️ Status mesin belum stabil, skip processing (debounce: " + String(STATUS_DEBOUNCE_TIME/1000) + "s)");
    return; // Skip processing jika belum stabil
  }
  
  // Dapatkan status yang sudah divalidasi
  bool stableStatus = getStableMachineStatus();
  if (currentStatus != stableStatus) {
    Serial.println("⚠️ Status tidak stabil: current=" + String(currentStatus) + ", stable=" + String(stableStatus));
    sendToBluetooth("⚠️ Status tidak stabil: current=" + String(currentStatus) + ", stable=" + String(stableStatus));
    return; // Skip processing jika tidak stabil
  }
  
  if (currentStatus != previousStatus) {
    // Double check status mesin untuk memastikan perubahan yang valid
    bool actualStatus = digitalRead(STATUS_PIN);
    if (actualStatus != currentStatus) {
      Serial.println("⚠️ Status mesin tidak konsisten, skip perubahan");
      sendToBluetooth("⚠️ Status mesin tidak konsisten, skip perubahan");
      return;
    }
    
    Serial.println(currentStatus ? "🔄 Mesin dinyalakan" : "🔄 Mesin dimatikan");
    sendToBluetooth(currentStatus ? "🔄 Mesin dinyalakan" : "🔄 Mesin dimatikan");
    
    // Monitor RAM saat perubahan status mesin
    Serial.println("📊 ======= RAM MONITORING STATUS CHANGE =======");
    sendToBluetooth("📊 ======= RAM MONITORING STATUS CHANGE =======");
    Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
    sendToBluetooth("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
    Serial.println("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
    Serial.println("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
    sendToBluetooth("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
    
    // Validasi RAM sebelum operasi
    if (ESP.getFreeHeap() < 25000) { // Jika RAM < 25KB
      Serial.println("⚠️ RAM rendah saat perubahan status: " + String(ESP.getFreeHeap()) + " bytes");
      sendToBluetooth("⚠️ RAM rendah saat perubahan status: " + String(ESP.getFreeHeap()) + " bytes");
      Serial.println("⚠️ Operasi MQTT mungkin gagal karena kekurangan RAM");
      sendToBluetooth("⚠️ Operasi MQTT mungkin gagal karena kekurangan RAM");
    }
    Serial.println("===============================================");
    sendToBluetooth("===============================================");
    
    // Update LED charger status secara real-time
    updateChargerLED();
    
    // Dapatkan timestamp untuk status mesin
    String timestamp = getRTCTimeFromGSM();
    
    // Validasi timestamp GSM
    if (timestamp == "") {
      Serial.println("❌ GSM timestamp tidak valid, skip publish status mesin perubahan");
      sendToBluetooth("❌ GSM timestamp tidak valid, skip publish status mesin perubahan");
      return;
    }
    
    String statusMesinStr = currentStatus ? "1" : "0";
    
    if (currentStatus) {
      // Mesin hidup - pastikan internet dan MQTT siap dulu
      Serial.println("🟢 Mesin hidup - Menyiapkan koneksi...");
      sendToBluetooth("🟢 Mesin hidup - Menyiapkan koneksi...");
      
      // Stop timer offline ketika mesin hidup
      stopOfflineTimer();
      
      // Aktifkan internet dengan delay
      if (!internetActive) {
        Serial.println("🌐 Mengaktifkan internet untuk publish status...");
        if (activateInternet()) {
          Serial.println("✅ Internet aktif, menunggu stabilisasi...");
          delay(3000);  // Tunggu stabilisasi koneksi
          
          // Connect MQTT
          if (!mqttConnected) {
            Serial.println("📡 Menghubungkan MQTT untuk publish status...");
            mqttConnected = mqttConnect();
            delay(2000);  // Tunggu koneksi MQTT stabil
          }
          
          // Sekarang publish status mesin dan kirim notification
          if (mqttConnected) {
            // STEP 1: Update adapter status terlebih dahulu
            Serial.println("🔌 Step 1: Update adapter status...");
            sendToBluetooth("🔌 Step 1: Update adapter status...");
            updateAdapterStatus(); // Update isAdapterOk dengan benar
            delay(500); // Tunggu update selesai
            
            // STEP 2: Publish status mesin dengan charging info
            Serial.println("📡 Step 2: Publishing status mesin dengan charging info...");
            sendToBluetooth("📡 Step 2: Publishing status mesin dengan charging info...");
            
                      // Debug charging status sebelum publish
          bool mesinStatus = digitalRead(STATUS_PIN);
          bool chargingActive = isChargingActive();
          Serial.println("🔍 DEBUG: Sebelum publish - mesinStatus=" + String(mesinStatus) + ", isAdapterOk=" + String(isAdapterOk) + ", chargingActive=" + String(chargingActive));
          sendToBluetooth("🔍 DEBUG: Sebelum publish - mesinStatus=" + String(mesinStatus) + ", isAdapterOk=" + String(isAdapterOk) + ", chargingActive=" + String(chargingActive));
            
            bool publishSuccess = publishStatusMesin(timestamp, statusMesinStr);
            
            if (!publishSuccess) {
              Serial.println("⚠️ Status mesin hidup tidak terpublish, akan dipublish saat online");
              sendToBluetooth("⚠️ Status mesin hidup tidak terpublish, akan dipublish saat online");
            } else {
              Serial.println("✅ Status mesin dengan charging info berhasil dipublish");
              sendToBluetooth("✅ Status mesin dengan charging info berhasil dipublish");
            }
            
            // STEP 3: Kirim notification charging on (LANGSUNG - TANPA FLAG)
            Serial.println("📢 Step 3: Mengirim notification charging on...");
            sendToBluetooth("📢 Step 3: Mengirim notification charging on...");
            sendStatusNotifications(1);
            Serial.println("✅ Notification charging on berhasil dikirim");
            sendToBluetooth("✅ Notification charging on berhasil dikirim");
            
            // STEP 4: Setelah charging status dan notification selesai, baru cek data SD Card
            Serial.println("📋 Step 4: Akan mengecek data SD Card dan publish jika ada...");
            sendToBluetooth("📋 Step 4: Akan mengecek data SD Card dan publish jika ada...");
            
            // Summary logging
            Serial.println("📊 ======= STATUS MACHINE CHANGE SUMMARY =======");
            sendToBluetooth("📊 ======= STATUS MACHINE CHANGE SUMMARY =======");
            Serial.println("✅ Step 1: Adapter status updated");
            sendToBluetooth("✅ Step 1: Adapter status updated");
            Serial.println("✅ Step 2: Status mesin dengan charging info published");
            sendToBluetooth("✅ Step 2: Status mesin dengan charging info published");
            Serial.println("✅ Step 3: Notification charging on sent");
            sendToBluetooth("✅ Step 3: Notification charging on sent");
            Serial.println("🔄 Step 4: Ready to publish data SD Card");
            sendToBluetooth("🔄 Step 4: Ready to publish data SD Card");
            Serial.println("=============================================");
            sendToBluetooth("=============================================");
          } else {
            Serial.println("⚠️ MQTT tidak terkoneksi, status mesin akan dipublish saat online");
            sendToBluetooth("⚠️ MQTT tidak terkoneksi, status mesin akan dipublish saat online");
          }
        } else {
          Serial.println("❌ Gagal aktifkan internet, status mesin akan dipublish saat online");
        }
      } else {
        // Internet sudah aktif, cek MQTT
        if (!mqttConnected) {
          Serial.println("📡 Menghubungkan MQTT untuk publish status...");
          mqttConnected = mqttConnect();
          delay(2000);
        }
        
        if (mqttConnected) {
          // STEP 1: Update adapter status terlebih dahulu
          Serial.println("🔌 Step 1: Update adapter status...");
          sendToBluetooth("🔌 Step 1: Update adapter status...");
          updateAdapterStatus(); // Update isAdapterOk dengan benar
          delay(500); // Tunggu update selesai
          
          // STEP 2: Publish status mesin dengan charging info
          Serial.println("📡 Step 2: Publishing status mesin dengan charging info...");
          sendToBluetooth("📡 Step 2: Publishing status mesin dengan charging info...");
          
                      // Debug charging status sebelum publish
            bool mesinStatus = digitalRead(STATUS_PIN);
            bool chargingActive = isChargingActive();
            Serial.println("🔍 DEBUG: Sebelum publish - mesinStatus=" + String(mesinStatus) + ", isAdapterOk=" + String(isAdapterOk) + ", chargingActive=" + String(chargingActive));
            sendToBluetooth("🔍 DEBUG: Sebelum publish - mesinStatus=" + String(mesinStatus) + ", isAdapterOk=" + String(isAdapterOk) + ", chargingActive=" + String(chargingActive));
          
          bool publishSuccess = publishStatusMesin(timestamp, statusMesinStr);
          
          if (!publishSuccess) {
            Serial.println("⚠️ Status mesin hidup tidak terpublish, akan dipublish saat online");
          } else {
            Serial.println("✅ Status mesin dengan charging info berhasil dipublish");
            sendToBluetooth("✅ Status mesin dengan charging info berhasil dipublish");
          }
          
          // STEP 3: Kirim notification charging on (LANGSUNG - TANPA FLAG)
          Serial.println("📢 Step 3: Mengirim notification charging on...");
          sendToBluetooth("📢 Step 3: Mengirim notification charging on...");
          sendStatusNotifications(1);
          Serial.println("✅ Notification charging on berhasil dikirim");
          sendToBluetooth("✅ Notification charging on berhasil dikirim");
          
          // STEP 4: Setelah charging status dan notification selesai, baru cek data SD Card
          Serial.println("📋 Step 4: Akan mengecek data SD Card dan publish jika ada...");
          sendToBluetooth("📋 Step 4: Akan mengecek data SD Card dan publish jika ada...");
          
          // Summary logging
          Serial.println("📊 ======= STATUS MACHINE CHANGE SUMMARY =======");
          sendToBluetooth("📊 ======= STATUS MACHINE CHANGE SUMMARY =======");
          Serial.println("✅ Step 1: Adapter status updated");
          sendToBluetooth("✅ Step 1: Adapter status updated");
          Serial.println("✅ Step 2: Status mesin dengan charging info published");
          sendToBluetooth("✅ Step 2: Status mesin dengan charging info published");
          Serial.println("✅ Step 3: Notification charging on sent");
          sendToBluetooth("✅ Step 3: Notification charging on sent");
          Serial.println("🔄 Step 4: Ready to publish data SD Card");
          sendToBluetooth("🔄 Step 4: Ready to publish data SD Card");
          Serial.println("=============================================");
          sendToBluetooth("=============================================");
        } else {
          Serial.println("⚠️ MQTT tidak terkoneksi, status mesin akan dipublish saat online");
        }
      }
      
    } else {
      // Mesin mati - coba publish dulu sebelum matikan internet
      Serial.println("🔴 Mesin mati - Mencoba publish status sebelum matikan internet...");
      sendToBluetooth("🔴 Mesin mati - Mencoba publish status...");
      
      // Cek apakah internet dan MQTT masih aktif
      if (internetActive && mqttConnected) {
        Serial.println("📡 Publishing status mesin perubahan...");
        bool publishSuccess = publishStatusMesin(timestamp, statusMesinStr);
        
        if (publishSuccess) {
          Serial.println("✅ Status mesin mati berhasil dipublish");
        } else {
          Serial.println("⚠️ Status mesin mati tidak terpublish, akan dipublish saat online");
        }
        
        // Kirim notification status mesin mati (LANGSUNG - TANPA FLAG)
        Serial.println("📢 Mengirim notification status mesin mati...");
        sendToBluetooth("📢 Mengirim notification status mesin mati...");
        sendStatusNotifications(0);
        Serial.println("✅ Notification status mesin mati berhasil dikirim");
        sendToBluetooth("✅ Notification status mesin mati berhasil dikirim");
        
        // Tunggu sebentar sebelum matikan internet
        delay(1000);
      } else {
        Serial.println("⚠️ Internet/MQTT tidak aktif, mencoba aktifkan untuk notification...");
        sendToBluetooth("⚠️ Internet/MQTT tidak aktif, mencoba aktifkan untuk notification...");
        
        // Coba aktifkan internet dan MQTT untuk notification
        if (activateInternet()) {
          Serial.println("🌐 Internet berhasil diaktifkan untuk notification");
          sendToBluetooth("🌐 Internet berhasil diaktifkan untuk notification");
          
          if (mqttConnectForPublish()) {
            Serial.println("📡 MQTT berhasil terkoneksi untuk notification");
            sendToBluetooth("📡 MQTT berhasil terkoneksi untuk notification");
            
            // Kirim notification status mesin mati
            Serial.println("📢 Mengirim notification status mesin mati...");
            sendToBluetooth("📢 Mengirim notification status mesin mati...");
            sendStatusNotifications(0);
            Serial.println("✅ Notification status mesin mati berhasil dikirim");
            sendToBluetooth("✅ Notification status mesin mati berhasil dikirim");
            
            // Matikan internet setelah notification
            delay(1000);
            deactivateInternet();
            Serial.println("🔌 Internet dimatikan setelah notification");
            sendToBluetooth("🔌 Internet dimatikan setelah notification");
          } else {
            Serial.println("❌ MQTT gagal terkoneksi untuk notification");
            sendToBluetooth("❌ MQTT gagal terkoneksi untuk notification");
            deactivateInternet();
          }
        } else {
          Serial.println("❌ Internet gagal diaktifkan untuk notification");
          sendToBluetooth("❌ Internet gagal diaktifkan untuk notification");
        }
      }
      
      // Matikan internet dan mulai timer offline
      deactivateInternet();
      loadIntervalFromSD(); // <-- Tambahkan baris ini agar timer offline update dari SPIFFS
      Serial.println("⏱️ Memulai timer offline dengan interval " + String(offlineInterval/1000) + " detik...");
      startOfflineTimer();
      Serial.println("✅ Timer offline berhasil dijalankan saat mesin mati");
    }
  }
}

// Fungsi untuk memulai timer offline
void startOfflineTimer() {
  if (!offlineTimerActive) {
    offlineTimer = millis();
    offlineTimerActive = true;
    lastOfflineSave = millis();
    Serial.println("⏱️ Timer offline dimulai dengan interval " + String(offlineInterval/1000) + " detik");
    Serial.println("🔄 Timer akan publish data offline setiap " + String(offlineInterval/1000) + " detik saat mesin mati");
  } else {
    Serial.println("ℹ️ Timer offline sudah aktif");
  }
}

// Fungsi untuk menghentikan timer offline
void stopOfflineTimer() {
  if (offlineTimerActive) {
    offlineTimerActive = false;
    Serial.println("⏹️ Timer offline dihentikan");
  }
}

// Fungsi untuk mengecek apakah timer sudah expired
bool isOfflineTimerExpired() {
  if (!offlineTimerActive) {
    Serial.println("⚠️ Timer offline tidak aktif");
    return false;
  }
  
  unsigned long currentTime = millis();
  unsigned long elapsed;
  
  // Handle millis() overflow (49.7 days)
  if (currentTime >= offlineTimer) {
    elapsed = currentTime - offlineTimer;
  } else {
    elapsed = (0xFFFFFFFF - offlineTimer) + currentTime + 1;
  }
  
  // Debug informasi timer (selalu tampilkan untuk troubleshooting)
  static unsigned long lastExpiredDebug = 0;
  if (millis() - lastExpiredDebug >= 10000) { // Tampilkan setiap 10 detik
    if (timerDebugEnabled) {
    Serial.println("🔍 Debug - Timer check: elapsed=" + String(elapsed/1000) + "s, interval=" + String(offlineInterval/1000) + "s, active=" + String(offlineTimerActive ? "YES" : "NO"));
  }
    lastExpiredDebug = millis();
  }
  
  if (elapsed >= offlineInterval) {
    Serial.println("⏰ Timer offline expired setelah " + String(elapsed/1000) + " detik");
    Serial.println("🔄 Timer akan direset setelah publish");
    return true;
  }
  
  return false;
}

// Fungsi untuk reset timer offline
void resetOfflineTimer() {
  if (offlineTimerActive) {
    offlineTimer = millis();
    Serial.println("🔄 Timer offline direset");
  }
}
