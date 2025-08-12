// bluetooth_function.ino
#include "configure.h"

void initBluetooth() {
  SerialBT.begin("GPS_EG25G_Tracker");
  Serial.println("📱 Bluetooth initialized with name: GPS_EG25G_Tracker");
  Serial.println("📱 Connect to this device via Bluetooth to monitor");
  delay(1000);
}

void sendToBluetooth(String message) {
  SerialBT.println(message);
  delay(10); // Small delay to ensure transmission
}

void sendSystemStatusToBT() {
  Serial.println("📊 ======= SYSTEM STATUS =======");
  sendToBluetooth("📊 ======= SYSTEM STATUS =======");
  
  // Status mesin
  bool statusMesin = digitalRead(STATUS_PIN);
  sendToBluetooth("🔌 Status Mesin: " + String(statusMesin ? "HIDUP" : "MATI"));
  
  // Status internet dan MQTT
  sendToBluetooth("🌐 Internet Active: " + String(internetActive ? "YES" : "NO"));
  sendToBluetooth("📡 MQTT Connected: " + String(mqttConnected ? "YES" : "NO"));
  
  // Status timer
  sendToBluetooth("⏱️ Offline Timer Active: " + String(offlineTimerActive ? "YES" : "NO"));
  sendToBluetooth("⏱️ Online Timer Active: " + String(onlineTimerActive ? "YES" : "NO"));
  
  // Status notifikasi (Sistem baru - notification langsung tanpa flag)
  sendToBluetooth("📤 Notification System: DIRECT SEND (No Flags)");
  sendToBluetooth("📤 Notification Status: SENT ON EVERY STATUS CHANGE");
  
  // Debug info untuk notifikasi
  sendToBluetooth("🔍 DEBUG: Status notifikasi detail:");
  sendToBluetooth("   - Notification System: SIMPLIFIED (No Flags)");
  sendToBluetooth("   - Notification Behavior: SEND IMMEDIATELY");
  sendToBluetooth("   - Notification Frequency: ONCE PER STATUS CHANGE");
  
  // RAM status
  sendToBluetooth("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  sendToBluetooth("💾 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
  
  Serial.println("📊 =============================");
  sendToBluetooth("📊 =============================");
}

void sendGPSStatusToBT() {
  sendToBluetooth("🛰️ ======= GPS STATUS =======");
  sendToBluetooth("GPS Enabled: " + String(gpsEnabled ? "YES" : "NO"));
  sendToBluetooth("GPS Connected: " + String(gpsConnected ? "YES" : "NO"));
  
  if (gpsFixAchieved) {
    sendToBluetooth("🎯 GPS Fix: ACHIEVED");
    sendToBluetooth("⏱️ Boot to GPS Fix: " + String(bootToGPSFixTime) + "s");
  } else {
    sendToBluetooth("🎯 GPS Fix: NOT ACHIEVED");
  }
  
  // Try to get current GPS data
  String timestamp, lat, lon, alt;
  if (getGPSDataImmediate(timestamp, lat, lon, alt)) {
    sendToBluetooth("📍 Current GPS Data:");
    sendToBluetooth("Latitude: " + lat);
    sendToBluetooth("Longitude: " + lon);
    sendToBluetooth("Altitude: " + alt);
    sendToBluetooth("Timestamp: " + timestamp);
  } else {
    sendToBluetooth("📍 GPS Data: NOT AVAILABLE");
  }
  
  sendToBluetooth("🛰️ =========================");
}

void sendGPSDataToBT() {
  Serial.println("🛰️ ======= GPS DATA TO BLUETOOTH =======");
  sendToBluetooth("🛰️ ======= GPS DATA TO BLUETOOTH =======");
  
  // Coba dapatkan data GPS terbaru
  String lat, lon, alt, timestamp;
  bool gpsFix = getGPSDataImmediate(timestamp, lat, lon, alt);
  
  if (gpsFix) {
    sendToBluetooth("✅ GPS Status: FIX");
    sendToBluetooth("📍 Latitude: " + lat);
    sendToBluetooth("📍 Longitude: " + lon);
    sendToBluetooth("📍 Altitude: " + alt + " m");
    sendToBluetooth("⏰ Timestamp: " + timestamp);
    
    // Tampilkan juga di Serial
    Serial.println("✅ GPS Status: FIX");
    Serial.println("📍 Latitude: " + lat);
    Serial.println("📍 Longitude: " + lon);
    Serial.println("📍 Altitude: " + alt + " m");
    Serial.println("⏰ Timestamp: " + timestamp);
  } else {
    sendToBluetooth("❌ GPS Status: NO FIX");
    sendToBluetooth("⚠️ Mencoba menggunakan data terakhir...");
    
    // Coba ambil data terakhir dari SD Card
    String lastPayload;
    if (loadLastGPS(lastPayload)) {
      StaticJsonDocument<512> doc;
      DeserializationError err = deserializeJson(doc, lastPayload);
      if (!err && doc.containsKey("data")) {
        String lastLat = doc["data"]["latitude"].as<String>();
        String lastLon = doc["data"]["longitude"].as<String>();
        String lastTimestamp = doc["timestamp"].as<String>();
        
        sendToBluetooth("📍 Last Latitude: " + lastLat);
        sendToBluetooth("📍 Last Longitude: " + lastLon);
        sendToBluetooth("⏰ Last Timestamp: " + lastTimestamp);
        
        Serial.println("📍 Last Latitude: " + lastLat);
        Serial.println("📍 Last Longitude: " + lastLon);
        Serial.println("⏰ Last Timestamp: " + lastTimestamp);
      }
    } else {
      sendToBluetooth("❌ Tidak ada data GPS terakhir");
      Serial.println("❌ Tidak ada data GPS terakhir");
    }
  }
  
  // Tampilkan status GPS
  sendToBluetooth("🛰️ GPS Enabled: " + String(gpsEnabled ? "YES" : "NO"));
  sendToBluetooth("🛰️ GPS Connected: " + String(gpsConnected ? "YES" : "NO"));
  sendToBluetooth("🎯 GPS Fix Achieved: " + String(gpsFixAchieved ? "YES" : "NO"));
  
  if (gpsFixAchieved) {
    sendToBluetooth("⏱️ Boot to GPS Fix: " + String(bootToGPSFixTime) + " seconds");
  }
  
  sendToBluetooth("=====================================");
  Serial.println("=====================================");
}

void testSerialConnection() {
  Serial.println("🔌 ======= SERIAL CONNECTION TEST =======");
  sendToBluetooth("🔌 ======= SERIAL CONNECTION TEST =======");
  sendToBluetooth("📡 Pin TX: GPIO27, Pin RX: GPIO26, Baud: 115200");
  
  // Test 1: Basic AT command
  sendToBluetooth("📡 Test 1: Basic AT command...");
  Serial.println("📡 Test 1: Basic AT command...");
  
  // Clear buffer
  while (SerialAT.available()) {
    SerialAT.read();
  }
  
  // Send AT command
  SerialAT.println("AT");
  delay(1000);
  
  String response = "";
  unsigned long startTime = millis();
  
  while (millis() - startTime < 3000) {
    if (SerialAT.available()) {
      response += SerialAT.readString();
    }
  }
  
  if (response.indexOf("OK") >= 0) {
    Serial.println("✅ AT command response: OK");
    sendToBluetooth("✅ AT command response: OK");
    sendToBluetooth("📄 Response: " + response);
  } else {
    Serial.println("❌ No OK response from AT command");
    sendToBluetooth("❌ No OK response from AT command");
    sendToBluetooth("📄 Response: " + response);
  }
  
  // Test 2: Check module type
  sendToBluetooth("📡 Test 2: Check module type...");
  Serial.println("📡 Test 2: Check module type...");
  
  // Clear buffer
  while (SerialAT.available()) {
    SerialAT.read();
  }
  
  // Send AT+CGMI command to get manufacturer
  SerialAT.println("AT+CGMI");
  delay(1000);
  
  response = "";
  startTime = millis();
  
  while (millis() - startTime < 3000) {
    if (SerialAT.available()) {
      response += SerialAT.readString();
    }
  }
  
  if (response.indexOf("Quectel") >= 0) {
    Serial.println("✅ Module detected: Quectel");
    sendToBluetooth("✅ Module detected: Quectel");
    sendToBluetooth("📄 Response: " + response);
  } else if (response.indexOf("SIMCom") >= 0) {
    Serial.println("✅ Module detected: SIMCom");
    sendToBluetooth("✅ Module detected: SIMCom");
    sendToBluetooth("📄 Response: " + response);
  } else {
    Serial.println("⚠️ Unknown module type");
    sendToBluetooth("⚠️ Unknown module type");
    sendToBluetooth("📄 Response: " + response);
  }
  
  // Test 3: Check signal strength
  sendToBluetooth("📡 Test 3: Check signal strength...");
  Serial.println("📡 Test 3: Check signal strength...");
  
  // Clear buffer
  while (SerialAT.available()) {
    SerialAT.read();
  }
  
  // Send AT+CSQ command
  SerialAT.println("AT+CSQ");
  delay(1000);
  
  response = "";
  startTime = millis();
  
  while (millis() - startTime < 3000) {
    if (SerialAT.available()) {
      response += SerialAT.readString();
    }
  }
  
  if (response.indexOf("+CSQ:") >= 0) {
    Serial.println("✅ Signal strength response received");
    sendToBluetooth("✅ Signal strength response received");
    sendToBluetooth("📄 Response: " + response);
  } else {
    Serial.println("❌ No signal strength response");
    sendToBluetooth("❌ No signal strength response");
    sendToBluetooth("📄 Response: " + response);
  }
  
  // Test 4: Check SIM card
  sendToBluetooth("📡 Test 4: Check SIM card...");
  Serial.println("📡 Test 4: Check SIM card...");
  
  // Clear buffer
  while (SerialAT.available()) {
    SerialAT.read();
  }
  
  // Send AT+CPIN? command
  SerialAT.println("AT+CPIN?");
  delay(1000);
  
  response = "";
  startTime = millis();
  
  while (millis() - startTime < 3000) {
    if (SerialAT.available()) {
      response += SerialAT.readString();
    }
  }
  
  if (response.indexOf("READY") >= 0) {
    Serial.println("✅ SIM card is ready");
    sendToBluetooth("✅ SIM card is ready");
    sendToBluetooth("📄 Response: " + response);
  } else {
    Serial.println("❌ SIM card not ready");
    sendToBluetooth("❌ SIM card not ready");
    sendToBluetooth("📄 Response: " + response);
  }
  
  Serial.println("🔌 ======================================");
  sendToBluetooth("🔌 ======================================");
}

void sendSerialTestToBT() {
  sendToBluetooth("🔌 ======= SERIAL TEST SUMMARY =======");
  sendToBluetooth("📡 Pin TX: GPIO27");
  sendToBluetooth("📡 Pin RX: GPIO26");
  sendToBluetooth("📡 Baud Rate: 115200");
  sendToBluetooth("🔌 =================================");
}

void sendMQTTStatusToBT() {
  sendToBluetooth("📡 ======= MQTT STATUS =======");
  sendToBluetooth("Connected: " + String(mqttConnected ? "YES" : "NO"));
  sendToBluetooth("Subscription: " + String(subscriptionActive ? "ACTIVE" : "INACTIVE"));
  sendToBluetooth("Host: " + String(mqttHost));
  sendToBluetooth("Port: " + String(mqttPort));
  sendToBluetooth("Topic: " + String(mqttTopic));
  
  if (mqttConnected) {
    sendToBluetooth("✅ MQTT Connection: STABLE");
  } else {
    sendToBluetooth("❌ MQTT Connection: DISCONNECTED");
  }
  
  sendToBluetooth("📡 ==========================");
}

void sendChargingStatusToBT() {
  Serial.println("🔋 ======= CHARGING STATUS =======");
  sendToBluetooth("🔋 ======= CHARGING STATUS =======");
  
  // Status mesin dan adapter menggunakan helper functions
  bool statusMesin = digitalRead(STATUS_PIN);
  bool adapterStatus = isAdapterConnected();
  bool chargingActive = isChargingActive();
  
  sendToBluetooth("🔌 Status Mesin: " + String(statusMesin ? "HIDUP" : "MATI"));
  sendToBluetooth("🔌 Status Adapter: " + String(adapterStatus ? "TERPASANG" : "TIDAK TERPASANG"));
  sendToBluetooth("🔋 Charging Active: " + String(chargingActive ? "YES" : "NO"));
  
  // Status notifikasi (Sistem baru - notification langsung tanpa flag)
  sendToBluetooth("📤 Notification System: DIRECT SEND (No Flags)");
  sendToBluetooth("📤 Notification Status: SENT ON EVERY STATUS CHANGE");
  
  // Debug info untuk notifikasi
  sendToBluetooth("🔍 DEBUG: Charging notification status:");
  sendToBluetooth("   - Notification System: SIMPLIFIED (No Flags)");
  sendToBluetooth("   - Notification Behavior: SEND IMMEDIATELY");
  sendToBluetooth("   - Notification Frequency: ONCE PER STATUS CHANGE");
  sendToBluetooth("   - Current charging state: " + String(chargingActive ? "CHARGING" : "NOT CHARGING"));
  
  Serial.println("🔋 =============================");
  sendToBluetooth("🔋 =============================");
}

// Override display functions to also send to Bluetooth
void displaySystemStatus() {
  Serial.println("🔍 ======= SYSTEM STATUS =======");
  Serial.println("🌐 Internet Active: " + String(internetActive ? "YES" : "NO"));
  Serial.println("📡 MQTT Connected: " + String(mqttConnected ? "YES" : "NO"));
  Serial.println("📋 MQTT Subscription: " + String(subscriptionActive ? "ACTIVE" : "INACTIVE"));
  Serial.println("⚙️ Machine Status: " + String(digitalRead(STATUS_PIN) ? "ON" : "OFF"));
  Serial.println("🛰️ GPS Enabled: " + String(gpsEnabled ? "YES" : "NO"));
  Serial.println("📡 GPS Connected: " + String(gpsConnected ? "YES" : "NO"));
  Serial.println("🔍 Timer Debug: " + String(timerDebugEnabled ? "ENABLED" : "DISABLED"));
  Serial.println("📤 Notification System: DIRECT SEND (No Flags)");
  
  // Timer status
  if (offlineTimerActive) {
    unsigned long currentMillis = millis();
    unsigned long elapsed;
    
    if (currentMillis >= offlineTimer) {
      elapsed = currentMillis - offlineTimer;
    } else {
      elapsed = (0xFFFFFFFF - offlineTimer) + currentMillis + 1;
    }
    
    unsigned long remaining = offlineInterval - elapsed;
    Serial.println("⏱️ Timer Active: YES");
    Serial.println("⏱️ Timer Interval: " + String(offlineInterval/1000) + "s");
    Serial.println("⏱️ Timer Elapsed: " + String(elapsed/1000) + "s");
    Serial.println("⏱️ Timer Remaining: " + String(remaining/1000) + "s");
  } else {
    Serial.println("⏱️ Timer Active: NO");
  }
  
  Serial.println("🔍 =============================");
  
  // Also send to Bluetooth
  sendSystemStatusToBT();
}

void displayGPSStatusSummary() {
  Serial.println("🛰️ ======= GPS STATUS SUMMARY =======");
  Serial.println("GPS Enabled: " + String(gpsEnabled ? "YES" : "NO"));
  Serial.println("GPS Connected: " + String(gpsConnected ? "YES" : "NO"));
  Serial.println("⏱️ Last Check: " + String((millis() - lastGPSCheck) / 1000) + "s ago");
  
  if (gpsFixAchieved) {
    Serial.println("🎯 GPS Fix Achieved: YES");
    Serial.println("⏱️ Boot to GPS Fix: " + String(bootToGPSFixTime) + " seconds");
  } else {
    Serial.println("🎯 GPS Fix Achieved: NO");
  }
  
  Serial.println("🛰️ =================================");
  
  // Also send to Bluetooth
  sendGPSStatusToBT();
} 