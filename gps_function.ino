// gps_function.ino
#include "configure.h"

void initGPS() {
  Serial.println("🛰️ Initializing GPS (EG25-G)...");
  sendToBluetooth("🛰️ Initializing GPS (EG25-G)...");

  // Check if GPS is already enabled
  if (checkGPSStatus()) {
    Serial.println("✅ GPS sudah aktif");
    sendToBluetooth("✅ GPS sudah aktif");
    gpsEnabled = true;
    return;
  }

  // Enable GPS for EG25-G dengan delay minimal
  Serial.println("🔄 Mengaktifkan GPS (EG25-G)...");
  sendToBluetooth("🔄 Mengaktifkan GPS (EG25-G)...");
  sendAT("AT+QGPS=1", 1000); // Kurangi delay dari 2000ms ke 1000ms

  // Verify GPS activation dengan timeout lebih singkat
  if (checkGPSStatus()) {
    Serial.println("✅ GPS berhasil diaktifkan");
    sendToBluetooth("✅ GPS berhasil diaktifkan");
    gpsEnabled = true;
  } else {
    Serial.println("❌ GPS gagal diaktifkan");
    sendToBluetooth("❌ GPS gagal diaktifkan");
    gpsEnabled = false;
    
    // Try to re-enable GPS after delay
    Serial.println("🔄 Mencoba mengaktifkan GPS lagi setelah delay...");
    sendToBluetooth("🔄 Mencoba mengaktifkan GPS lagi setelah delay...");
    delay(3000);
    sendAT("AT+QGPS=1", 2000);
    
    if (checkGPSStatus()) {
      Serial.println("✅ GPS berhasil diaktifkan pada percobaan kedua");
      sendToBluetooth("✅ GPS berhasil diaktifkan pada percobaan kedua");
      gpsEnabled = true;
    } else {
      Serial.println("❌ GPS tetap gagal diaktifkan");
      sendToBluetooth("❌ GPS tetap gagal diaktifkan");
    }
  }
}

bool checkGPSStatus() {
  Serial.println("🔍 Checking GPS status...");
  sendToBluetooth("🔍 Checking GPS status...");

  // Clear serial buffer
  while (SerialAT.available()) SerialAT.read();

  SerialAT.println("AT+QGPS?");
  sendToBluetooth("📡 Command: AT+QGPS?");
  unsigned long timeout = millis();
  String response = "";

  while (millis() - timeout < 2000) { // Kurangi timeout dari 3000ms ke 2000ms
    if (SerialAT.available()) {
      response += SerialAT.readString();
      if (response.indexOf("OK") >= 0) break;
    }
  }

  Serial.println("📡 GPS Status Response: " + response);
  sendToBluetooth("📡 GPS Status Response: " + response);

  // Parse response
  if (response.indexOf("+QGPS: 1") >= 0) {
    Serial.println("✅ GPS Status: ENABLED");
    sendToBluetooth("✅ GPS Status: ENABLED");
    gpsEnabled = true;
    return true;
  } else if (response.indexOf("+QGPS: 0") >= 0) {
    Serial.println("🔴 GPS Status: DISABLED");
    sendToBluetooth("🔴 GPS Status: DISABLED");
    gpsEnabled = false;
    return false;
  } else {
    Serial.println("⚠️ GPS Status: UNKNOWN");
    sendToBluetooth("⚠️ GPS Status: UNKNOWN");
    return false;
  }
}

void monitorGPSStatus() {
  unsigned long currentMillis = millis();
  unsigned long elapsedSinceLastCheck;

  // Handle millis() overflow
  if (currentMillis >= lastGPSCheck) {
    elapsedSinceLastCheck = currentMillis - lastGPSCheck;
  } else {
    elapsedSinceLastCheck = (0xFFFFFFFF - lastGPSCheck) + currentMillis + 1;
  }

  if (elapsedSinceLastCheck > gpsCheckInterval) {
    lastGPSCheck = currentMillis;

    if (!gpsEnabled) {
      Serial.println("⚠️ GPS tidak aktif, mencoba mengaktifkan...");
      initGPS();
    } else {
      // Check if GPS is still responding
      checkGPSStatus();
    }
  }
}

