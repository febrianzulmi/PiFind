// main_function.ino
#include "configure.h"

void setup() {
  Serial.begin(115200);
  SerialAT.begin(baud, SERIAL_8N1, 27, 26);
  
  // Build MQTT topics dynamically
  buildMqttTopics();
  
  // Catat waktu boot
  bootTime = millis();
  Serial.println("🚀 ======= SISTEM STARTUP =======");
  Serial.println("⏰ Boot time recorded: " + String(bootTime) + "ms");
  sendToBluetooth("🚀 ======= SISTEM STARTUP =======");
  sendToBluetooth("⏰ Boot time recorded: " + String(bootTime) + "ms");

  // Initialize Bluetooth (cepat)
  initBluetooth();

  // Cek status mesin terlebih dahulu untuk menentukan prioritas
  bool initialStatus = digitalRead(STATUS_PIN);
  lastStatusMesin = initialStatus;
  
  if (initialStatus) {
    // Mesin hidup - setup normal
    Serial.println("🟢 Mesin hidup saat startup - Setup normal");
    sendToBluetooth("🟢 Mesin hidup saat startup - Setup normal");
    
    // Setup charger (bisa lambat karena mesin hidup)
    setupChargerModule();
    
    // Setup storage
    initSDCard();
    delay(1000); // Kurangi delay
    
    // Load interval
    loadIntervalFromSD();
    
    // Setup GPS
    initGPS();
    delay(500); // Kurangi delay
    
    // Setup internet dan MQTT
    if (activateInternet()) {
      mqttConnected = mqttConnect();
      if (mqttConnected) {
        String timestamp = getRTCTimeFromGSM();
        
        // Validasi timestamp GSM
        if (timestamp == "") {
          Serial.println("❌ GSM timestamp tidak valid, skip publish status mesin startup");
          sendToBluetooth("❌ GSM timestamp tidak valid, skip publish status mesin startup");
        } else {
          String statusMesinStr = "1";
          Serial.println("📡 Publishing status mesin startup...");
          bool publishSuccess = publishStatusMesin(timestamp, statusMesinStr);
          
          if (publishSuccess) {
            Serial.println("✅ Status mesin startup berhasil dipublish");
            sendToBluetooth("✅ Status mesin startup berhasil dipublish");
            
            // Kirim notification charging on saat startup
            Serial.println("📢 Mengirim notification charging on saat startup...");
            sendToBluetooth("📢 Mengirim notification charging on saat startup...");
            sendStatusNotifications(1);
            Serial.println("✅ Notification charging on berhasil dikirim saat startup");
            sendToBluetooth("✅ Notification charging on berhasil dikirim saat startup");
          } else {
            Serial.println("⚠️ Status mesin startup tidak terpublish");
            sendToBluetooth("⚠️ Status mesin startup tidak terpublish");
          }
        }
      }
    }
  } else {
    // Mesin mati 
    Serial.println("🔴 Mesin mati saat startup ");
    sendToBluetooth("🔴 Mesin mati saat startup ");
    
    // Setup storage minimal
    initSDCard();
    delay(500); // Delay minimal
    
    // Load interval
    loadIntervalFromSD();
    
    // Setup GPS dengan prioritas tinggi
    Serial.println("🛰️ Setup GPS dengan prioritas tinggi...");
    sendToBluetooth("🛰️ Setup GPS dengan prioritas tinggi...");
    initGPS();
    
    // Setup charger dengan delay minimal
    Serial.println("🔋 Setup charger dengan delay minimal...");
    sendToBluetooth("🔋 Setup charger dengan delay minimal...");
    setupChargerModuleMinimal();
    
    // Mulai timer offline
    startOfflineTimer();
    
    // Kirim notification charging off saat startup (mesin mati)
    Serial.println("📢 Mengirim notification charging off saat startup...");
    sendToBluetooth("📢 Mengirim notification charging off saat startup...");
    sendStatusNotifications(0);
    Serial.println("✅ Notification charging off berhasil dikirim saat startup");
    sendToBluetooth("✅ Notification charging off berhasil dikirim saat startup");
    
    // Coba dapatkan data GPS segera
    Serial.println("🎯 Mencoba mendapatkan data GPS");
    sendToBluetooth("🎯 Mencoba mendapatkan data GPS");
    
    String lat, lon, alt, timestamp;
    bool gpsFix = getGPSDataImmediate(timestamp, lat, lon, alt);
    
    if (gpsFix) {
      // Buat payload GPS dan simpan last GPS (semua mode)
      String gpsPayload = buildGPSPayload(timestamp, lat, lon);
      
      // ✅ Simpan last GPS data (semua mode)
      saveLastGPS(gpsPayload);
      Serial.println("💾 Last GPS data tersimpan");
      sendToBluetooth("💾 Last GPS data tersimpan");
      
      if (!digitalRead(STATUS_PIN)) { // Jika mesin mati, simpan juga ke offline data
        saveOfflineGPSData(gpsPayload);
        Serial.println("✅ Data GPS berhasil didapat dan disimpan (mesin mati)");
        sendToBluetooth("✅ Data GPS berhasil didapat dan disimpan (mesin mati)");
      } else {
        Serial.println("✅ Data GPS berhasil didapat (mesin hidup, last GPS tersimpan)");
        sendToBluetooth("✅ Data GPS berhasil didapat (mesin hidup, last GPS tersimpan)");
      }
    } else {
      // Jika GPS belum fix, gunakan data terakhir atau default
      String gpsPayload = getFallbackPayload(timestamp);
      if (gpsPayload != "") {
        // ✅ Simpan last GPS data dari fallback (semua mode)
        saveLastGPS(gpsPayload);
        Serial.println("💾 Last GPS data tersimpan dari fallback");
        sendToBluetooth("💾 Last GPS data tersimpan dari fallback");
        
        if (!digitalRead(STATUS_PIN)) { // Jika mesin mati, simpan juga ke offline data
          // Tambahan validasi: hanya simpan jika data benar-benar baru
          Serial.println("⚠️ GPS belum fix, mencoba simpan fallback data (mesin mati)");
          sendToBluetooth("⚠️ GPS belum fix, mencoba simpan fallback data (mesin mati)");
          
          // Cek apakah data ini benar-benar baru atau hanya duplikat
          if (isNewGPSData(gpsPayload)) {
            saveOfflineGPSData(gpsPayload);
            Serial.println("✅ Fallback data tersimpan (data baru)");
            sendToBluetooth("✅ Fallback data tersimpan (data baru)");
          } else {
            Serial.println("⚠️ Fallback data tidak disimpan (duplikat)");
            sendToBluetooth("⚠️ Fallback data tidak disimpan (duplikat)");
          }
        } else {
          Serial.println("⚠️ GPS belum fix, last GPS tersimpan dari fallback (mesin hidup)");
          sendToBluetooth("⚠️ GPS belum fix, last GPS tersimpan dari fallback (mesin hidup)");
        }
      } else {
        Serial.println("⚠️ GPS belum fix, tidak ada data valid untuk disimpan");
        sendToBluetooth("⚠️ GPS belum fix, tidak ada data valid untuk disimpan");
      }
    }
  }

  // Display initial GPS status
  displayGPSStatusSummary();
  
  // Tampilkan status SD Card dan GPS data
  Serial.println("📊 ======= STARTUP SUMMARY =======");
  sendToBluetooth("📊 ======= STARTUP SUMMARY =======");
  
  // Tampilkan status SD Card
  displaySDCardInfo();
  
  // Tampilkan data GPS terbaru
  sendGPSDataToBT();

  Serial.println("🚀 ======= SISTEM SIAP =======");
  sendToBluetooth("🚀 ======= SISTEM SIAP =======");
}


