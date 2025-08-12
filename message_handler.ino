// message_handler.ino
#include "configure.h"
#include <ArduinoJson.h>

// Handler global untuk membaca SerialAT per karakter dan buffering +QMTRECV
void handleSerialATMQTT() {
  static String buffer = "";
  static int openBraces = 0, closeBraces = 0;
  static bool buffering = false;

  while (SerialAT.available()) {
    char c = SerialAT.read();
    buffer += c;

    // Deteksi awal +QMTRECV
    if (!buffering && buffer.startsWith("+QMTRECV:")) {
      buffering = true;
      openBraces = 0;
      closeBraces = 0;
      if (timerDebugEnabled) {
        Serial.println("🟢 [DEBUG] Mulai buffering +QMTRECV");
      }
    }

    if (buffering) {
      if (c == '{') openBraces++;
      if (c == '}') closeBraces++;
      if (c == '[') openBraces++;
      if (c == ']') closeBraces++;
    }

    // Jika sudah newline dan buffer mengandung +QMTRECV
    if (c == '\n' && buffering) {
      if (timerDebugEnabled) {
        Serial.println("🟢 [DEBUG] Buffer akhir: " + buffer);
      }
      // Jika jumlah kurung kurawal/array seimbang, proses
      if (openBraces > 0 && openBraces == closeBraces) {
        if (timerDebugEnabled) {
          Serial.println("🟢 [DEBUG] Buffer utuh, panggil processMqttReceiveWithResult()");
        }
        processMqttReceiveWithResult(buffer);
        buffer = "";
        buffering = false;
        openBraces = 0;
        closeBraces = 0;
      } else if (buffer.length() > 2048) {
        if (timerDebugEnabled) {
          Serial.println("🟢 [DEBUG] Buffer terlalu panjang, reset");
        }
        buffer = "";
        buffering = false;
        openBraces = 0;
        closeBraces = 0;
      }
    }
    // Jika buffer terlalu panjang tanpa seimbang, reset
    if (buffer.length() > 4096) {
      if (timerDebugEnabled) {
        Serial.println("🟢 [DEBUG] Buffer overflow, reset");
      }
      buffer = "";
      buffering = false;
      openBraces = 0;
      closeBraces = 0;
    }
  }
}

// Fungsi untuk membaca pesan MQTT masuk dengan frame per baris
void checkIncomingMQTT() {
  static String buffer = "";

  while (SerialAT.available()) {
    char c = SerialAT.read();
    buffer += c;

    if (c == '\n') {
      buffer.trim();

      if (buffer.startsWith("+QMTRECV:")) {
        int firstQuote = buffer.indexOf('"');
        int secondQuote = buffer.indexOf('"', firstQuote + 1);
        int thirdQuote = buffer.indexOf('"', secondQuote + 1);
        int lastQuote = buffer.lastIndexOf('"');
        String topic = "";
        String payload = "";
        if (firstQuote >= 0 && secondQuote > firstQuote) {
          topic = buffer.substring(firstQuote + 1, secondQuote);
        }
        if (thirdQuote >= 0 && lastQuote > thirdQuote) {
          payload = buffer.substring(thirdQuote + 1, lastQuote);
        }

        // Cek apakah payload valid JSON
        StaticJsonDocument<256> testDoc;
        DeserializationError err = deserializeJson(testDoc, payload);
        if (!err) {
          handleMQTTMessage(topic, payload);
        } else {
          Serial.println("⚠️ Payload MQTT bukan JSON valid, diabaikan:");
          Serial.println(payload);
        }
      }
      buffer = "";
    }
  }
}

// Handler untuk pesan MQTT masuk
void handleMQTTMessage(String topic, String payload) {
  Serial.println("📨 MQTT Message:");
  Serial.println("Topic: " + topic);
  Serial.println("Payload: " + payload);

  if (topic == mqttIntervalTopic) {
    processIntervalMessage(payload);
  }
  // Tambahkan handler lain jika perlu
}