String getRTCTimeFromGSM() {
  SerialAT.println("AT+CCLK?");
  unsigned long timeout = millis();
  String response = "";
  while (millis() - timeout < 2000) { // Increased timeout
    if (SerialAT.available()) {
      response += SerialAT.readString();
      if (response.indexOf("OK") >= 0) break;
    }
  }
  
  // Debug response
  Serial.println("📡 GSM RTC Response: " + response);
  
  int start = response.indexOf('"');
  int end = response.indexOf('"', start + 1);
  if (start >= 0 && end > start) {
    String rawTime = response.substring(start + 1, end);
    Serial.println("📅 Raw GSM Time: " + rawTime);
    
    // Validasi format: "YY/MM/DD,HH:MM:SS+TZ"
    if (rawTime.length() < 17) {
      Serial.println("❌ GSM timestamp terlalu pendek: " + String(rawTime.length()));
      Serial.println("⚠️ Menggunakan fallback timestamp");
      return getFallbackTimestamp(); // Return fallback timestamp
    }
    
    // Parse timestamp
    int year = 2000 + rawTime.substring(0, 2).toInt();
    int month = rawTime.substring(3, 5).toInt();
    int day = rawTime.substring(6, 8).toInt();
    int hour = rawTime.substring(9, 11).toInt();
    int minute = rawTime.substring(12, 14).toInt();
    int second = rawTime.substring(15, 17).toInt();
    
    // Validasi range yang lebih longgar
    if (year < 2020 || year > 2030 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) {
      Serial.println("❌ GSM timestamp range tidak valid:");
      Serial.println("   Year: " + String(year) + " (valid: 2020-2030)");
      Serial.println("   Month: " + String(month) + " (valid: 1-12)");
      Serial.println("   Day: " + String(day) + " (valid: 1-31)");
      Serial.println("   Hour: " + String(hour) + " (valid: 0-23)");
      Serial.println("   Minute: " + String(minute) + " (valid: 0-59)");
      Serial.println("   Second: " + String(second) + " (valid: 0-59)");
      Serial.println("⚠️ Menggunakan fallback timestamp");
      return getFallbackTimestamp(); // Return fallback timestamp
    }
    
    // Format timestamp
    char buf[30];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    String formattedTime = String(buf);
    
    Serial.println("✅ GSM timestamp valid: " + formattedTime);
    return formattedTime;
  }
  
  Serial.println("❌ GSM timestamp tidak ditemukan dalam response");
  Serial.println("⚠️ Menggunakan fallback timestamp");
  return getFallbackTimestamp(); // Return fallback timestamp
}

// Function untuk memberikan fallback timestamp
String getFallbackTimestamp() {
  // Gunakan millis() untuk membuat timestamp relatif
  unsigned long currentMillis = millis();
  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  // Format: YYYY-MM-DD HH:MM:SS (dengan base time)
  char buf[30];
  snprintf(buf, sizeof(buf), "2025-01-01 %02lu:%02lu:%02lu", 
           hours % 24, minutes % 60, seconds % 60);
  
  Serial.println("🔄 Fallback timestamp: " + String(buf));
  return String(buf);
}

// Function untuk parsing timestamp GPS yang benar
String parseGPSTimestamp(String timeUTC, String date) {
  // Debug info
  sendToBluetooth("🔍 Parsing GPS timestamp:");
  sendToBluetooth("   Time UTC: '" + timeUTC + "'");
  sendToBluetooth("   Date: '" + date + "'");
  
  // Validasi input
  if (timeUTC.length() < 6 || date.length() < 6) {
    sendToBluetooth("❌ Input terlalu pendek");
    return getRTCTimeFromGSM();
  }
  
  // Parse time: "040414.0" -> hh=04, mm=04, ss=14
  String hh = timeUTC.substring(0, 2);
  String mm = timeUTC.substring(2, 4);
  String ss = timeUTC.substring(4, 6);
  
  // Parse date: "310725" -> dd=31, MM=07, yy=25
  String dd = date.substring(0, 2);
  String MM = date.substring(2, 4);
  String yy = date.substring(4, 6);
  
  // Debug parsing
  sendToBluetooth("   Parsed: hh=" + hh + ", mm=" + mm + ", ss=" + ss);
  sendToBluetooth("   Parsed: dd=" + dd + ", MM=" + MM + ", yy=" + yy);
  
  // Validasi angka
  int hhInt = hh.toInt();
  int mmInt = mm.toInt();
  int ssInt = ss.toInt();
  int ddInt = dd.toInt();
  int MMInt = MM.toInt();
  int yyInt = yy.toInt();
  
  // Validasi range
  if (hhInt < 0 || hhInt > 23 || mmInt < 0 || mmInt > 59 || ssInt < 0 || ssInt > 59 ||
      ddInt < 1 || ddInt > 31 || MMInt < 1 || MMInt > 12 || yyInt < 0 || yyInt > 99) {
    sendToBluetooth("❌ Range tidak valid, menggunakan GSM timestamp");
    return getRTCTimeFromGSM();
  }
  
  // Buat timestamp
  String timestamp = "20" + yy + "-" + MM + "-" + dd + " " + hh + ":" + mm + ":" + ss;
  sendToBluetooth("✅ Timestamp berhasil: " + timestamp);
  
  return timestamp;
}