void loop() {
  // Monitor GPS status periodically
  monitorGPSStatus();

  // Monitor MQTT connection periodically
  monitorMqttConnection();
  
  // Monitor SD Card status periodically (setiap 5 menit)
  static unsigned long lastSDCheck = 0;
  if (millis() - lastSDCheck > 300000) { // 5 menit
    lastSDCheck = millis();
    Serial.println("📊 Periodic SD Card status check...");
    displaySDCardInfo();
  }
  
  // Monitor RAM status periodically (setiap 2 menit)
  static unsigned long lastRAMCheck = 0;
  if (millis() - lastRAMCheck > 120000) { // 2 menit
    lastRAMCheck = millis();
    Serial.println("📊 ======= PERIODIC RAM MONITORING =======");
    sendToBluetooth("📊 ======= PERIODIC RAM MONITORING =======");
    Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
    sendToBluetooth("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
    Serial.println("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
    Serial.println("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
    sendToBluetooth("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
    
    // Warning jika RAM rendah
    if (ESP.getFreeHeap() < 30000) {
      Serial.println("⚠️ RAM rendah: " + String(ESP.getFreeHeap()) + " bytes");
      sendToBluetooth("⚠️ RAM rendah: " + String(ESP.getFreeHeap()) + " bytes");
      Serial.println("⚠️ MQTT connection mungkin gagal");
      sendToBluetooth("⚠️ MQTT connection mungkin gagal");
    }
    Serial.println("=========================================");
    sendToBluetooth("=========================================");
  }
  
  // Monitor charger LED status secara berkala (setiap 30 detik)
  static unsigned long lastLEDCheck = 0;
  if (millis() - lastLEDCheck > 30000) { // 30 detik
    lastLEDCheck = millis();
    updateChargerLED(); // Update LED status
  }
  
  // Monitor adapter status secara berkala (setiap 10 detik)
  static unsigned long lastAdapterCheck = 0;
  if (millis() - lastAdapterCheck > 10000) { // 10 detik
    lastAdapterCheck = millis();
    updateAdapterStatus(); // Update adapter status
  }
  
  // Monitor power supply stability
  monitorPowerSupply();


  
  // Debug charging status
  bool currentChargingStatus = isChargingActive();
  sendToBluetooth("🔍 DEBUG: isAdapterOk=" + String(isAdapterOk) + ", chargingStatus=" + String(currentChargingStatus ? "ACTIVE" : "INACTIVE"));

  // Handler parsing interval dari pesan MQTT
  parseIntervalFromSerialAdvanced();

  // Selalu cek pesan masuk dari server di setiap loop
  checkMqttMessages();

  bool statusMesin = digitalRead(STATUS_PIN);
  String statusMesinStr = statusMesin ? "1" : "0";

  // Deteksi perubahan status mesin dan handle perubahan
  if (statusMesin != lastStatusMesin) {
    handleMachineStatusChange(statusMesin, lastStatusMesin);
    lastStatusMesin = statusMesin;
  }

  if (statusMesin) {
    Serial.println("🟢 MESIN HIDUP - Mode Online (publish dengan timer interval)");
    stopOfflineTimer();
    startOnlineTimer(); // Start online timer

    if (!internetActive) {
      Serial.println("🌐 Mengaktifkan internet...");
      activateInternet();
    }
    if (!mqttConnected && internetActive) {
      Serial.println("📡 Menghubungkan MQTT...");
      mqttConnected = mqttConnect();
      if (mqttConnected) {
        Serial.println("🔍 Memastikan MQTT subscription aktif...");
        ensureMqttSubscription();
        delay(1000);  // Tunggu MQTT stabil
      }
    }

    // Publish seluruh data tersimpan di SD Card saat mesin hidup dan MQTT terkoneksi
    bool storedDataPublished = false;
    
    // DEBUG: Cek status SD Card untuk troubleshooting
    Serial.println("🔍 ======= DEBUG SD CARD STATUS =======");
    sendToBluetooth("🔍 ======= DEBUG SD CARD STATUS =======");
    
    Serial.println("📁 File exists: " + String(SD.exists(offlineDataFile) ? "YES" : "NO"));
    sendToBluetooth("📁 File exists: " + String(SD.exists(offlineDataFile) ? "YES" : "NO"));
    
    if (SD.exists(offlineDataFile)) {
      File debugFile = SD.open(offlineDataFile, FILE_READ);
      if (debugFile) {
        Serial.println("📏 File size: " + String(debugFile.size()) + " bytes");
        sendToBluetooth("📏 File size: " + String(debugFile.size()) + " bytes");
        debugFile.close();
      } else {
        Serial.println("❌ Gagal membuka file untuk debug");
        sendToBluetooth("❌ Gagal membuka file untuk debug");
      }
    }
    Serial.println("=====================================");
    sendToBluetooth("=====================================");
    
    // Pesan ini muncul setelah koneksi internet dan MQTT berhasil
    Serial.println("📋 Akan mengecek data SD Card dan publish jika ada...");
    sendToBluetooth("📋 Akan mengecek data SD Card dan publish jika ada...");
    
    // Cek ukuran file untuk informasi saja (tidak untuk skip publish)
    if (SD.exists(offlineDataFile)) {
      File sizeCheckFile = SD.open(offlineDataFile, FILE_READ);
      if (sizeCheckFile) {
        size_t fileSize = sizeCheckFile.size();
        sizeCheckFile.close();
        
        Serial.println("📁 File size: " + String(fileSize) + " bytes");
        sendToBluetooth("📁 File size: " + String(fileSize) + " bytes");
        
        if (fileSize > 30000) {
          Serial.println("⚠️ File terlalu besar, akan menggunakan chunked loading");
          sendToBluetooth("⚠️ File terlalu besar, akan menggunakan chunked loading");
          Serial.println("📊 Estimasi data: ~" + String(fileSize / 200) + " entries");
          sendToBluetooth("📊 Estimasi data: ~" + String(fileSize / 200) + " entries");
        } else {
          // Untuk file kecil, tampilkan count yang akurat
          int dataCount = getOfflineGPSDataCount();
          Serial.println("📊 Data tersimpan di SD Card: " + String(dataCount));
          sendToBluetooth("📊 Data tersimpan di SD Card: " + String(dataCount));
        }
        
        Serial.println("✅ Akan mencoba publish data SD Card...");
        sendToBluetooth("✅ Akan mencoba publish data SD Card...");
      } else {
        Serial.println("⚠️ Gagal membuka file untuk cek ukuran, tetap akan coba publish");
        sendToBluetooth("⚠️ Gagal membuka file untuk cek ukuran, tetap akan coba publish");
      }
    } else {
      Serial.println("ℹ️ File offline data tidak ada, tidak ada data untuk dipublish");
      sendToBluetooth("ℹ️ File offline data tidak ada, tidak ada data untuk dipublish");
    }
    
    if (mqttConnected && internetActive) {
      Serial.println("📤 ======= PUBLISH ALL STORED DATA (MACHINE ON) =======");
      sendToBluetooth("📤 ======= PUBLISH ALL STORED DATA (MACHINE ON) =======");
      Serial.println("✅ MQTT terkoneksi, memulai publish seluruh data tersimpan...");
      sendToBluetooth("✅ MQTT terkoneksi, memulai publish seluruh data tersimpan...");
      Serial.println("📋 Step 1: Publish data dari SD Card dengan timestamp asli");
      sendToBluetooth("📋 Step 1: Publish data dari SD Card dengan timestamp asli");
      Serial.println("📋 Step 2: Setelah selesai, baru publish data GPS real-time baru");
      sendToBluetooth("📋 Step 2: Setelah selesai, baru publish data GPS real-time baru");
      
      storedDataPublished = publishAllStoredDataWhenMachineOn();
      
      Serial.println("📤 ======= PUBLISH ALL STORED DATA COMPLETED =======");
      sendToBluetooth("📤 ======= PUBLISH ALL STORED DATA COMPLETED =======");
    } else {
      Serial.println("⚠️ MQTT/Internet tidak terkoneksi, publish data tersimpan akan ditunda...");
      sendToBluetooth("⚠️ MQTT/Internet tidak terkoneksi, publish data tersimpan akan ditunda...");
      Serial.println("🌐 Internet Active: " + String(internetActive ? "YES" : "NO"));
      sendToBluetooth("🌐 Internet Active: " + String(internetActive ? "YES" : "NO"));
      Serial.println("📡 MQTT Connected: " + String(mqttConnected ? "YES" : "NO"));
      sendToBluetooth("📡 MQTT Connected: " + String(mqttConnected ? "YES" : "NO"));
    }
    Serial.println("🔍 ================================================");
    sendToBluetooth("🔍 ================================================");
      
    // VALIDASI: Cek apakah masih ada data tersisa di SD Card (hanya jika ada data yang dipublish)
    if (storedDataPublished) {
        Serial.println("🔍 ======= VALIDASI DATA SD CARD SETELAH PUBLISH =======");
        sendToBluetooth("🔍 ======= VALIDASI DATA SD CARD SETELAH PUBLISH =======");
        
        int remainingData = getOfflineGPSDataCount();
        Serial.println("📊 Data tersisa setelah publish: " + String(remainingData));
        sendToBluetooth("📊 Data tersisa setelah publish: " + String(remainingData));
        
        if (remainingData > 0) {
          Serial.println("⚠️ PERINGATAN: Masih ada " + String(remainingData) + " data tersisa di SD Card!");
          sendToBluetooth("⚠️ PERINGATAN: Masih ada " + String(remainingData) + " data tersisa di SD Card!");
          Serial.println("📋 Akan mencoba publish ulang data tersisa...");
          sendToBluetooth("📋 Akan mencoba publish ulang data tersisa...");
          
          // Coba publish ulang data tersisa dengan retry yang lebih agresif
          int retryCount = 0;
          const int maxRetries = 3;
          bool retryPublished = false;
          
          while (remainingData > 0 && retryCount < maxRetries) {
            retryCount++;
            Serial.println("🔄 Retry ke-" + String(retryCount) + "/" + String(maxRetries) + " untuk publish data tersisa...");
            sendToBluetooth("🔄 Retry ke-" + String(retryCount) + "/" + String(maxRetries) + " untuk publish data tersisa...");
            
            retryPublished = publishAllStoredDataWhenMachineOn();
            
            if (retryPublished) {
              int newRemainingData = getOfflineGPSDataCount();
              Serial.println("✅ Retry ke-" + String(retryCount) + " berhasil, data tersisa: " + String(newRemainingData));
              sendToBluetooth("✅ Retry ke-" + String(retryCount) + " berhasil, data tersisa: " + String(newRemainingData));
              
              if (newRemainingData == 0) {
                Serial.println("🎉 SEMUA DATA BERHASIL DIPUBLISH!");
                sendToBluetooth("🎉 SEMUA DATA BERHASIL DIPUBLISH!");
                break;
              } else if (newRemainingData >= remainingData) {
                Serial.println("⚠️ Tidak ada progress, data tersisa tetap sama");
                sendToBluetooth("⚠️ Tidak ada progress, data tersisa tetap sama");
                break; // Stop jika tidak ada progress
              }
              
              remainingData = newRemainingData;
            } else {
              Serial.println("❌ Retry ke-" + String(retryCount) + " gagal");
              sendToBluetooth("❌ Retry ke-" + String(retryCount) + " gagal");
            }
            
            // Delay sebelum retry berikutnya
            if (retryCount < maxRetries && remainingData > 0) {
              Serial.println("⏱️ Menunggu 5 detik sebelum retry berikutnya...");
              sendToBluetooth("⏱️ Menunggu 5 detik sebelum retry berikutnya...");
              delay(5000);
            }
          }
          
          // Final check
          int finalRemaining = getOfflineGPSDataCount();
          if (finalRemaining == 0) {
            Serial.println("🎉 SEMUA DATA BERHASIL DIPUBLISH SETELAH RETRY!");
            sendToBluetooth("🎉 SEMUA DATA BERHASIL DIPUBLISH SETELAH RETRY!");
          } else {
            Serial.println("❌ Masih ada " + String(finalRemaining) + " data tersisa setelah " + String(maxRetries) + " retry");
            sendToBluetooth("❌ Masih ada " + String(finalRemaining) + " data tersisa setelah " + String(maxRetries) + " retry");
          }
        } else {
          Serial.println("✅ SEMUA DATA BERHASIL DIPUBLISH - SD Card kosong");
          sendToBluetooth("✅ SEMUA DATA BERHASIL DIPUBLISH - SD Card kosong");
        }
        Serial.println("🔍 ================================================");
        sendToBluetooth("🔍 ================================================");
      }
    

    // Setelah publish data tersimpan selesai, mulai publish data GPS dengan timer interval
    Serial.println("🔄 ======= MULAI PUBLISH GPS DENGAN TIMER INTERVAL =======");
    sendToBluetooth("🔄 ======= MULAI PUBLISH GPS DENGAN TIMER INTERVAL =======");
    Serial.println("📋 Status: Data tersimpan sudah dipublish, mulai GPS dengan timer interval");
    sendToBluetooth("📋 Status: Data tersimpan sudah dipublish, mulai GPS dengan timer interval");
    Serial.println("📋 IMPORTANT: Data menggunakan timestamp ASLI dari GPS (tidak di-override)");
    sendToBluetooth("📋 IMPORTANT: Data menggunakan timestamp ASLI dari GPS (tidak di-override)");
    
    // Final validation sebelum mulai GPS real-time
    Serial.println("🔍 ======= FINAL VALIDATION SEBELUM GPS REAL-TIME =======");
    sendToBluetooth("🔍 ======= FINAL VALIDATION SEBELUM GPS REAL-TIME =======");
    
    if (SD.exists(offlineDataFile)) {
      int finalDataCount = getOfflineGPSDataCount();
      Serial.println("📊 Final data count di SD Card: " + String(finalDataCount));
      sendToBluetooth("📊 Final data count di SD Card: " + String(finalDataCount));
      
      if (finalDataCount > 0) {
        Serial.println("⚠️ PERINGATAN: Masih ada " + String(finalDataCount) + " data tersisa!");
        sendToBluetooth("⚠️ PERINGATAN: Masih ada " + String(finalDataCount) + " data tersisa!");
        Serial.println("📋 Sistem akan tetap melanjutkan ke GPS real-time");
        sendToBluetooth("📋 Sistem akan tetap melanjutkan ke GPS real-time");
      } else {
        Serial.println("✅ SD Card kosong, siap untuk GPS real-time");
        sendToBluetooth("✅ SD Card kosong, siap untuk GPS real-time");
      }
    } else {
      Serial.println("✅ File offline data tidak ada, siap untuk GPS real-time");
      sendToBluetooth("✅ File offline data tidak ada, siap untuk GPS real-time");
    }
    Serial.println("=====================================================");
    sendToBluetooth("=====================================================");
    
    // Loop untuk publish data dengan timer interval selama mesin hidup
    while (digitalRead(STATUS_PIN) && mqttConnected && internetActive) {
      // Double check status mesin untuk memastikan benar-benar hidup
      bool currentMachineStatus = digitalRead(STATUS_PIN);
      if (!currentMachineStatus) {
        Serial.println("🛑 INTERRUPT: Status mesin berubah menjadi MATI saat publish GPS real-time");
        sendToBluetooth("🛑 INTERRUPT: Status mesin berubah menjadi MATI saat publish GPS real-time");
        Serial.println("🔄 Menghentikan publish GPS real-time dan beralih ke mode offline");
        sendToBluetooth("🔄 Menghentikan publish GPS real-time dan beralih ke mode offline");
        stopOnlineTimer(); // Pastikan timer online dihentikan
        
        // JANGAN reset flag notifikasi di sini karena status mesin = 0
        // Flag harus tetap true agar notifikasi tidak dikirim lagi
        Serial.println("ℹ️ Flag notifikasi offline tetap true untuk mencegah duplikat saat online timer stop");
        sendToBluetooth("ℹ️ Flag notifikasi offline tetap true untuk mencegah duplikat saat online timer stop");
        
        break;
      }
      
      // Validasi tambahan: pastikan tidak ada konflik dengan timer offline
      if (isOfflineTimerExpired()) {
        Serial.println("⚠️ KONFLIK TIMER: Timer offline expired saat status mesin = 1, skip online timer");
        sendToBluetooth("⚠️ KONFLIK TIMER: Timer offline expired saat status mesin = 1, skip online timer");
        continue; // Skip timer online, lanjut ke iterasi berikutnya
      }
      
      // Selalu cek pesan masuk untuk interval updates
      checkMqttMessages();
      
      // Pastikan subscription tetap aktif/nonstop
      if (!subscriptionActive) {
        Serial.println("🔄 Subscription tidak aktif, mencoba subscribe ulang...");
        ensureMqttSubscription();
      }

      // Cek apakah timer online sudah expired
      if (isOnlineTimerExpired()) {
        // Double check status mesin untuk memastikan benar-benar hidup
        bool currentStatusForTimer = digitalRead(STATUS_PIN);
        if (!currentStatusForTimer) {
          Serial.println("⚠️ KONFLIK TIMER: Status mesin = 0 saat online timer expired, skip publish");
          sendToBluetooth("⚠️ KONFLIK TIMER: Status mesin = 0 saat online timer expired, skip publish");
          stopOnlineTimer(); // Hentikan timer online
          
          // JANGAN reset flag notifikasi di sini karena status mesin = 0
          // Flag harus tetap true agar notifikasi tidak dikirim lagi
          Serial.println("ℹ️ Flag notifikasi offline tetap true untuk mencegah duplikat");
          sendToBluetooth("ℹ️ Flag notifikasi offline tetap true untuk mencegah duplikat");
          
          break; // Keluar dari loop
        }
        
        Serial.println("⏰ ======= ONLINE TIMER EXPIRED =======");
        sendToBluetooth("⏰ ======= ONLINE TIMER EXPIRED =======");
        Serial.println("⏰ Online timer expired, memulai publish data...");
        sendToBluetooth("⏰ Online timer expired, memulai publish data...");
        Serial.println("📋 Mode: Status Mesin = 1 (Online Timer)");
        sendToBluetooth("📋 Mode: Status Mesin = 1 (Online Timer)");
        
        // Publish data GPS dengan timer interval
        bool gpsFix = false;
        String timestamp;
        String lat, lon, alt;
      
      // Gunakan getGPSData langsung untuk data real-time, bukan getGPSPayload yang bisa fallback
      gpsFix = getGPSData(timestamp, lat, lon, alt);
      
      // GUNAKAN TIMESTAMP ASLI DARI GPS, JANGAN OVERRIDE DENGAN GSM
      // timestamp = getRTCTimeFromGSM(); // <-- DIHAPUS: Jangan override timestamp asli

      // Publish GPS hanya jika ada data GPS real-time yang valid
      if (gpsFix) {
        // Buat payload GPS real-time dengan status "9"
        StaticJsonDocument<256> doc;
        doc["node_id"] = node_id;
        doc["timestamp"] = timestamp;
        doc["status"] = "9"; // Real-time GPS data
        JsonObject data = doc.createNestedObject("data");
        data["latitude"] = lat;
        data["longitude"] = lon;
        data["altitude"] = alt;
        String json;
        serializeJson(doc, json);
        
        // ✅ Simpan last GPS data (mode online)
        saveLastGPS(json);
        Serial.println("💾 Last GPS data tersimpan (mode online)");
        
        Serial.println("📤 Publishing data GPS real-time baru...");
        Serial.println("📍 Latitude: " + lat);
        Serial.println("📍 Longitude: " + lon);
        Serial.println("⏰ Timestamp: " + timestamp + " (ASLI dari GPS)");
        
        if (!mqttPublish(json)) {
          Serial.println("❌ Publish GPS real-time gagal, cek koneksi!");
          checkMqttConnection();
          break;
        } else {
          Serial.println("✅ Data GPS real-time berhasil dipublish");
        }
      } else {
        // ⚠️ GPS tidak fix, ambil data terakhir dari last_data.json dan update timestamp dengan GSM realtime
        Serial.println("⚠️ GPS tidak fix, mengambil data terakhir dari last_data.json...");
        sendToBluetooth("⚠️ GPS tidak fix, mengambil data terakhir dari last_data.json...");
        
        String lastGPSJson = getLastGPSWithRealtimeTimestamp();
        
        if (lastGPSJson != "") {
          if (!mqttPublish(lastGPSJson)) {
            Serial.println("❌ Publish GPS data terakhir gagal, cek koneksi!");
            checkMqttConnection();
            break;
          } else {
            Serial.println("✅ Data GPS terakhir berhasil dipublish dengan timestamp GSM realtime");
            sendToBluetooth("✅ Data GPS terakhir berhasil dipublish dengan timestamp GSM realtime");
          }
        } else {
          Serial.println("❌ Tidak ada data GPS terakhir yang valid untuk dipublish");
          sendToBluetooth("❌ Tidak ada data GPS terakhir yang valid untuk dipublish");
        }
      }

      // Publish status mesin setiap siklus
      if (mqttConnected) {
        // Double check status mesin sebelum publish
        bool currentStatusForPublish = digitalRead(STATUS_PIN);
        if (!currentStatusForPublish) {
          Serial.println("⚠️ KONFLIK TIMER: Status mesin = 0 saat publish status, skip publish status mesin");
          sendToBluetooth("⚠️ KONFLIK TIMER: Status mesin = 0 saat publish status, skip publish status mesin");
          break; // Keluar dari loop
        }
        
        Serial.println("📡 Publishing status mesin...");
        // Gunakan alarm status
        int status = digitalRead(STATUS_PIN) ? 1 : 0;
        String alarmStatus = determineAlarmStatus(status, statusMesinAlarmType, statusMesinAlarmHigh, statusMesinAlarmLow);
        // Buat payload status mesin dengan alarm_status sebagai status utama
        StaticJsonDocument<256> doc;
        doc["node_id"] = node_id;
        doc["timestamp"] = timestamp;
        doc["status"] = alarmStatus;
        JsonObject data = doc.createNestedObject("data");
        data["statusMesin"] = statusMesinStr;
        String statusPayload;
        serializeJson(doc, statusPayload);
        mqttPublish(statusPayload);
      }

      // Publish status GPS sesuai hasil getGPSPayload
      String statusGPS = gpsFix ? "1" : "0";
      if (!publishStatusGPS(timestamp, statusGPS)) {
        Serial.println("❌ Publish statusGPS gagal, cek koneksi!");
        checkMqttConnection();
        break;
      }

        // Summary hasil publish online timer
        Serial.println("📤 ======= ONLINE TIMER PUBLISH SUMMARY =======");
        Serial.println("📡 Data GPS: " + String(gpsFix ? "✅ REAL-TIME" : "⚠️ LAST DATA"));
        Serial.println("⏰ Timestamp: " + timestamp);
        Serial.println("🔄 Timer: " + String(onlineInterval / 1000) + "s interval");
        Serial.println("=============================================");
        
        sendToBluetooth("📤 ======= ONLINE TIMER PUBLISH SUMMARY =======");
        sendToBluetooth("📡 Data GPS: " + String(gpsFix ? "✅ REAL-TIME" : "⚠️ LAST DATA"));
        sendToBluetooth("⏰ Timestamp: " + timestamp);
        sendToBluetooth("🔄 Timer: " + String(onlineInterval / 1000) + "s interval");
        sendToBluetooth("=============================================");
        
        // Double check status mesin sebelum reset timer
        bool currentStatusForReset = digitalRead(STATUS_PIN);
        if (!currentStatusForReset) {
          Serial.println("⚠️ KONFLIK TIMER: Status mesin = 0 saat reset timer, hentikan timer online");
          sendToBluetooth("⚠️ KONFLIK TIMER: Status mesin = 0 saat reset timer, hentikan timer online");
          stopOnlineTimer(); // Hentikan timer online
          break; // Keluar dari loop
        }
        
        // Reset timer setelah publish
        resetOnlineTimer();
        Serial.println("🔄 Online timer reset setelah publish");
        sendToBluetooth("🔄 Online timer reset setelah publish");
        
        // Cek pesan masuk lagi setelah publish
        checkMqttMessages();
        
        // Delay sebelum siklus berikutnya
        delay(1000);  // Delay 1 detik sebelum siklus berikutnya
        
        // Debug info setiap siklus
        Serial.println("🔄 Online timer cycle completed (GPS: " + String(gpsFix ? "FIX" : "NO FIX") + ", Timestamp: " + timestamp + ")");
        sendToBluetooth("🔄 Online timer cycle completed (GPS: " + String(gpsFix ? "FIX" : "NO FIX") + ", Timestamp: " + timestamp + ")");
      } else {
        // Timer belum expired, cek pesan masuk dan delay kecil
        checkMqttMessages();
        delay(1000); // Delay 1 detik untuk cek timer
        
        // Debug info setiap 5 detik
        static unsigned long lastDebugTime = 0;
        if (millis() - lastDebugTime > 5000) {
          lastDebugTime = millis();
          unsigned long elapsed = millis() - onlineTimer;
          unsigned long remainingTime = (onlineInterval - elapsed) / 1000;
          Serial.println("⏰ Online timer: " + String(remainingTime) + "s remaining (" + String(elapsed / 1000) + "s elapsed)");
          sendToBluetooth("⏰ Online timer: " + String(remainingTime) + "s remaining (" + String(elapsed / 1000) + "s elapsed)");
        }
      }
    }
    Serial.println("🔄 ======= CYCLE END (ONLINE) =======");
    Serial.println("⚠️ Mesin masih hidup, akan kembali ke loop utama untuk monitoring...");
    // HAPUS return statement agar loop utama tetap berjalan
    // return;  // <-- DIHAPUS: Agar sistem tetap di loop utama
  } else {
    // Mesin mati, stop online timer
    if (onlineTimerActive) {
      stopOnlineTimer();
    }
  }

  // Mode offline (MESIN MATI) - tetap pakai timer interval
  unsigned long currentMillis = millis();
  unsigned long elapsedSinceLastSend;
  if (currentMillis >= lastSend) {
    elapsedSinceLastSend = currentMillis - lastSend;
  } else {
    elapsedSinceLastSend = (0xFFFFFFFF - lastSend) + currentMillis + 1;
  }
  
  if (elapsedSinceLastSend > sendInterval) {
    // Double check status mesin untuk memastikan benar-benar mati
    bool currentStatusMesin = digitalRead(STATUS_PIN);
    if (currentStatusMesin) {
      Serial.println("⚠️ Status mesin berubah menjadi HIDUP saat offline cycle, skip offline processing");
      sendToBluetooth("⚠️ Status mesin berubah menjadi HIDUP saat offline cycle, skip offline processing");
      return;
    }
    
    lastSend = currentMillis;
    Serial.println("\n🔄 ======= CYCLE START (OFFLINE) =======");
    Serial.println("🔴 MESIN MATI - Mode Offline");
    
    // Coba dapatkan data GPS terbaru dengan prioritas tinggi
    Serial.println("🎯 Mencoba mendapatkan data GPS terbaru...");
    sendToBluetooth("🎯 Mencoba mendapatkan data GPS terbaru...");
    String lat, lon, alt, timestamp;
    bool gpsFix = getGPSDataImmediate(timestamp, lat, lon, alt);
    
    if (gpsFix) {
      // Tampilkan hasil GPS di Bluetooth
      sendToBluetooth("✅ GPS Fix berhasil di mode offline");
      sendToBluetooth("📍 Latitude: " + lat);
      sendToBluetooth("📍 Longitude: " + lon);
      sendToBluetooth("⏰ Timestamp: " + timestamp);
      
      // Buat payload GPS dengan status "9" (real-time) dan publish langsung
      String gpsPayload = buildGPSPayload(timestamp, lat, lon);
      
      // ✅ Simpan last GPS data (mode offline)
      saveLastGPS(gpsPayload);
      Serial.println("💾 Last GPS data tersimpan (mode offline)");
      
      // Publish data GPS terbaru langsung dengan status "9" jika MQTT terkoneksi
      if (mqttConnected) {
        Serial.println("📤 Publishing data GPS terbaru dengan status '9' (real-time)...");
        if (mqttPublish(gpsPayload)) {
          Serial.println("✅ Data GPS terbaru berhasil dipublish dengan status '9'");
        } else {
          Serial.println("❌ Gagal publish data GPS terbaru");
        }
      }
      
      // Simpan data GPS untuk backup (dengan status "0")
      saveOfflineGPSData(gpsPayload);
      Serial.println("✅ Data GPS terbaru berhasil didapat, dipublish, dan disimpan");
      Serial.println("⏰ Sumber: Timer sendInterval (" + String(sendInterval/1000) + " detik)");
      sendToBluetooth("⏰ Sumber: Timer sendInterval (" + String(sendInterval/1000) + " detik)");
    } else {
      // Tampilkan status GPS tidak fix di Bluetooth
      sendToBluetooth("❌ GPS tidak fix di mode offline");
      sendToBluetooth("⚠️ Mencoba menggunakan data terakhir dari SD Card");
      
      // Jika GPS belum fix, gunakan data terakhir dari SD Card dengan status "9"
      Serial.println("⚠️ GPS belum fix, mencoba menggunakan data terakhir dari SD Card...");
      String fallbackTimestamp;
      String gpsPayload = getFallbackPayload(fallbackTimestamp);
      
      if (gpsPayload != "") {
        // ✅ Simpan last GPS data dari fallback (mode offline)
        saveLastGPS(gpsPayload);
        Serial.println("💾 Last GPS data tersimpan dari fallback (mode offline)");
        
        // Publish data fallback dengan status "9" jika MQTT terkoneksi
        if (mqttConnected) {
          Serial.println("📤 Publishing data GPS terakhir dari SD Card dengan status '9'...");
          if (mqttPublish(gpsPayload)) {
            Serial.println("✅ Data GPS terakhir berhasil dipublish dengan status '9'");
          } else {
            Serial.println("❌ Gagal publish data GPS terakhir");
          }
        }
        
        // Update timestamp untuk status mesin dan statusGPS
        timestamp = fallbackTimestamp;
        Serial.println("⚠️ GPS belum fix, menggunakan data terakhir dari SD Card");
      } else {
        Serial.println("⚠️ GPS belum fix dan tidak ada data terakhir yang valid");
      }
    }
    
    // Publish status mesin mati dengan informasi charging lengkap jika MQTT masih terkoneksi
    if (mqttConnected) {
      String statusMesinStr = "0";
      Serial.println("📡 Publishing status mesin mati dengan charging info...");
      publishStatusMesin(timestamp, statusMesinStr);
    }
    
    // Publish statusGPS sesuai hasil getGPSDataImmediate jika MQTT masih terkoneksi
    if (mqttConnected) {
      String statusGPS = gpsFix ? "1" : "0";
      Serial.println("📡 Publishing statusGPS (offline mode)...");
      publishStatusGPS(timestamp, statusGPS);
    }
    
    // Cek apakah timer sudah expired
    Serial.println("🔄 ======= CYCLE END (OFFLINE) =======\n");
    
    // Validasi status mesin setelah cycle end
    bool statusAfterCycle = digitalRead(STATUS_PIN);
    if (statusAfterCycle) {
      Serial.println("⚠️ Status mesin berubah menjadi HIDUP setelah cycle end, akan diproses di loop berikutnya");
      sendToBluetooth("⚠️ Status mesin berubah menjadi HIDUP setelah cycle end, akan diproses di loop berikutnya");
    }
  }
  
  // Cek timer offline di setiap loop (lebih presisi)
  if (!statusMesin && isOfflineTimerExpired()) {
    // Double check status mesin untuk memastikan benar-benar mati
    bool currentStatusMesin = digitalRead(STATUS_PIN);
    if (currentStatusMesin) {
      Serial.println("⚠️ Status mesin berubah menjadi HIDUP saat timer expired, skip offline publish");
      sendToBluetooth("⚠️ Status mesin berubah menjadi HIDUP saat timer expired, skip offline publish");
      resetOfflineTimer();
      return;
    }
    
    Serial.println("⏰ Timer offline expired, memulai publish data tersimpan...");
    sendToBluetooth("⏰ Timer offline expired, memulai publish data tersimpan...");
    sendToBluetooth("🔍 DEBUG: NOTIFICATION SYSTEM - Direct send on status change");
    sendToBluetooth("🔍 DEBUG: NOTIFICATION SYSTEM - Notification akan dikirim saat timer expired");
    
    // Aktifkan internet sementara untuk publish
    if (activateInternet()) {
      Serial.println("🌐 Internet aktif sementara untuk publish data tersimpan");
      
      // Connect MQTT sementara
      bool originalMqttState = mqttConnected;
      if (mqttConnectForPublish()) {
        Serial.println("📡 MQTT terkoneksi sementara untuk publish data tersimpan");
        
        // Subscribe ke topic interval untuk menerima update interval
        Serial.println("🔔 Subscribe ke topic interval untuk update timer...");
        sendToBluetooth("🔔 Subscribe ke topic interval untuk update timer...");
        if (mqttSubscribe(mqttIntervalTopic)) {
          Serial.println("✅ Subscribe interval berhasil, cek retain message...");
          sendToBluetooth("✅ Subscribe interval berhasil, cek retain message...");
          checkMqttMessages(); // Cek apakah ada retain message
        } else {
          Serial.println("❌ Subscribe interval gagal");
          sendToBluetooth("❌ Subscribe interval gagal");
        }
        
        // Publish data tersimpan
        bool publishSuccess = false;
        bool statusMesinPublished = false;
        bool statusGPSPublished = false;
        String lat, lon, alt, timestamp;
        bool gpsFix = getGPSDataImmediate(timestamp, lat, lon, alt);
        
        if (gpsFix) {
          String gpsPayload = buildGPSPayload(timestamp, lat, lon);
          publishSuccess = mqttPublish(gpsPayload);
          
          if (publishSuccess) {
            Serial.println("✅ Berhasil publish data GPS terbaru dengan status '9'");
          } else {
            Serial.println("❌ Gagal publish data GPS terbaru");
          }
        } else {
          // Gunakan data terakhir dari SD Card
          String fallbackTimestamp;
          String gpsPayload = getFallbackPayload(fallbackTimestamp);
          
          if (gpsPayload != "") {
            publishSuccess = mqttPublish(gpsPayload);
            
            if (publishSuccess) {
              Serial.println("✅ Berhasil publish data GPS terakhir dengan status '9'");
            } else {
              Serial.println("❌ Gagal publish data GPS terakhir");
            }
          } else {
            Serial.println("❌ Tidak ada data GPS valid untuk dipublish");
          }
        }
        
        // Publish status mesin mati
        Serial.println("📡 Publishing status mesin mati saat timer expired...");
        String statusMesinStr = "0";
        statusMesinPublished = publishStatusMesin(timestamp, statusMesinStr);
        if (statusMesinPublished) {
          Serial.println("✅ Status mesin mati berhasil dipublish saat timer expired");
        } else {
          Serial.println("❌ Status mesin mati gagal dipublish saat timer expired");
        }
        
        // Publish status GPS
        Serial.println("📡 Publishing status GPS saat timer expired...");
        String statusGPS = gpsFix ? "1" : "0";
        statusGPSPublished = publishStatusGPS(timestamp, statusGPS);
        if (statusGPSPublished) {
          Serial.println("✅ Status GPS berhasil dipublish saat timer expired");
        } else {
          Serial.println("❌ Status GPS gagal dipublish saat timer expired");
        }
        
        // Kirim notification saat timer expired untuk memastikan notification terkirim
        Serial.println("📢 Mengirim notification saat timer expired...");
        sendToBluetooth("📢 Mengirim notification saat timer expired...");
        sendStatusNotifications(0);
        Serial.println("✅ Notification timer expired berhasil dikirim");
        sendToBluetooth("✅ Notification timer expired berhasil dikirim");
        // }
        
        // Disconnect MQTT dan matikan internet
        mqttConnected = originalMqttState;
        sendAT("AT+QMTDISC=0", 2000);
        deactivateInternet();
        
        // Summary hasil publish timer expired
        Serial.println("📤 ======= TIMER EXPIRED PUBLISH SUMMARY =======");
        Serial.println("📡 Data GPS: " + String(publishSuccess ? "✅ SUCCESS" : "❌ FAILED"));
        Serial.println("⚙️ Status Mesin: " + String(statusMesinPublished ? "✅ SUCCESS" : "❌ FAILED"));
        Serial.println("🛰️ Status GPS: " + String(statusGPSPublished ? "✅ SUCCESS" : "❌ FAILED"));
        Serial.println("📢 Notification: ✅ SENT ON STATUS CHANGE");
        Serial.println("=============================================");
        
        if (publishSuccess || statusMesinPublished || statusGPSPublished) {
          Serial.println("✅ Publish timer expired berhasil");
          sendToBluetooth("✅ Publish timer expired berhasil");
        } else {
          Serial.println("❌ Publish timer expired gagal");
          sendToBluetooth("❌ Publish timer expired gagal");
        }
      } else {
        Serial.println("❌ Gagal koneksi MQTT sementara");
        sendToBluetooth("❌ Gagal koneksi MQTT sementara");
        deactivateInternet();
      }
    } else {
      Serial.println("❌ Gagal aktifkan internet sementara");
      sendToBluetooth("❌ Gagal aktifkan internet sementara");
    }
    
    // Reset timer offline
    resetOfflineTimer();
  } else if (!statusMesin) {
    // Mesin mati tapi timer belum expired, cek interval
    static unsigned long lastOfflineCheck = 0;
    if (millis() - lastOfflineCheck > 30000) { // Cek setiap 30 detik
      lastOfflineCheck = millis();
      Serial.println("⏰ Timer offline: " + String((offlineInterval - (millis() - offlineTimer)) / 1000) + " detik tersisa");
      sendToBluetooth("⏰ Timer offline: " + String((offlineInterval - (millis() - offlineTimer)) / 1000) + " detik tersisa");
    }
  }
  
  delay(10);
}