// file_manager_function.ino
#include "configure.h"

void initSDCard() {
  Serial.println("🔧 ======= SD CARD INITIALIZATION =======");
  sendToBluetooth("🔧 ======= SD CARD INITIALIZATION =======");
  
  // Debug pin configuration
  Serial.println("📌 SD Card Pin Configuration:");
  sendToBluetooth("📌 SD Card Pin Configuration:");
  Serial.println("   MOSI: GPIO " + String(SD_MOSI));
  sendToBluetooth("   MOSI: GPIO " + String(SD_MOSI));
  Serial.println("   MISO: GPIO " + String(SD_MISO));
  sendToBluetooth("   MISO: GPIO " + String(SD_MISO));
  Serial.println("   SCLK: GPIO " + String(SD_SCLK));
  sendToBluetooth("   SCLK: GPIO " + String(SD_SCLK));
  Serial.println("   CS:   GPIO " + String(SD_CS));
  sendToBluetooth("   CS:   GPIO " + String(SD_CS));
  
  // Check if pins might conflict with partition scheme
  if (SD_MOSI == 23) {
    Serial.println("⚠️ WARNING: GPIO 23 (MOSI) might conflict with PSRAM!");
    sendToBluetooth("⚠️ WARNING: GPIO 23 (MOSI) might conflict with PSRAM!");
    Serial.println("💡 Solution: Try changing partition scheme to 'Default' or 'No PSRAM'");
    sendToBluetooth("💡 Solution: Try changing partition scheme to 'Default' or 'No PSRAM'");
  }
  
  if (SD_MISO == 19) {
    Serial.println("⚠️ WARNING: GPIO 19 (MISO) might conflict with UART0!");
    sendToBluetooth("⚠️ WARNING: GPIO 19 (MISO) might conflict with UART0!");
  }
  
  // Try to initialize SPI
  Serial.println("🔄 Initializing SPI for SD Card...");
  sendToBluetooth("🔄 Initializing SPI for SD Card...");
  
  SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
  delay(100); // Give some time for SPI to stabilize
  
  // Try to mount SD card with retry
  int retryCount = 0;
  bool sdMounted = false;
  
  while (retryCount < 3 && !sdMounted) {
    Serial.println("🔄 Attempt " + String(retryCount + 1) + " to mount SD Card...");
    sendToBluetooth("🔄 Attempt " + String(retryCount + 1) + " to mount SD Card...");
    
    sdMounted = SD.begin(SD_CS);
    
    if (!sdMounted) {
      Serial.println("❌ Attempt " + String(retryCount + 1) + " failed");
      sendToBluetooth("❌ Attempt " + String(retryCount + 1) + " failed");
      retryCount++;
      delay(1000); // Wait before retry
    }
  }
  
  if (!sdMounted) {
    Serial.println("❌ Gagal mount SD Card setelah " + String(retryCount) + " percobaan");
    sendToBluetooth("❌ Gagal mount SD Card setelah " + String(retryCount) + " percobaan");
    return;
  }
  
  Serial.println("✅ SD Card berhasil di-mount!");
  sendToBluetooth("✅ SD Card berhasil di-mount!");
  
  // Tampilkan informasi SD Card
  displaySDCardInfo();
}