// Function untuk validasi timestamp GPS
bool isValidGPSTimestamp(String timestamp) {
  // Cek format dasar: "YYYY-MM-DD HH:MM:SS"
  if (timestamp.length() != 19) {
    sendToBluetooth("❌ Timestamp length tidak valid: " + String(timestamp.length()));
    return false;
  }
  
  // Cek karakter separator
  if (timestamp.charAt(4) != '-' || timestamp.charAt(7) != '-' || 
      timestamp.charAt(10) != ' ' || timestamp.charAt(13) != ':' || timestamp.charAt(16) != ':') {
    sendToBluetooth("❌ Format timestamp tidak valid");
    return false;
  }
  
  // Parse dan validasi komponen
  String yearStr = timestamp.substring(0, 4);
  String monthStr = timestamp.substring(5, 7);
  String dayStr = timestamp.substring(8, 10);
  String hourStr = timestamp.substring(11, 13);
  String minuteStr = timestamp.substring(14, 16);
  String secondStr = timestamp.substring(17, 19);
  
  int year = yearStr.toInt();
  int month = monthStr.toInt();
  int day = dayStr.toInt();
  int hour = hourStr.toInt();
  int minute = minuteStr.toInt();
  int second = secondStr.toInt();
  
  // Validasi range
  if (year < 2020 || year > 2030) {
    sendToBluetooth("❌ Tahun tidak valid: " + String(year));
    return false;
  }
  if (month < 1 || month > 12) {
    sendToBluetooth("❌ Bulan tidak valid: " + String(month));
    return false;
  }
  if (day < 1 || day > 31) {
    sendToBluetooth("❌ Tanggal tidak valid: " + String(day));
    return false;
  }
  if (hour < 0 || hour > 23) {
    sendToBluetooth("❌ Jam tidak valid: " + String(hour));
    return false;
  }
  if (minute < 0 || minute > 59) {
    sendToBluetooth("❌ Menit tidak valid: " + String(minute));
    return false;
  }
  if (second < 0 || second > 59) {
    sendToBluetooth("❌ Detik tidak valid: " + String(second));
    return false;
  }
  
  // Validasi tanggal spesifik (bulan dengan 30/31 hari)
  int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
    daysInMonth[1] = 29; // Tahun kabisat
  }
  
  if (day > daysInMonth[month - 1]) {
    sendToBluetooth("❌ Tanggal tidak valid untuk bulan " + String(month) + ": " + String(day));
    return false;
  }
  
  sendToBluetooth("✅ Timestamp valid: " + timestamp);
  return true;
}

String buildGPSPayload(String timestamp, String lat, String lon) {
  if (timerDebugEnabled) {
    Serial.println("🔍 Debug - buildGPSPayload()");
  }
  Serial.println("📅 Timestamp: " + timestamp);
  Serial.println("🌐 Latitude: " + lat);
  Serial.println("🌐 Longitude: " + lon);
  
  StaticJsonDocument<256> doc;
  doc["node_id"] = node_id;
  doc["timestamp"] = timestamp;
  doc["status"] = "9";  // Real-time GPS data
  JsonObject data = doc.createNestedObject("data");
  data["latitude"] = lat;
  data["longitude"] = lon;
  
  String json;
  serializeJson(doc, json);
  
  Serial.println("📄 Generated JSON: " + json);
  Serial.println("📏 JSON Length: " + String(json.length()) + " bytes");
  
  return json;
}