bool isValidJson(const String& str) {
  StaticJsonDocument<32> doc;
  return deserializeJson(doc, str) == DeserializationError::Ok;
}

// Tambahkan variabel global untuk buffering payload MQTT
String mqttPayloadBuffer = "";
int openBraces = 0;
int closeBraces = 0;

void processMqttReceive(String line) {
  if (timerDebugEnabled) {
    Serial.println("🟢 [DEBUG] Masuk processMqttReceive()");
  }
  // Ambil payload JSON dari line
  int thirdQuote = line.indexOf('"', line.indexOf('"', line.indexOf('"') + 1) + 1);
  int lastQuote = line.lastIndexOf('"');
  String message = "";
  if (thirdQuote >= 0 && lastQuote > thirdQuote) {
    message = line.substring(thirdQuote + 1, lastQuote);
  }

  // Cek dan parse JSON
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (!error && doc.containsKey("interval")) {
    int newInterval = doc["interval"];
    Serial.println("✅ INTERVAL ditemukan: " + String(newInterval));

    // Kurangi 40 detik dari interval yang masuk
    if (newInterval > 50) {
      newInterval -= 50;
      Serial.println("🔄 Interval dikurangi 50 detik: " + String(newInterval + 50) + "s -> " + String(newInterval) + "s");
    } else {
      newInterval = 1;  // Minimum 1 detik
      Serial.println("⚠️ Interval terlalu kecil, diset ke minimum 1 detik");
    }

    unsigned long newIntervalMs = newInterval * 1000;
    if (newIntervalMs != offlineInterval) {
      offlineInterval = newIntervalMs;
      Serial.println("🔄 offlineInterval diupdate ke " + String(offlineInterval / 1000) + " detik");
      if (offlineTimerActive) {
        resetOfflineTimer();
        Serial.println("🔄 Timer offline direset");
      }
    } else {
      Serial.println("ℹ️ Interval sama, tidak perlu update");
    }
  } else {
    Serial.println("❌ Tidak ditemukan key 'interval' atau JSON error");
  }
}

// Return true jika parsing berhasil, false jika gagal
bool processMqttReceiveWithResult(String line) {
  if (timerDebugEnabled) {
    Serial.println("🟢 [DEBUG] Masuk processMqttReceiveWithResult()");
  }
  // Ekstrak topic dan message
  int firstQuote = line.indexOf('"');
  int secondQuote = line.indexOf('"', firstQuote + 1);
  int thirdQuote = line.indexOf('"', secondQuote + 1);
  int lastQuote = line.lastIndexOf('"');
  String topic = "";
  String message = "";
  if (firstQuote >= 0 && secondQuote > firstQuote) {
    topic = line.substring(firstQuote + 1, secondQuote);
  }
  if (thirdQuote >= 0 && lastQuote > thirdQuote) {
    message = line.substring(thirdQuote + 1, lastQuote);
    Serial.println("🔍 Akan parsing JSON: " + message);
  }

  // Parse JSON
  StaticJsonDocument<2048> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (!error && doc.containsKey("interval")) {
    int newInterval = doc["interval"];
    Serial.println("✅ INTERVAL ditemukan: " + String(newInterval));

    // Kurangi 50 detik dari interval yang masuk
    if (newInterval > 50) {
      newInterval -= 50;
      Serial.println("🔄 Interval dikurangi 50 detik: " + String(newInterval + 50) + "s -> " + String(newInterval) + "s");
    } else {
      newInterval = 1;  // Minimum 1 detik
      Serial.println("⚠️ Interval terlalu kecil, diset ke minimum 1 detik");
    }

    unsigned long newIntervalMs = newInterval * 1000;
    if (newIntervalMs != offlineInterval) {
      offlineInterval = newIntervalMs;
      Serial.println("🔄 offlineInterval diupdate ke " + String(offlineInterval / 1000) + " detik");
      if (offlineTimerActive) {
        resetOfflineTimer();
        Serial.println("🔄 Timer offline direset");
      }
      saveIntervalToSD(newInterval);  // Simpan interval yang sudah dikurangi ke SD Card
    } else {
      Serial.println("ℹ️ Interval sama, tidak perlu update");
    }
    return true;
  } else {
    Serial.println("❌ Tidak ditemukan key 'interval' atau JSON error");
    return false;
  }
}

