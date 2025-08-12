// gsm_function.ino
#include "configure.h"

void sendAT(String cmd, unsigned long delayTime) {
  SerialAT.println(cmd);
  Serial.print(">> ");
  Serial.println(cmd);
  delay(delayTime);
  while (SerialAT.available()) {
    String resp = SerialAT.readString();
    Serial.print("<< ");
    Serial.println(resp);
  }
}

bool sendAndWait(String cmd, String expected, unsigned long timeout) {
  // Clear buffer first
  while (SerialAT.available()) {
    SerialAT.read();
  }

  SerialAT.println(cmd);
  Serial.print(">> ");
  Serial.println(cmd);
  sendToBluetooth(">> " + cmd);

  unsigned long start = millis();
  String response = "";

  while (millis() - start < timeout) {
    while (SerialAT.available()) {
      char c = SerialAT.read();
      response += c;

      // Check if we got the expected response
      if (response.indexOf(expected) >= 0) {
        Serial.print("<< ");
        Serial.println(response);
        sendToBluetooth("<< " + response);
        Serial.println("✅ Response received: " + expected);
        sendToBluetooth("✅ Response received: " + expected);
        return true;
      }

      // Check for modem restart (RDY signal)
      if (response.indexOf("RDY") >= 0) {
        Serial.print("<< ");
        Serial.println(response);
        sendToBluetooth("<< " + response);
        Serial.println("⚠️ MODEM RESTART DETECTED (RDY signal)");
        sendToBluetooth("⚠️ MODEM RESTART DETECTED (RDY signal)");
        
        // Reset connection flags
        mqttConnected = false;
        internetActive = false;
        gpsEnabled = false;
        
        // Wait for modem to stabilize
        Serial.println("⏱️ Menunggu modem stabil setelah restart...");
        sendToBluetooth("⏱️ Menunggu modem stabil setelah restart...");
        delay(5000); // Wait 5 seconds for modem to stabilize
        
        return false;
      }

      // Check for common error responses
      if (response.indexOf("ERROR") >= 0) {
        Serial.print("<< ");
        Serial.println(response);
        sendToBluetooth("<< " + response);
        Serial.println("❌ ERROR response received");
        sendToBluetooth("❌ ERROR response received");
        return false;
      }
      
      // Check for specific MQTT errors
      if (response.indexOf("+QMTSUB: 0,1,1,") >= 0) {
        Serial.print("<< ");
        Serial.println(response);
        sendToBluetooth("<< " + response);
        Serial.println("❌ MQTT Subscribe failed");
        sendToBluetooth("❌ MQTT Subscribe failed");
        return false;
      }
      
      // Check for timeout or connection issues
      if (response.indexOf("+QMTTOPEN: 0,1") >= 0) {
        Serial.print("<< ");
        Serial.println(response);
        sendToBluetooth("<< " + response);
        Serial.println("❌ MQTT Connection failed");
        sendToBluetooth("❌ MQTT Connection failed");
        return false;
      }
      
      // Check for successful subscribe with different QoS levels
      if (response.indexOf("+QMTSUB: 0,1,0,0") >= 0 || 
          response.indexOf("+QMTSUB: 0,1,0,1") >= 0 || 
          response.indexOf("+QMTSUB: 0,1,0,2") >= 0) {
        Serial.print("<< ");
        Serial.println(response);
        sendToBluetooth("<< " + response);
        Serial.println("✅ MQTT Subscribe successful");
        sendToBluetooth("✅ MQTT Subscribe successful");
        return true;
      }
    }
    delay(10);  // Small delay to prevent excessive CPU usage
  }

  Serial.print("<< ");
  Serial.println(response);
  sendToBluetooth("<< " + response);
  Serial.println("❌ Timeout waiting for: " + expected);
  sendToBluetooth("❌ Timeout waiting for: " + expected);
  return false;
}

bool activateInternet() {
  if (internetActive) {
    Serial.println("✅ Internet sudah aktif");
    sendToBluetooth("✅ Internet sudah aktif");
    return true;
  }

  Serial.println("🌐 Mengaktifkan internet...");
  sendToBluetooth("🌐 Mengaktifkan internet...");

  // Configure APN
  sendAT("AT+QICSGP=1,1,\"internet\",\"\",\"\",1");
  delay(500);  // Lebih singkat

  // Check if context is already active
  SerialAT.println("AT+QIACT?");
  delay(500);  // Lebih singkat
  bool alreadyActive = false;
  unsigned long startCheck = millis();
  String resp = "";

  while (millis() - startCheck < 5000) {  // Increased timeout
    if (SerialAT.available()) {
      resp += SerialAT.readString();
      if (resp.indexOf("+QIACT: 1,1,") >= 0) {
        alreadyActive = true;
        break;
      }
    }
    delay(100);  // Small delay to prevent blocking
  }

  if (alreadyActive) {
    Serial.println("✅ Context sudah aktif");
    sendToBluetooth("✅ Context sudah aktif");
    internetActive = true;
    return true;
  }

  // Activate context
  Serial.println("🔄 Mengaktifkan context...");
  if (!sendAndWait("AT+QIACT=1", "OK", 20000)) {  // Increased timeout
    Serial.println("❌ Gagal mengaktifkan internet");
    internetActive = false;
    return false;
  }

  // Verify activation dengan delay yang lebih lama
  Serial.println("⏱️ Menunggu stabilisasi koneksi...");
  delay(1000);  // Lebih singkat
  
  SerialAT.println("AT+QIACT?");
  delay(500);
  resp = "";
  startCheck = millis();

  while (millis() - startCheck < 5000) {  // Increased timeout
    if (SerialAT.available()) {
      resp += SerialAT.readString();
      if (resp.indexOf("+QIACT: 1,1,") >= 0) {
        Serial.println("✅ Internet berhasil diaktifkan");
        sendToBluetooth("✅ Internet berhasil diaktifkan");
        Serial.println("📡 Context info: " + resp);
        internetActive = true;
        return true;
      }
    }
    delay(100);  // Small delay to prevent blocking
  }

  Serial.println("❌ Gagal verifikasi aktivasi internet");
  internetActive = false;
  return false;
}

bool deactivateInternet() {
  if (!internetActive) {
    Serial.println("✅ Internet sudah tidak aktif");
    return true;
  }

  Serial.println("🔌 Menonaktifkan internet...");

  // Disconnect MQTT first if connected
  if (mqttConnected) {
    Serial.println("🔌 Disconnecting MQTT first...");
    sendAT("AT+QMTDISC=0", 2000);
    mqttConnected = false;
  }

  // Deactivate context dengan retry mechanism
  const int maxRetries = 3;
  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    Serial.println("🔄 Mencoba deactivate context... Percobaan ke-" + String(attempt) + "/" + String(maxRetries));
    
    if (sendAndWait("AT+QIDEACT=1", "OK", 10000)) {
      Serial.println("✅ Internet berhasil dinonaktifkan");
      internetActive = false;
      return true;
    } else {
      Serial.println("❌ Gagal deactivate context (Percobaan " + String(attempt) + ")");
      
      if (attempt < maxRetries) {
        Serial.println("⏱️ Menunggu 1 detik sebelum mencoba lagi...");
        delay(1000);
      }
    }
  }
  
  Serial.println("❌ Gagal menonaktifkan internet setelah " + String(maxRetries) + " percobaan");
  // Force set internetActive to false even if deactivation failed
  internetActive = false;
  return false;
}