// Fungsi untuk build GPS payload yang akan disimpan di SPIFFS
String buildGPSPayloadForStorage(String timestamp, String lat, String lon) {
  // Validasi timestamp sebelum menyimpan
  if (!isValidGPSTimestamp(timestamp)) {
    Serial.println("❌ Timestamp GPS tidak valid, tidak akan disimpan ke SD Card");
    sendToBluetooth("❌ Timestamp GPS tidak valid, tidak akan disimpan ke SD Card");
    return ""; // Return empty string untuk menandakan tidak valid
  }
  
  Serial.println("✅ Timestamp GPS valid, akan disimpan ke SD Card");
  sendToBluetooth("✅ Timestamp GPS valid, akan disimpan ke SD Card");
  
  StaticJsonDocument<256> doc;
  doc["node_id"] = node_id;
  doc["timestamp"] = timestamp;
  doc["status"] = "0";  // Stored GPS data
  JsonObject data = doc.createNestedObject("data");
  data["latitude"] = lat;
  data["longitude"] = lon;
  String json;
  serializeJson(doc, json);
  return json;
}

void displayGPSInfo(String lat, String lon, String timestamp, bool isRealTime) {
  Serial.println("📍 =======  GPS INFO  =======");
  Serial.println("📡 Status: " + String(isRealTime ? "REAL-TIME" : "CACHED"));
  Serial.println("🌐 Latitude: " + lat);
  Serial.println("🌐 Longitude: " + lon);
  Serial.println("⏰ Timestamp: " + timestamp);
  Serial.println("📍 =======================");
}

void displayGPSPayload(String payload) {
  Serial.println("📦 =======  GPS PAYLOAD  =======");
  Serial.println(payload);
  Serial.println("📦 =============================");
}