void displaySDCardInfo() {
  Serial.println("📊 ======= SD CARD INFO =======");
  sendToBluetooth("📊 ======= SD CARD INFO =======");
  
  // Cek apakah SD Card terpasang
  if (!SD.begin(SD_CS)) {
    Serial.println("❌ SD Card tidak terpasang atau error");
    sendToBluetooth("❌ SD Card tidak terpasang atau error");
    return;
  }
  
  // Tampilkan informasi storage
  uint8_t cardType = SD.cardType();
  String cardTypeStr = "";
  
  switch (cardType) {
    case CARD_NONE:
      cardTypeStr = "Tidak ada kartu";
      break;
    case CARD_MMC:
      cardTypeStr = "MMC";
      break;
    case CARD_SD:
      cardTypeStr = "SD";
      break;
    case CARD_SDHC:
      cardTypeStr = "SDHC";
      break;
    default:
      cardTypeStr = "UNKNOWN";
      break;
  }
  
  Serial.println("📦 Card Type: " + cardTypeStr);
  sendToBluetooth("📦 Card Type: " + cardTypeStr);
  
  // Tampilkan ukuran storage
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.println("💾 Card Size: " + String(cardSize) + " MB");
  sendToBluetooth("💾 Card Size: " + String(cardSize) + " MB");
  
  // Tampilkan free space
  uint64_t freeSpace = SD.totalBytes() / (1024 * 1024);
  Serial.println("📈 Total Space: " + String(freeSpace) + " MB");
  sendToBluetooth("📈 Total Space: " + String(freeSpace) + " MB");
  
  // Cek file GPS terakhir
  if (SD.exists(lastGPSFile)) {
    File file = SD.open(lastGPSFile, FILE_READ);
    if (file) {
      Serial.println("📍 Last GPS file: Ada (" + String(file.size()) + " bytes)");
      sendToBluetooth("📍 Last GPS file: Ada (" + String(file.size()) + " bytes)");
      file.close();
    }
  } else {
    Serial.println("📍 Last GPS file: Tidak ada");
    sendToBluetooth("📍 Last GPS file: Tidak ada");
  }
  
  // Cek file offline data
  if (SD.exists(offlineDataFile)) {
    File file = SD.open(offlineDataFile, FILE_READ);
    if (file) {
      String content = file.readString();
      file.close();
      
      StaticJsonDocument<1024> doc;
      DeserializationError err = deserializeJson(doc, content);
      if (!err && doc.containsKey("offline_gps_data")) {
        JsonArray dataArray = doc["offline_gps_data"];
        Serial.println("📦 Offline GPS data: " + String(dataArray.size()) + " entries");
        sendToBluetooth("📦 Offline GPS data: " + String(dataArray.size()) + " entries");
      }
    }
  } else {
    Serial.println("📦 Offline GPS data: Tidak ada");
    sendToBluetooth("📦 Offline GPS data: Tidak ada");
  }
  
  Serial.println("===============================");
  sendToBluetooth("===============================");
}

void saveLastGPS(String fullPayload) {
  File file = SD.open(lastGPSFile, FILE_WRITE);
  if (!file) {
    Serial.println("❌ Gagal menyimpan GPS terakhir ke SD Card");
    return;
  }
  file.print(fullPayload);
  file.close();
  Serial.println("✅ GPS terakhir tersimpan di SD Card");
}

bool loadLastGPS(String &payload) {
  File file = SD.open(lastGPSFile, FILE_READ);
  if (!file || file.size() == 0) return false;
  payload = file.readString();
  file.close();
  return true;
}

// Fungsi helper untuk mengambil data GPS terakhir dan update timestamp dengan GSM realtime
String getLastGPSWithRealtimeTimestamp() {
  String lastGPSPayload;
  if (!loadLastGPS(lastGPSPayload)) {
    Serial.println("❌ Tidak ada data GPS terakhir yang tersimpan");
    sendToBluetooth("❌ Tidak ada data GPS terakhir yang tersimpan");
    return "";
  }

  // Parse data GPS terakhir
  StaticJsonDocument<512> lastDoc;
  DeserializationError err = deserializeJson(lastDoc, lastGPSPayload);
  
  if (err || !lastDoc.containsKey("data") || 
      !lastDoc["data"].containsKey("latitude") || 
      !lastDoc["data"].containsKey("longitude")) {
    Serial.println("❌ Data GPS terakhir tidak valid atau rusak");
    sendToBluetooth("❌ Data GPS terakhir tidak valid atau rusak");
    return "";
  }

  String lastLat = lastDoc["data"]["latitude"].as<String>();
  String lastLon = lastDoc["data"]["longitude"].as<String>();
  String lastAlt = lastDoc["data"].containsKey("altitude") ? lastDoc["data"]["altitude"].as<String>() : "0";
  
  // Validasi koordinat tidak 0.0,0.0
  if (lastLat == "0.0" && lastLon == "0.0") {
    Serial.println("❌ Data GPS terakhir adalah koordinat default (0.0,0.0), tidak valid");
    sendToBluetooth("❌ Data GPS terakhir adalah koordinat default (0.0,0.0), tidak valid");
    return "";
  }

  // Ambil timestamp GSM realtime
  String gsmTimestamp = getRTCTimeFromGSM();
  
  if (gsmTimestamp == "") {
    Serial.println("❌ GSM timestamp tidak valid, tidak bisa update timestamp");
    sendToBluetooth("❌ GSM timestamp tidak valid, tidak bisa update timestamp");
    return "";
  }

  // Buat payload baru dengan data terakhir dan timestamp GSM realtime
  StaticJsonDocument<256> newDoc;
  newDoc["node_id"] = node_id;
  newDoc["timestamp"] = gsmTimestamp;
  newDoc["status"] = "9"; // Real-time GPS data (dengan data terakhir)
  JsonObject data = newDoc.createNestedObject("data");
  data["latitude"] = lastLat;
  data["longitude"] = lastLon;
  data["altitude"] = lastAlt;
  String newJson;
  serializeJson(newDoc, newJson);

  // ✅ Update last GPS data dengan timestamp baru
  saveLastGPS(newJson);
  Serial.println("💾 Last GPS data diupdate dengan timestamp GSM realtime");
  sendToBluetooth("💾 Last GPS data diupdate dengan timestamp GSM realtime");
  
  Serial.println("📤 Data GPS terakhir dengan timestamp GSM realtime siap dipublish...");
  Serial.println("📍 Latitude: " + lastLat);
  Serial.println("📍 Longitude: " + lastLon);
  Serial.println("⏰ Original Timestamp: " + lastDoc["timestamp"].as<String>());
  Serial.println("⏰ New Timestamp: " + gsmTimestamp + " (GSM realtime)");
  sendToBluetooth("📍 Latitude: " + lastLat);
  sendToBluetooth("📍 Longitude: " + lastLon);
  sendToBluetooth("⏰ New Timestamp: " + gsmTimestamp + " (GSM realtime)");

  return newJson;
}

