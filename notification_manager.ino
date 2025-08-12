// notification_manager.ino
#include "configure.h"
#include <ArduinoJson.h>

// Function untuk publish notification dengan topic khusus
bool mqttPublishNotification(String payload) {
  if (!mqttConnected) {
    Serial.println("❌ MQTT tidak terkoneksi untuk notification");
    return false;
  }

  if (payload.length() == 0) {
    Serial.println("❌ Payload notification kosong");
    return false;
  }

  // Validasi payload tidak null atau kosong
  if (payload == "null" || payload == "" || payload.length() < 10) {
    Serial.println("❌ Payload notification tidak valid: " + payload);
    Serial.println("📏 Payload length: " + String(payload.length()));
    return false;
  }

  // Debug: tampilkan payload notification yang akan dikirim (hanya jika debug enabled)
  if (timerDebugEnabled) {
    Serial.println("🔍 Debug - Notification payload yang akan dikirim:");
    Serial.println("📄 Content: " + payload);
    Serial.println("📏 Length: " + String(payload.length()) + " bytes");
    Serial.println("📡 Topic: " + String(mqtt_pub_notif));
  }

  int len = payload.length();
  Serial.println("🔄 Memulai publish notification (Length: " + String(len) + ")...");

  SerialAT.println("AT+QMTPUBEX=0,0,0,0,\"" + String(mqtt_pub_notif) + "\"," + String(len));
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
    Serial.println("❌ Tidak menerima prompt '>' dari modem untuk notification");
    Serial.println("Response: " + response);
    return false;
  }

  delay(20);
  Serial.println("✅ Kirim notification payload...");
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
      if (line.indexOf("+QMTPUBEX: 0,0,0") >= 0) {
        Serial.println("✅ Notification publish berhasil dikonfirmasi");
        return true;
      }
      if (line.indexOf("ERROR") >= 0) {
        Serial.println("❌ Notification publish gagal: " + line);
        return false;
      }
    }
  }

  Serial.println("⚠️ Timeout menunggu konfirmasi notification publish");
  Serial.println("Response: " + response);
  return false;
}

// Function untuk mengirim notification berdasarkan perubahan status mesin
void sendStatusNotifications(int statusMachine) {
  Serial.println("📢 ======= SEND STATUS NOTIFICATIONS =======");
  Serial.println("⚙️ Status Mesin: " + String(statusMachine ? "The device is now charging." : "The device is no longer charging."));
  sendToBluetooth("📢 ======= SEND STATUS NOTIFICATIONS =======");
  sendToBluetooth("⚙️ Status Mesin: " + String(statusMachine ? "The device is now charging." : "The device is no longer charging."));
  sendToBluetooth("🔍 DEBUG: statusMachine parameter = " + String(statusMachine));

  // Cek apakah notification sudah dikirim untuk status ini
  if (notificationSentForCurrentStatus && lastNotificationStatus == statusMachine) {
    Serial.println("⚠️ Notification sudah dikirim untuk status ini, skip...");
    sendToBluetooth("⚠️ Notification sudah dikirim untuk status ini, skip...");
    sendToBluetooth("🔍 DEBUG: lastNotificationStatus = " + String(lastNotificationStatus));
    return;
  }

  // Cek koneksi MQTT sebelum kirim notification
  if (!mqttConnected) {
    Serial.println("❌ MQTT tidak terkoneksi untuk notification");
    sendToBluetooth("❌ MQTT tidak terkoneksi untuk notification");
    return;
  }

  const char* subjectText = (statusMachine == 1) ? "Charging On" : "Charging Off";
  const char* statusText = (statusMachine == 1) ? "The device is now charging." : "The device is no longer charging.";
  
  sendToBluetooth("🔍 DEBUG: subjectText = " + String(subjectText));
  sendToBluetooth("🔍 DEBUG: statusText = " + String(statusText));
  sendToBluetooth("🔍 DEBUG: MQTT Connected = " + String(mqttConnected ? "YES" : "NO"));

  // Kirim notification untuk setiap channel
  sendToBluetooth("📤 Mulai kirim notifikasi ke semua channel...");
  
  bool emailSent = sendStatusNotificationToChannel("email", statusText, subjectText);
  delay(500);
  bool webSent = sendStatusNotificationToChannel("web", statusText, subjectText);
  delay(500);
  bool telegramSent = sendStatusNotificationToChannel("telegram", statusText, subjectText);
  delay(500);
  bool waSent = sendStatusNotificationToChannel("wa", statusText, subjectText);
  
  // Summary hasil pengiriman
  sendToBluetooth("📊 ======= NOTIFICATION SUMMARY =======");
  sendToBluetooth("📧 Email: " + String(emailSent ? "✅ SUCCESS" : "❌ FAILED"));
  sendToBluetooth("🌐 Web: " + String(webSent ? "✅ SUCCESS" : "❌ FAILED"));
  sendToBluetooth("📱 Telegram: " + String(telegramSent ? "✅ SUCCESS" : "❌ FAILED"));
  sendToBluetooth("📞 WhatsApp: " + String(waSent ? "✅ SUCCESS" : "❌ FAILED"));
  sendToBluetooth("=====================================");
  
  sendToBluetooth("✅ Selesai kirim notifikasi ke semua channel");

  // Update flag setelah notification berhasil dikirim
  notificationSentForCurrentStatus = true;
  lastNotificationStatus = statusMachine;
  Serial.println("✅ Notification flag updated - status: " + String(statusMachine));
  sendToBluetooth("✅ Notification flag updated - status: " + String(statusMachine));

  Serial.println("📢 ======================================");
  sendToBluetooth("📢 ======================================");
}