// Tambahkan fungsi getGPSData
bool getGPSData(String &timestamp, String &lat, String &lon, String &alt) {
  Serial.println("🎯 Mencoba mendapatkan data GPS...");
  sendToBluetooth("🎯 Mencoba mendapatkan data GPS...");
  
  // Aktifkan GPS hanya jika belum aktif
  if (!gpsEnabled) {
    Serial.println("🔄 GPS belum aktif, mengaktifkan...");
    sendToBluetooth("🔄 GPS belum aktif, mengaktifkan...");
    if (!checkGPSStatus()) {
      sendAT("AT+QGPS=1", 2000);
      delay(3000);
      gpsEnabled = checkGPSStatus();
      if (!gpsEnabled) {
        Serial.println("❌ Gagal mengaktifkan GPS");
        sendToBluetooth("❌ Gagal mengaktifkan GPS");
        return false;
      }
    }
  }
  
  Serial.println("📡 Mengirim AT+QGPSLOC=2...");
  sendToBluetooth("📡 Mengirim AT+QGPSLOC=2...");
  SerialAT.println("AT+QGPSLOC=2");
  unsigned long timeout = millis() + 3000;

  while (millis() < timeout) {
    if (SerialAT.available()) {
      String resp = SerialAT.readStringUntil('\n');
      Serial.println("<< " + resp);
      sendToBluetooth("<< " + resp);

      if (resp.startsWith("+QGPSLOC: ")) {
        resp.replace("+QGPSLOC: ", "");
        resp.trim();
        Serial.println(">> Parsed GPSLOC: " + resp);
        sendToBluetooth(">> Parsed GPSLOC: " + resp);

        // Format: hhmmss.s,lat,lon,alt,...,ddmmyy,...
        int idx1 = 0;
        String timeUTC = resp.substring(0, resp.indexOf(',', idx1));
        idx1 = resp.indexOf(',') + 1;
        lat = resp.substring(idx1, resp.indexOf(',', idx1));
        idx1 = resp.indexOf(',', idx1) + 1;
        lon = resp.substring(idx1, resp.indexOf(',', idx1));
        idx1 = resp.indexOf(',', idx1) + 1;
        alt = resp.substring(idx1, resp.indexOf(',', idx1));
        idx1 = resp.indexOf(',', idx1) + 1;

        // Parse date dengan logic yang lebih robust
        // Cari field terakhir yang berisi 6 digit (ddmmyy format)
        String date = "";
        int lastComma = resp.lastIndexOf(',');
        if (lastComma >= 0) {
          String lastField = resp.substring(lastComma + 1);
          lastField.trim();
          
          // Debug info
          sendToBluetooth("🔍 Last field: '" + lastField + "'");
          
          // Cek apakah field terakhir adalah 6 digit (ddmmyy)
          if (lastField.length() == 6 && lastField.toInt() > 0) {
            date = lastField;
            sendToBluetooth("✅ Date found: " + date);
          } else {
            // Coba cari field sebelum terakhir
            int secondLastComma = resp.lastIndexOf(',', lastComma - 1);
            if (secondLastComma >= 0) {
              String secondLastField = resp.substring(secondLastComma + 1, lastComma);
              secondLastField.trim();
              sendToBluetooth("🔍 Second last field: '" + secondLastField + "'");
              
              if (secondLastField.length() == 6 && secondLastField.toInt() > 0) {
                date = secondLastField;
                sendToBluetooth("✅ Date found: " + date);
              } else {
                // Fallback: gunakan logic lama
                sendToBluetooth("⚠️ Using fallback date parsing");
                idx1 = 0;
                for (int i = 0; i < 6; i++) idx1 = resp.indexOf(',', idx1) + 1;
                date = resp.substring(idx1, resp.indexOf(',', idx1));
              }
            } else {
              // Fallback: gunakan logic lama
              sendToBluetooth("⚠️ Using fallback date parsing");
              idx1 = 0;
              for (int i = 0; i < 6; i++) idx1 = resp.indexOf(',', idx1) + 1;
              date = resp.substring(idx1, resp.indexOf(',', idx1));
            }
          }
        } else {
          sendToBluetooth("❌ No comma found in GPS response");
          date = "";
        }

        // Gunakan function parsing yang baru
        timestamp = parseGPSTimestamp(timeUTC, date);
        
        // Tampilkan timestamp GPS data mentah dan hasil parsing di Bluetooth
        sendToBluetooth("🛰️ ======= GPS TIMESTAMP INFO (REALTIME MODE) =======");
        sendToBluetooth("📅 Time UTC (raw): " + timeUTC);
        sendToBluetooth("📅 Date (raw): " + date);
        sendToBluetooth("📅 Final Timestamp: " + timestamp);
        sendToBluetooth("🛰️ ==============================================");
        
        // Catat waktu pertama kali GPS fix
        if (!gpsFixAchieved) {
          firstGPSFixTime = millis();
          gpsFixAchieved = true;
          
          // Hitung waktu dari boot sampai GPS fix
          unsigned long elapsed;
          if (firstGPSFixTime >= bootTime) {
            elapsed = firstGPSFixTime - bootTime;
          } else {
            elapsed = (0xFFFFFFFF - bootTime) + firstGPSFixTime + 1;
          }
          bootToGPSFixTime = elapsed / 1000; // Konversi ke detik
          
          Serial.println("🎯 ======= GPS FIX ACHIEVED =======");
          Serial.println("⏰ Boot time: " + String(bootTime) + "ms");
          Serial.println("⏰ First GPS fix time: " + String(firstGPSFixTime) + "ms");
          Serial.println("⏱️ Boot to GPS fix: " + String(bootToGPSFixTime) + " seconds");
          Serial.println("🎯 ================================");
        }
        
        return true;
      }
    }
  }

  Serial.println("❌ Timeout membaca GPSLOC");
  sendToBluetooth("❌ Timeout membaca GPSLOC");
  return false;
}