// Fungsi untuk mengecek retain message
void checkRetainMessage() {
  Serial.println("🔍 Checking for retain message...");

  unsigned long startTime = millis();
  String buffer = "";
  int openBraces = 0, closeBraces = 0;
  bool buffering = false;
  bool parsedSuccessfully = false;

  while (millis() - startTime < 5000 && !parsedSuccessfully) {  // 2 detik timeout
    while (SerialAT.available()) {
      String line = SerialAT.readStringUntil('\n');
      line.trim();

      if (line.length() > 0) {
        Serial.println("📡 Received: " + line);

        // Jika ada +QMTRECV, mulai buffering baru
        if (line.indexOf("+QMTRECV:") >= 0) {
          buffer = "";
          openBraces = 0;
          closeBraces = 0;
          buffering = false;

          int firstBrace = line.indexOf('{');
          if (firstBrace >= 0) {
            buffer = line.substring(line.indexOf('+QMTRECV:'), line.length());
            buffering = true;
            for (int i = 0; i < buffer.length(); i++) {
              if (buffer[i] == '{') openBraces++;
              if (buffer[i] == '}') closeBraces++;
            }
            if (openBraces > 0 && openBraces == closeBraces) {
              parsedSuccessfully = processMqttReceiveWithResult(buffer);
              buffering = false;
              if (parsedSuccessfully) break;  // keluar jika sukses
              continue;                       // lanjut ke baris berikutnya
            }
          }
        } else if (buffering) {
          buffer += line;
          for (int i = 0; i < line.length(); i++) {
            if (line[i] == '{') openBraces++;
            if (line[i] == '}') closeBraces++;
          }
          if (openBraces > 0 && openBraces == closeBraces) {
            parsedSuccessfully = processMqttReceiveWithResult(buffer);
            buffering = false;
            if (parsedSuccessfully) break;  // keluar jika sukses
          }
        }
      }
    }
    delay(10);
  }

  if (parsedSuccessfully) {
    Serial.println("✅ Retain message processed successfully");
  } else {
    Serial.println("⚠️ No retain message found or processing failed");
  }
}

// Fungsi untuk membaca MQTT messages yang masuk
void checkMqttMessages() {
  static String buffer = "";
  static unsigned long lastCheck = 0;
  
  // Cek setiap 100ms
  if (millis() - lastCheck < 100) {
    return;
  }
  lastCheck = millis();

  while (SerialAT.available()) {
    char c = SerialAT.read();
    buffer += c;

    if (c == '\n') {
      buffer.trim();
      
      if (buffer.startsWith("+QMTRECV:")) {
        Serial.println("📨 MQTT Message received:");
        Serial.println(buffer);
        
        // Parse dan handle message
        int firstQuote = buffer.indexOf('"');
        int secondQuote = buffer.indexOf('"', firstQuote + 1);
        int thirdQuote = buffer.indexOf('"', secondQuote + 1);
        int lastQuote = buffer.lastIndexOf('"');
        
        String topic = "";
        String payload = "";
        
        if (firstQuote >= 0 && secondQuote > firstQuote) {
          topic = buffer.substring(firstQuote + 1, secondQuote);
        }
        if (thirdQuote >= 0 && lastQuote > thirdQuote) {
          payload = buffer.substring(thirdQuote + 1, lastQuote);
        }
        
        Serial.println("📡 Topic: " + topic);
        Serial.println("📄 Payload: " + payload);
        
        // Handle berdasarkan topic
        if (topic == mqttIntervalTopic) {
          processIntervalMessage(payload);
        }
      }
      
      buffer = "";
    }
    
    // Prevent buffer overflow
    if (buffer.length() > 2048) {
      buffer = "";
    }
  }
} 