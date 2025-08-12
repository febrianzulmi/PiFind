// interval_manager.ino
#include "configure.h"

// Fungsi untuk menyimpan interval ke SD Card
void saveIntervalToSD(int interval) {
  File file = SD.open("/current_interval.txt", FILE_WRITE);
  if (file) {
    file.println(interval);
    file.close();
    Serial.println("💾 Interval " + String(interval) + " saved to SD Card");
  } else {
    Serial.println("❌ Failed to save interval to SD Card");
  }
}

// Fungsi untuk memuat interval dari SD Card saat boot
void loadIntervalFromSD() {
  if (SD.exists("/current_interval.txt")) {
    File file = SD.open("/current_interval.txt", FILE_READ);
    if (file) {
      String intervalStr = file.readStringUntil('\n');
      intervalStr.trim();
      int savedInterval = intervalStr.toInt();
      file.close();
      if (savedInterval > 0 && savedInterval <= 86400) {
        offlineInterval = savedInterval * 1000UL;
        onlineInterval = savedInterval * 1000UL; // Update online interval juga
        Serial.println("📂 Loaded interval from SD Card: " + String(savedInterval) + " seconds");
        Serial.println("📂 Online interval also updated: " + String(savedInterval) + " seconds");
      } else {
        Serial.println("⚠️ Invalid interval in SD Card: " + String(savedInterval) + ", using default");
        offlineInterval = 10000;  // Fallback 30 detik
        onlineInterval = 10000;   // Fallback 30 detik
      }
    }
  } else {
    Serial.println("📂 No saved interval found, using default");
  }
}

// Fungsi parsing interval dari pesan +QMTRECV
void parseIntervalFromSerialAdvanced() {
  static String serialBuffer = "";
  static int lastParsedInterval = -1;
  while (SerialAT.available()) {
    char c = SerialAT.read();
    serialBuffer += c;
    if (c == '\n' || serialBuffer.length() > 2048) {
      serialBuffer.trim();
      if (serialBuffer.startsWith("+QMTRECV:") && serialBuffer.indexOf("\"interval\":") > 0) {
        Serial.println("📡 ======= INTERVAL MQTT MESSAGE =======");
        Serial.println("📄 Raw: " + serialBuffer);
        int intervalPos = serialBuffer.indexOf("\"interval\":");
        if (intervalPos > 0) {
          String afterInterval = serialBuffer.substring(intervalPos + 11);
          afterInterval.trim();
          String intervalValue = "";
          bool foundDigit = false;
          for (int i = 0; i < afterInterval.length(); i++) {
            char ch = afterInterval[i];
            if (isDigit(ch)) {
              intervalValue += ch;
              foundDigit = true;
            } else if (foundDigit) {
              break;
            }
          }
          if (intervalValue.length() > 0) {
            int newInterval = intervalValue.toInt();
            Serial.println("✅ Interval extracted: " + String(newInterval));
            if (newInterval == lastParsedInterval) {
              Serial.println("ℹ️ Interval sama dengan yang terakhir diproses, diabaikan (duplicate)");
            } else if (newInterval > 0 && newInterval <= 86400) {
              // Kurangi 40 detik dari interval yang masuk
              int originalInterval = newInterval;
              if (newInterval > 50) {
                newInterval -= 50;
                Serial.println("🔄 Interval dikurangi 50 detik: " + String(originalInterval) + "s -> " + String(newInterval) + "s");
              } else {
                newInterval = 1;  // Minimum 1 detik
                Serial.println("⚠️ Interval terlalu kecil, diset ke minimum 1 detik");
              }

              unsigned long newIntervalMs = newInterval * 1000UL;
              if (newIntervalMs != offlineInterval) {
                unsigned long oldInterval = offlineInterval / 1000;
                offlineInterval = newIntervalMs;
                Serial.println("🔄 Interval updated: " + String(oldInterval) + "s -> " + String(newInterval) + "s");
                if (offlineTimerActive) {
                  resetOfflineTimer();
                  Serial.println("🔄 Offline timer reset with new interval");
                }
                saveIntervalToSD(newInterval);  // Simpan interval yang sudah dikurangi ke SD Card
                Serial.println("💾 Interval saved to SD Card");
              } else {
                Serial.println("ℹ️ Interval unchanged (" + String(newInterval) + "s)");
              }
              lastParsedInterval = originalInterval;  // Simpan original interval untuk duplicate check
            } else {
              Serial.println("❌ Invalid interval range: " + String(newInterval));
            }
          } else {
            Serial.println("❌ No valid interval number found");
          }
        }
        serialBuffer = "";
      } else if (serialBuffer.length() > 2048) {
        serialBuffer = "";
      }
    }
  }
}

// ======= ONLINE TIMER FUNCTIONS =======

void startOnlineTimer() {
  onlineTimer = millis();
  onlineTimerActive = true;
  Serial.println("⏰ Online timer started");
  sendToBluetooth("⏰ Online timer started");
  Serial.println("⏰ Online timer interval: " + String(onlineInterval / 1000) + " seconds");
  sendToBluetooth("⏰ Online timer interval: " + String(onlineInterval / 1000) + " seconds");
}

void stopOnlineTimer() {
  onlineTimerActive = false;
  Serial.println("⏰ Online timer stopped");
  sendToBluetooth("⏰ Online timer stopped");
}

void resetOnlineTimer() {
  onlineTimer = millis();
  Serial.println("🔄 Online timer reset");
  sendToBluetooth("🔄 Online timer reset");
  Serial.println("⏰ Next online timer: " + String(onlineInterval / 1000) + " seconds");
  sendToBluetooth("⏰ Next online timer: " + String(onlineInterval / 1000) + " seconds");
}

bool isOnlineTimerExpired() {
  if (!onlineTimerActive) {
    return false;
  }
  
  unsigned long currentMillis = millis();
  unsigned long elapsed;
  
  // Handle millis() overflow
  if (currentMillis >= onlineTimer) {
    elapsed = currentMillis - onlineTimer;
  } else {
    elapsed = (0xFFFFFFFF - onlineTimer) + currentMillis + 1;
  }
  
  bool expired = elapsed >= onlineInterval;
  
  // Log timer status setiap 5 detik
  static unsigned long lastTimerLog = 0;
  if (millis() - lastTimerLog > 5000) {
    lastTimerLog = millis();
    unsigned long remainingTime = (onlineInterval - elapsed) / 1000;
    Serial.println("⏰ Online timer status: " + String(remainingTime) + "s remaining (" + String(elapsed / 1000) + "s elapsed)");
    sendToBluetooth("⏰ Online timer status: " + String(remainingTime) + "s remaining (" + String(elapsed / 1000) + "s elapsed)");
  }
  
  return expired;
} 