// Fungsi untuk mendapatkan data GPS segera setelah boot
bool getGPSDataImmediate(String &timestamp, String &lat, String &lon, String &alt) {
  Serial.println("🎯 Mencoba mendapatkan data GPS segera...");
  sendToBluetooth("🎯 Mencoba mendapatkan data GPS segera...");
  
  // Aktifkan GPS jika belum aktif
  if (!gpsEnabled) {
    Serial.println("🔄 Mengaktifkan GPS untuk data segera...");
    sendAT("AT+QGPS=1", 500); // Delay minimal
    gpsEnabled = checkGPSStatus();
    if (!gpsEnabled) {
      Serial.println("❌ GPS gagal diaktifkan untuk data segera");
      return false;
    }
  }
  
  // Coba ambil data GPS dengan timeout minimal
  SerialAT.println("AT+QGPSLOC=2");
  unsigned long timeout = millis() + 2000; // Timeout 2 detik saja

  while (millis() < timeout) {
    if (SerialAT.available()) {
      String resp = SerialAT.readStringUntil('\n');
      Serial.println("<< " + resp);

      if (resp.startsWith("+QGPSLOC: ")) {
        resp.replace("+QGPSLOC: ", "");
        resp.trim();
        Serial.println(">> Parsed GPSLOC: " + resp);
        sendToBluetooth(">> Parsed GPSLOC: " + resp);

        // Format: hhmmss.s,lat,lon,alt,...,ddmmyy,...
        int idx1 = 0;
        String timeUTC = resp.substring(0, resp.indexOf(',', idx1));
        idx1 = resp.indexOf(',') + 1;
        lat = resp.substring(idx1, resp.indexOf(',', idx1));
        idx1 = resp.indexOf(',', idx1) + 1;
        lon = resp.substring(idx1, resp.indexOf(',', idx1));
        idx1 = resp.indexOf(',', idx1) + 1;
        alt = resp.substring(idx1, resp.indexOf(',', idx1));
        idx1 = resp.indexOf(',', idx1) + 1;

        // Parse date dengan logic yang lebih robust
        // Cari field terakhir yang berisi 6 digit (ddmmyy format)
        String date = "";
        int lastComma = resp.lastIndexOf(',');
        if (lastComma >= 0) {
          String lastField = resp.substring(lastComma + 1);
          lastField.trim();
          
          // Debug info
          sendToBluetooth("🔍 Last field: '" + lastField + "'");
          
          // Cek apakah field terakhir adalah 6 digit (ddmmyy)
          if (lastField.length() == 6 && lastField.toInt() > 0) {
            date = lastField;
            sendToBluetooth("✅ Date found: " + date);
          } else {
            // Coba cari field sebelum terakhir
            int secondLastComma = resp.lastIndexOf(',', lastComma - 1);
            if (secondLastComma >= 0) {
              String secondLastField = resp.substring(secondLastComma + 1, lastComma);
              secondLastField.trim();
              sendToBluetooth("🔍 Second last field: '" + secondLastField + "'");
              
              if (secondLastField.length() == 6 && secondLastField.toInt() > 0) {
                date = secondLastField;
                sendToBluetooth("✅ Date found: " + date);
              } else {
                // Fallback: gunakan logic lama
                sendToBluetooth("⚠️ Using fallback date parsing");
                idx1 = 0;
                for (int i = 0; i < 6; i++) idx1 = resp.indexOf(',', idx1) + 1;
                date = resp.substring(idx1, resp.indexOf(',', idx1));
              }
            } else {
              // Fallback: gunakan logic lama
              sendToBluetooth("⚠️ Using fallback date parsing");
              idx1 = 0;
              for (int i = 0; i < 6; i++) idx1 = resp.indexOf(',', idx1) + 1;
              date = resp.substring(idx1, resp.indexOf(',', idx1));
            }
          }
        } else {
          sendToBluetooth("❌ No comma found in GPS response");
          date = "";
        }

        // Gunakan function parsing yang baru
        timestamp = parseGPSTimestamp(timeUTC, date);
        
        // Tampilkan timestamp GPS data mentah dan hasil parsing di Bluetooth
        sendToBluetooth("🛰️ ======= GPS TIMESTAMP INFO (OFFLINE MODE) =======");
        sendToBluetooth("📅 Time UTC (raw): " + timeUTC);
        sendToBluetooth("📅 Date (raw): " + date);
        sendToBluetooth("📅 Final Timestamp: " + timestamp);
        sendToBluetooth("🛰️ ==============================================");
        
        // Catat waktu pertama kali GPS fix
        if (!gpsFixAchieved) {
          firstGPSFixTime = millis();
          gpsFixAchieved = true;
          
          // Hitung waktu dari boot sampai GPS fix
          unsigned long elapsed;
          if (firstGPSFixTime >= bootTime) {
            elapsed = firstGPSFixTime - bootTime;
          } else {
            elapsed = (0xFFFFFFFF - bootTime) + firstGPSFixTime + 1;
          }
          bootToGPSFixTime = elapsed / 1000; // Konversi ke detik
          
          Serial.println("🎯 ======= GPS FIX ACHIEVED (IMMEDIATE) =======");
          Serial.println("⏰ Boot time: " + String(bootTime) + "ms");
          Serial.println("⏰ First GPS fix time: " + String(firstGPSFixTime) + "ms");
          Serial.println("⏱️ Boot to GPS fix: " + String(bootToGPSFixTime) + " seconds");
          Serial.println("🎯 ===========================================");
        }
        
        Serial.println("✅ Data GPS berhasil didapat segera");
        return true;
      }
    }
  }

  Serial.println("❌ Timeout mendapatkan data GPS segera");
  sendToBluetooth("❌ Timeout mendapatkan data GPS segera");
  return false;
}