// Function untuk mengirim notification status ke channel tertentu
bool sendStatusNotificationToChannel(String channel, const char* statusText, const char* subjectText) {
  char buffer[256];
  StaticJsonDocument<256> doc;

  doc["node_id"] = node_id;
  doc["subject"] = subjectText;
  doc["message"] = statusText;
  doc["type"] = channel;
  doc["level"] = "1";

  serializeJson(doc, buffer);
  if (timerDebugEnabled) {
    Serial.println("📤 Kirim Notification ke " + channel + ": " + String(buffer));
  }
  sendToBluetooth("📤 Kirim Notification ke " + channel + ": " + String(statusText));
  sendToBluetooth("🔍 DEBUG: Channel = " + channel);
  sendToBluetooth("🔍 DEBUG: Payload length = " + String(strlen(buffer)) + " bytes");

  if (mqttPublishNotification(buffer)) {
    Serial.println("✅ MQTT publish notification " + channel + " sukses");
    sendToBluetooth("✅ Notification " + channel + " berhasil dikirim");
    return true;
  } else {
    Serial.println("❌ MQTT gagal untuk " + channel + ", coba reconnect + retry...");
    sendToBluetooth("❌ Notification " + channel + " gagal, mencoba retry...");

    // Coba reconnect MQTT
    if (mqttConnect()) {
      if (mqttPublishNotification(buffer)) {
        Serial.println("✅ MQTT retry " + channel + " sukses");
        sendToBluetooth("✅ Notification " + channel + " retry berhasil");
        return true;
      } else {
        Serial.println("❌ MQTT retry " + channel + " gagal");
        sendToBluetooth("❌ Notification " + channel + " retry gagal");
      }
    } else {
      Serial.println("❌ MQTT reconnect gagal untuk " + channel);
      sendToBluetooth("❌ MQTT reconnect gagal untuk " + channel);
    }
  }
  
  // Return false jika semua percobaan gagal
  return false;
}

// Function untuk reset notification flag
void resetNotificationFlag() {
  notificationSentForCurrentStatus = false;
  lastNotificationStatus = -1;
  Serial.println("🔄 Notification flag reset");
  sendToBluetooth("🔄 Notification flag reset");
} 