bool isNewGPS(String newLat, String newLon, String newTimestamp) {
  String lastPayload;
  if (!loadLastGPS(lastPayload)) return true;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, lastPayload);
  if (err) {
    Serial.println("❌ JSON parsing error in isNewGPS: " + String(err.c_str()));
    return true;
  }

  String lastLat = doc["data"]["latitude"];
  String lastLon = doc["data"]["longitude"];
  String lastTime = doc["timestamp"];

  return (lastLat != newLat || lastLon != newLon || lastTime != newTimestamp);
}

void saveOfflineGPSData(String gpsPayload) {
  if (gpsPayload == "") {
    Serial.println("⚠️ GPS payload kosong, tidak disimpan");
    return;
  }

  // Monitor RAM sebelum operasi file
  Serial.println("📊 ======= RAM MONITORING SAVE OFFLINE =======");
  sendToBluetooth("📊 ======= RAM MONITORING SAVE OFFLINE =======");
  Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
  sendToBluetooth("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
  Serial.println("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Used Heap: " + String(ESP.getHeapSize() - ESP.getFreeHeap()) + " bytes");
  Serial.println("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
  sendToBluetooth("💾 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
  
  // Validasi RAM sebelum operasi
  if (ESP.getFreeHeap() < 15000) { // Jika RAM < 15KB (lebih ketat)
    Serial.println("⚠️ RAM rendah sebelum save offline: " + String(ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("⚠️ RAM rendah sebelum save offline: " + String(ESP.getFreeHeap()) + " bytes");
    Serial.println("⚠️ Operasi file mungkin gagal karena kekurangan RAM");
    sendToBluetooth("⚠️ Operasi file mungkin gagal karena kekurangan RAM");
  }
  Serial.println("=============================================");
  sendToBluetooth("=============================================");

  if (gpsPayload.length() > 512) {
    Serial.println("⚠️ GPS payload terlalu besar (" + String(gpsPayload.length()) + " chars), tidak disimpan");
    return;
  }

  StaticJsonDocument<512> gpsDoc;
  DeserializationError err = deserializeJson(gpsDoc, gpsPayload);
  if (err) {
    Serial.println("❌ Gagal parse GPS payload untuk storage: " + String(err.c_str()));
    return;
  }

  String lat = gpsDoc["data"]["latitude"].as<String>();
  String lon = gpsDoc["data"]["longitude"].as<String>();
  String timestamp = gpsDoc["timestamp"].as<String>();

  // Validasi yang lebih ketat untuk mencegah duplikasi
  if (SD.exists(offlineDataFile)) {
    File checkFile = SD.open(offlineDataFile, FILE_READ);
    if (checkFile) {
      StaticJsonDocument<1024> checkDoc;
      DeserializationError checkErr = deserializeJson(checkDoc, checkFile);
      checkFile.close();
      if (!checkErr && checkDoc.containsKey("offline_gps_data")) {
        JsonArray checkArray = checkDoc["offline_gps_data"];
        
        // Cek duplikasi berdasarkan koordinat (tanpa timestamp)
        for (JsonVariant item : checkArray) {
          if (item.containsKey("data")) {
            String existingLat = item["data"]["latitude"].as<String>();
            String existingLon = item["data"]["longitude"].as<String>();
            
            // Jika koordinat sama, anggap duplikat (terlepas dari timestamp)
            if (existingLat == lat && existingLon == lon) {
              Serial.println("⚠️ GPS data dengan koordinat sama sudah ada, tidak disimpan (duplikat)");
              Serial.println("📍 Koordinat: " + lat + ", " + lon);
              return;
            }
          }
        }
        
        // Cek apakah data terakhir sama dengan yang akan disimpan
        if (checkArray.size() > 0) {
          JsonVariant lastItem = checkArray[checkArray.size() - 1];
          if (lastItem.containsKey("data")) {
            String lastLat = lastItem["data"]["latitude"].as<String>();
            String lastLon = lastItem["data"]["longitude"].as<String>();
            
            if (lastLat == lat && lastLon == lon) {
              Serial.println("⚠️ Data terakhir sama dengan yang akan disimpan, tidak disimpan (duplikat)");
              Serial.println("📍 Koordinat: " + lat + ", " + lon);
              return;
            }
          }
        }
      }
    }
  }

  String storagePayload = buildGPSPayloadForStorage(timestamp, lat, lon);
  
  // Cek apakah payload valid (tidak empty string)
  if (storagePayload == "") {
    Serial.println("❌ GPS payload tidak valid, tidak akan disimpan ke SD Card");
    sendToBluetooth("❌ GPS payload tidak valid, tidak akan disimpan ke SD Card");
    return;
  }

  StaticJsonDocument<1024> doc;
  if (SD.exists(offlineDataFile)) {
    File file = SD.open(offlineDataFile, FILE_READ);
    if (!file) {
      Serial.println("❌ Gagal membuka file offline untuk read dari SD Card");
      return;
    }
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
      Serial.println("❌ JSON parsing error: " + String(err.c_str()));
      SD.remove(offlineDataFile);
      doc.createNestedArray("offline_gps_data");
    }
  } else {
    doc.createNestedArray("offline_gps_data");
  }

  JsonArray dataArray = doc["offline_gps_data"];
  StaticJsonDocument<512> temp;
  deserializeJson(temp, storagePayload);
  dataArray.add(temp);

  File file = SD.open(offlineDataFile, FILE_WRITE);
  serializeJson(doc, file);
  file.close();
  Serial.println("💾 GPS data offline ditambahkan ke SD Card (total: " + String(dataArray.size()) + " entries)");
  Serial.println("📍 Koordinat baru: " + lat + ", " + lon);
  Serial.println("⏰ Timestamp: " + timestamp);
}

int getOfflineGPSDataCount() {
  // Monitor RAM sebelum cek jumlah data
  Serial.println("📊 ======= RAM MONITORING DATA COUNT =======");
  sendToBluetooth("📊 ======= RAM MONITORING DATA COUNT =======");
  Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  
  if (ESP.getFreeHeap() < 10000) {
    Serial.println("⚠️ RAM rendah untuk cek data count: " + String(ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("⚠️ RAM rendah untuk cek data count: " + String(ESP.getFreeHeap()) + " bytes");
  }
  Serial.println("===========================================");
  sendToBluetooth("===========================================");
  
  if (!SD.exists(offlineDataFile)) {
    return 0;
  }
  
  // Cek ukuran file terlebih dahulu
  File sizeFile = SD.open(offlineDataFile, FILE_READ);
  if (!sizeFile) {
    return 0;
  }
  
  size_t fileSize = sizeFile.size();
  sizeFile.close();
  
  Serial.println("📁 File size: " + String(fileSize) + " bytes");
  sendToBluetooth("📁 File size: " + String(fileSize) + " bytes");
  
  // Jika file terlalu besar, estimasi count tanpa load seluruh file
  if (fileSize > 30000) {
    Serial.println("⚠️ File terlalu besar, estimasi count tanpa load seluruh file");
    sendToBluetooth("⚠️ File terlalu besar, estimasi count tanpa load seluruh file");
    
    // Estimasi berdasarkan ukuran file (rata-rata ~200 bytes per entry)
    int estimatedCount = fileSize / 200;
    Serial.println("📊 Estimasi data count: ~" + String(estimatedCount) + " entries");
    sendToBluetooth("📊 Estimasi data count: ~" + String(estimatedCount) + " entries");
    
    return estimatedCount;
  }
  
  // Untuk file kecil, gunakan metode normal
  File file = SD.open(offlineDataFile, FILE_READ);
  if (!file) {
    return 0;
  }
  
  // Gunakan DynamicJsonDocument untuk file yang lebih besar
  DynamicJsonDocument doc(fileSize * 2);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  
  if (err) {
    Serial.println("❌ Gagal parse JSON untuk count: " + String(err.c_str()));
    sendToBluetooth("❌ Gagal parse JSON untuk count: " + String(err.c_str()));
    return 0;
  }
  
  if (!doc.containsKey("offline_gps_data")) {
    return 0;
  }
  
  int actualCount = doc["offline_gps_data"].size();
  Serial.println("📊 Actual data count: " + String(actualCount) + " entries");
  sendToBluetooth("📊 Actual data count: " + String(actualCount) + " entries");
  
  return actualCount;
}

// Fungsi untuk load dan publish data offline dengan chunked loading
bool publishOfflineDataChunked(int chunkSize) {
  if (!SD.exists(offlineDataFile)) {
    Serial.println("❌ File offline tidak ada di SD Card");
    sendToBluetooth("❌ File offline tidak ada di SD Card");
    return false;
  }

  // Monitor RAM sebelum operasi
  Serial.println("📊 ======= RAM MONITORING CHUNKED PUBLISH =======");
  sendToBluetooth("📊 ======= RAM MONITORING CHUNKED PUBLISH =======");
  Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  
  if (ESP.getFreeHeap() < 20000) {
    Serial.println("⚠️ RAM rendah untuk chunked publish: " + String(ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("⚠️ RAM rendah untuk chunked publish: " + String(ESP.getFreeHeap()) + " bytes");
  }
  Serial.println("===============================================");
  sendToBluetooth("===============================================");

  // Cek ukuran file
  File sizeFile = SD.open(offlineDataFile, FILE_READ);
  if (!sizeFile) {
    Serial.println("❌ Gagal membuka file untuk cek ukuran");
    sendToBluetooth("❌ Gagal membuka file untuk cek ukuran");
    return false;
  }
  
  size_t fileSize = sizeFile.size();
  sizeFile.close();
  
  Serial.println("📁 File size: " + String(fileSize) + " bytes");
  sendToBluetooth("📁 File size: " + String(fileSize) + " bytes");
  
  // Jika file terlalu besar, gunakan chunked loading dengan RAM monitoring ketat
  if (fileSize > 25000) { // Turunkan threshold dari 30KB ke 25KB
    Serial.println("⚠️ File terlalu besar (" + String(fileSize) + " bytes), menggunakan chunked loading");
    sendToBluetooth("⚠️ File terlalu besar (" + String(fileSize) + " bytes), menggunakan chunked loading");
    
    // Cek RAM sebelum mulai chunked loading
    size_t freeHeap = ESP.getFreeHeap();
    Serial.println("💾 Free RAM sebelum chunked loading: " + String(freeHeap) + " bytes");
    sendToBluetooth("💾 Free RAM sebelum chunked loading: " + String(freeHeap) + " bytes");
    
    if (freeHeap < 30000) { // Jika RAM < 30KB, gunakan chunk size 1
      Serial.println("⚠️ RAM rendah, menggunakan chunk size 1 untuk file besar");
      sendToBluetooth("⚠️ RAM rendah, menggunakan chunk size 1 untuk file besar");
      return processOfflineDataInChunks(1); // Force chunk size 1
    } else {
      return processOfflineDataInChunks(chunkSize);
    }
  } else {
    Serial.println("✅ File size normal, menggunakan normal loading");
    sendToBluetooth("✅ File size normal, menggunakan normal loading");
    return publishOfflineDataNormal();
  }
}

// Fungsi untuk process data dalam chunks
bool processOfflineDataInChunks(int chunkSize) {
  Serial.println("🔄 ======= CHUNKED PROCESSING START =======");
  sendToBluetooth("🔄 ======= CHUNKED PROCESSING START =======");
  Serial.println("📦 Chunk size: " + String(chunkSize) + " entries (optimized for low RAM)");
  sendToBluetooth("📦 Chunk size: " + String(chunkSize) + " entries (optimized for low RAM)");
  Serial.println("💾 Target RAM usage: < 5KB per chunk");
  sendToBluetooth("💾 Target RAM usage: < 5KB per chunk");
  
  int totalProcessed = 0;
  int totalPublished = 0;
  int totalFailed = 0;
  int consecutiveEmptyChunks = 0;
  const int maxEmptyChunks = 3; // Maksimal 3 chunk kosong berturut-turut
  
  while (true) {
    // Cek status mesin untuk interrupt
    bool currentMachineStatus = digitalRead(STATUS_PIN);
    if (!currentMachineStatus) {
      Serial.println("🛑 INTERRUPT: Status mesin berubah menjadi MATI");
      sendToBluetooth("🛑 INTERRUPT: Status mesin berubah menjadi MATI");
      Serial.println("🔄 Menghentikan chunked processing dan beralih ke mode offline");
      sendToBluetooth("🔄 Menghentikan chunked processing dan beralih ke mode offline");
      Serial.println("📊 Progress terhenti pada: " + String(totalProcessed) + " entries");
      sendToBluetooth("📊 Progress terhenti pada: " + String(totalProcessed) + " entries");
      break;
    }
    
    // Cek RAM sebelum setiap chunk
    size_t freeHeap = ESP.getFreeHeap();
    Serial.println("💾 Free RAM sebelum chunk: " + String(freeHeap) + " bytes");
    sendToBluetooth("💾 Free RAM sebelum chunk: " + String(freeHeap) + " bytes");
    
    if (freeHeap < 15000) { // Lebih ketat: RAM < 15KB
      Serial.println("⚠️ RAM rendah, pause chunked processing");
      sendToBluetooth("⚠️ RAM rendah, pause chunked processing");
      Serial.println("💾 Free RAM: " + String(freeHeap) + " bytes (minimum: 15000 bytes)");
      sendToBluetooth("💾 Free RAM: " + String(freeHeap) + " bytes (minimum: 15000 bytes)");
      delay(15000); // Tunggu 15 detik untuk recovery
      continue;
    }
    
    // Load chunk data
    int chunkCount = loadAndPublishChunk(chunkSize);
    
    if (chunkCount == 0) {
      consecutiveEmptyChunks++;
      Serial.println("⚠️ Chunk kosong ke-" + String(consecutiveEmptyChunks));
      sendToBluetooth("⚠️ Chunk kosong ke-" + String(consecutiveEmptyChunks));
      
      if (consecutiveEmptyChunks >= maxEmptyChunks) {
        Serial.println("✅ Semua data telah diproses (3 chunk kosong berturut-turut)");
        sendToBluetooth("✅ Semua data telah diproses (3 chunk kosong berturut-turut)");
        break;
      }
    } else {
      consecutiveEmptyChunks = 0; // Reset counter
      totalProcessed += chunkCount;
      Serial.println("📊 Progress: " + String(totalProcessed) + " entries diproses");
      sendToBluetooth("📊 Progress: " + String(totalProcessed) + " entries diproses");
    }
    
    // Jeda antar chunk untuk stabilisasi RAM
    delay(3000); // Lebih lama untuk recovery RAM
  }
  
  Serial.println("📊 ======= CHUNKED PROCESSING SUMMARY =======");
  sendToBluetooth("📊 ======= CHUNKED PROCESSING SUMMARY =======");
  Serial.println("✅ Total processed: " + String(totalProcessed) + " entries");
  sendToBluetooth("✅ Total processed: " + String(totalProcessed) + " entries");
  Serial.println("✅ Total published: " + String(totalPublished) + " entries");
  sendToBluetooth("✅ Total published: " + String(totalPublished) + " entries");
  Serial.println("❌ Total failed: " + String(totalFailed) + " entries");
  sendToBluetooth("❌ Total failed: " + String(totalFailed) + " entries");
  Serial.println("=============================================");
  sendToBluetooth("=============================================");
  
  return totalPublished > 0;
}

// Fungsi untuk load dan publish satu chunk
int loadAndPublishChunk(int chunkSize) {
  if (!SD.exists(offlineDataFile)) {
    return 0;
  }
  
  File file = SD.open(offlineDataFile, FILE_READ);
  if (!file) {
    Serial.println("❌ Gagal membuka file untuk chunk loading");
    sendToBluetooth("❌ Gagal membuka file untuk chunk loading");
    return 0;
  }
  
  // Load file dengan DynamicJsonDocument
  size_t fileSize = file.size();
  
  // Cek RAM sebelum load file
  size_t freeHeapBefore = ESP.getFreeHeap();
  Serial.println("💾 Free RAM sebelum load file: " + String(freeHeapBefore) + " bytes");
  sendToBluetooth("💾 Free RAM sebelum load file: " + String(freeHeapBefore) + " bytes");
  
  // Jika file terlalu besar dan RAM terbatas, gunakan chunk size 1
  if (fileSize > 40000 && freeHeapBefore < 40000) {
    Serial.println("⚠️ File sangat besar dan RAM terbatas, gunakan chunk size 1");
    sendToBluetooth("⚠️ File sangat besar dan RAM terbatas, gunakan chunk size 1");
    chunkSize = 1; // Force chunk size 1
  }
  
  DynamicJsonDocument doc(fileSize * 2);
  
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  
  // Cek RAM setelah load file
  size_t freeHeapAfter = ESP.getFreeHeap();
  Serial.println("💾 Free RAM setelah load file: " + String(freeHeapAfter) + " bytes");
  sendToBluetooth("💾 Free RAM setelah load file: " + String(freeHeapAfter) + " bytes");
  Serial.println("💾 RAM yang digunakan: " + String(freeHeapBefore - freeHeapAfter) + " bytes");
  sendToBluetooth("💾 RAM yang digunakan: " + String(freeHeapBefore - freeHeapAfter) + " bytes");
  
  if (freeHeapAfter < 20000) {
    Serial.println("⚠️ RAM sangat rendah setelah load file: " + String(freeHeapAfter) + " bytes");
    sendToBluetooth("⚠️ RAM sangat rendah setelah load file: " + String(freeHeapAfter) + " bytes");
  }
  
  if (err) {
    Serial.println("❌ Gagal parse JSON untuk chunk: " + String(err.c_str()));
    sendToBluetooth("❌ Gagal parse JSON untuk chunk: " + String(err.c_str()));
    return 0;
  }
  
  if (!doc.containsKey("offline_gps_data")) {
    Serial.println("❌ Tidak ada offline_gps_data dalam file");
    sendToBluetooth("❌ Tidak ada offline_gps_data dalam file");
    return 0;
  }
  
  JsonArray dataArray = doc["offline_gps_data"];
  int totalData = dataArray.size();
  
  if (totalData == 0) {
    Serial.println("✅ File kosong, tidak ada data untuk diproses");
    sendToBluetooth("✅ File kosong, tidak ada data untuk diproses");
    return 0;
  }
  
  Serial.println("📦 Processing chunk: " + String(min(chunkSize, totalData)) + " dari " + String(totalData) + " entries");
  sendToBluetooth("📦 Processing chunk: " + String(min(chunkSize, totalData)) + " dari " + String(totalData) + " entries");
  
  int published = 0;
  int failed = 0;
  
  // Process chunk data
  for (int i = 0; i < min(chunkSize, totalData); i++) {
    // Cek status mesin untuk interrupt di setiap publish
    bool currentMachineStatus = digitalRead(STATUS_PIN);
    if (!currentMachineStatus) {
      Serial.println("🛑 INTERRUPT: Status mesin MATI saat publish chunk data " + String(i + 1));
      sendToBluetooth("🛑 INTERRUPT: Status mesin MATI saat publish chunk data " + String(i + 1));
      Serial.println("🔄 Menghentikan publish dan kembali ke mode offline");
      sendToBluetooth("🔄 Menghentikan publish dan kembali ke mode offline");
      break;
    }
    
    JsonVariant item = dataArray[0]; // Selalu ambil data pertama
    String payload;
    serializeJson(item, payload);
    
    Serial.println("📤 Publishing chunk data " + String(i + 1) + "/" + String(min(chunkSize, totalData)) + "...");
    sendToBluetooth("📤 Publishing chunk data " + String(i + 1) + "/" + String(min(chunkSize, totalData)) + "...");
    
    if (mqttPublish(payload)) {
      published++;
      Serial.println("✅ Chunk data published successfully");
      sendToBluetooth("✅ Chunk data published successfully");
      
      // Hapus data yang berhasil dipublish
      dataArray.remove(0);
      
      // Update file setelah setiap publish
      File writeFile = SD.open(offlineDataFile, FILE_WRITE);
      if (writeFile) {
        serializeJson(doc, writeFile);
        writeFile.close();
        Serial.println("🗑️ Data dihapus dari SD Card, tersisa: " + String(dataArray.size()) + " entries");
        sendToBluetooth("🗑️ Data dihapus dari SD Card, tersisa: " + String(dataArray.size()) + " entries");
      }
    } else {
      failed++;
      Serial.println("❌ Chunk data publish failed");
      sendToBluetooth("❌ Chunk data publish failed");
      
      // Retry mechanism untuk publish yang gagal
      Serial.println("🔄 Mencoba retry publish data yang gagal...");
      sendToBluetooth("🔄 Mencoba retry publish data yang gagal...");
      delay(2000); // Tunggu 2 detik sebelum retry
      
      if (mqttPublish(payload)) {
        published++;
        failed--; // Kurangi failed count karena retry berhasil
        Serial.println("✅ Retry berhasil, chunk data published");
        sendToBluetooth("✅ Retry berhasil, chunk data published");
        
        // Hapus data yang berhasil dipublish
        dataArray.remove(0);
        
        // Update file setelah retry berhasil
        File writeFile = SD.open(offlineDataFile, FILE_WRITE);
        if (writeFile) {
          serializeJson(doc, writeFile);
          writeFile.close();
          Serial.println("🗑️ Data dihapus dari SD Card setelah retry, tersisa: " + String(dataArray.size()) + " entries");
          sendToBluetooth("🗑️ Data dihapus dari SD Card setelah retry, tersisa: " + String(dataArray.size()) + " entries");
        }
      } else {
        Serial.println("❌ Retry juga gagal, lanjutkan ke data berikutnya");
        sendToBluetooth("❌ Retry juga gagal, lanjutkan ke data berikutnya");
        // Jangan break, lanjutkan ke data berikutnya
      }
    }
    
    // Jeda antar publish dalam chunk
    delay(500);
  }
  
  Serial.println("📊 Chunk summary: " + String(published) + " published, " + String(failed) + " failed");
  sendToBluetooth("📊 Chunk summary: " + String(published) + " published, " + String(failed) + " failed");
  
  // Memory cleanup setelah chunk
  Serial.println("🧹 Memory cleanup setelah chunk...");
  sendToBluetooth("🧹 Memory cleanup setelah chunk...");
  
  // Force garbage collection
  ESP.getFreeHeap(); // Trigger memory cleanup
  
  size_t freeHeapAfterCleanup = ESP.getFreeHeap();
  Serial.println("💾 Free RAM setelah cleanup: " + String(freeHeapAfterCleanup) + " bytes");
  sendToBluetooth("💾 Free RAM setelah cleanup: " + String(freeHeapAfterCleanup) + " bytes");
  
  return published;
}

// Fungsi untuk normal loading (file kecil)
bool publishOfflineDataNormal() {
  // Gunakan fungsi publishOfflineData yang sudah ada
  return publishOfflineData();
}

// Fungsi untuk mengecek apakah data GPS benar-benar baru
bool isNewGPSData(String gpsPayload) {
  if (gpsPayload == "") {
    return false;
  }

  // Monitor RAM sebelum validasi GPS data
  Serial.println("📊 ======= RAM MONITORING GPS VALIDATION =======");
  sendToBluetooth("📊 ======= RAM MONITORING GPS VALIDATION =======");
  Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  
  if (ESP.getFreeHeap() < 15000) {
    Serial.println("⚠️ RAM rendah untuk validasi GPS: " + String(ESP.getFreeHeap()) + " bytes");
    sendToBluetooth("⚠️ RAM rendah untuk validasi GPS: " + String(ESP.getFreeHeap()) + " bytes");
  }
  Serial.println("===============================================");
  sendToBluetooth("===============================================");

  StaticJsonDocument<512> gpsDoc;
  DeserializationError err = deserializeJson(gpsDoc, gpsPayload);
  if (err) {
    Serial.println("❌ Gagal parse GPS payload untuk validasi");
    sendToBluetooth("❌ Gagal parse GPS payload untuk validasi");
    return false;
  }

  String lat = gpsDoc["data"]["latitude"].as<String>();
  String lon = gpsDoc["data"]["longitude"].as<String>();

  // Jika tidak ada file offline, anggap data baru
  if (!SD.exists(offlineDataFile)) {
    Serial.println("✅ Data baru (file offline tidak ada)");
    return true;
  }

  File file = SD.open(offlineDataFile, FILE_READ);
  if (!file) {
    Serial.println("✅ Data baru (gagal buka file offline)");
    return true;
  }

  StaticJsonDocument<1024> doc;
  DeserializationError fileErr = deserializeJson(doc, file);
  file.close();
  
  if (fileErr || !doc.containsKey("offline_gps_data")) {
    Serial.println("✅ Data baru (file offline kosong/rusak)");
    return true;
  }

  JsonArray dataArray = doc["offline_gps_data"];
  
  // Cek apakah koordinat sudah ada
  for (JsonVariant item : dataArray) {
    if (item.containsKey("data")) {
      String existingLat = item["data"]["latitude"].as<String>();
      String existingLon = item["data"]["longitude"].as<String>();
      
      if (existingLat == lat && existingLon == lon) {
        Serial.println("❌ Data duplikat (koordinat sama): " + lat + ", " + lon);
        return false;
      }
    }
  }

  Serial.println("✅ Data baru (koordinat berbeda): " + lat + ", " + lon);
  return true;
}