// Ubah getGPSPayload agar menggunakan getGPSData
String getGPSPayload(bool &gpsSuccess, String &timestampRef) {
  String lat, lon, alt, timestamp;
  gpsSuccess = getGPSData(timestamp, lat, lon, alt);

  // Ganti timestamp dengan timestamp GSM
  String gsmTimestamp = getRTCTimeFromGSM();
  
  // Validasi timestamp GSM
  if (gsmTimestamp == "") {
    Serial.println("❌ GSM timestamp tidak valid, skip publish GPS data");
    sendToBluetooth("❌ GSM timestamp tidak valid, skip publish GPS data");
    return ""; // Return empty string untuk skip publish
  }
  
  timestamp = gsmTimestamp;
  timestampRef = timestamp;

  if (gpsSuccess) {
    StaticJsonDocument<256> doc;
    doc["node_id"] = node_id;
    doc["timestamp"] = timestamp;
    doc["status"] = "9";
    JsonObject data = doc.createNestedObject("data");
    data["latitude"] = lat;
    data["longitude"] = lon;
    data["altitude"] = alt;
    String json;
    serializeJson(doc, json);
    return json;
  } else {
    // Jika GPS tidak fix, gunakan data terakhir dari SD Card
    Serial.println("⚠️ GPS tidak fix, mencoba menggunakan data terakhir dari SD Card...");
    String fallbackPayload = getFallbackPayload(timestampRef);
    
    if (fallbackPayload == "") {
      Serial.println("❌ Tidak ada data GPS valid untuk dipublish");
      return ""; // Return empty string untuk skip publish
    }
    
    return fallbackPayload;
  }
}

String getFallbackPayload(String &timestampRef) {
  Serial.println("🔄 Menggunakan GPS data terakhir...");

  String payload;
  if (!loadLastGPS(payload)) {
    Serial.println("❌ Tidak ada GPS data terakhir, skip publish");
    return ""; // Return empty string untuk skip publish
  }

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.println("❌ GPS data terakhir rusak, skip publish");
    return ""; // Return empty string untuk skip publish
  }

  // Validasi data GPS terakhir
  if (!doc.containsKey("data") || !doc["data"].containsKey("latitude") || !doc["data"].containsKey("longitude")) {
    Serial.println("❌ GPS data terakhir tidak lengkap, skip publish");
    return ""; // Return empty string untuk skip publish
  }

  String lat = doc["data"]["latitude"].as<String>();
  String lon = doc["data"]["longitude"].as<String>();
  
  // Validasi koordinat tidak 0.0,0.0
  if (lat == "0.0" && lon == "0.0") {
    Serial.println("❌ GPS data terakhir adalah koordinat default (0.0,0.0), skip publish");
    return ""; // Return empty string untuk skip publish
  }

  // Gunakan timestamp GSM real-time untuk publish
  String realtimeTimestamp = getRTCTimeFromGSM();
  
  // Validasi timestamp GSM
  if (realtimeTimestamp == "") {
    Serial.println("❌ GSM timestamp tidak valid, skip publish fallback data");
    sendToBluetooth("❌ GSM timestamp tidak valid, skip publish fallback data");
    return ""; // Return empty string untuk skip publish
  }
  
  timestampRef = realtimeTimestamp;

  // Buat payload baru dengan status "9" dan timestamp GSM real-time
  StaticJsonDocument<256> doc_new;
  doc_new["node_id"] = node_id;
  doc_new["timestamp"] = realtimeTimestamp;
  doc_new["status"] = "9";  // Status "9" untuk real-time publish
  JsonObject data = doc_new.createNestedObject("data");
  data["latitude"] = lat;
  data["longitude"] = lon;
  
  String json;
  serializeJson(doc_new, json);

  Serial.println("✅ Menggunakan GPS data terakhir dengan status '9' dan timestamp GSM real-time");
  Serial.println("📍 Latitude: " + lat);
  Serial.println("📍 Longitude: " + lon);
  Serial.println("⏰ Original Timestamp: " + doc["timestamp"].as<String>());
  Serial.println("⏰ Realtime Timestamp: " + realtimeTimestamp);
  Serial.println("📊 Status: 9 (Real-time publish)");
  displayGPSInfo(lat, lon, realtimeTimestamp, false);
  displayGPSPayload(json);

  return json;
}

// Function moved to bluetooth_function.ino to avoid redefinition
