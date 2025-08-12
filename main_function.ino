// main_function.ino
#include "configure.h"

// This file contains helper functions and should not have setup() or loop()
// The main setup() and loop() functions are in GPS_QUECTEL_Tracker.ino 

// Power Supply Monitoring Functions

void monitorPowerSupply() {
  unsigned long currentMillis = millis();
  unsigned long elapsedSinceLastCheck;
  
  // Handle millis() overflow
  if (currentMillis >= lastPowerCheck) {
    elapsedSinceLastCheck = currentMillis - lastPowerCheck;
  } else {
    elapsedSinceLastCheck = (0xFFFFFFFF - lastPowerCheck) + currentMillis + 1;
  }
  
  if (elapsedSinceLastCheck > powerCheckInterval) {
    lastPowerCheck = currentMillis;
    
    bool statusMesin = digitalRead(STATUS_PIN);
    size_t currentFreeHeap = ESP.getFreeHeap();
    
    // Check power stability
    if (statusMesin) {
      // Status mesin = 1 (AC power ON)
      if (currentFreeHeap < 30000) {
        powerStable = false;
        Serial.println("⚠️ Power supply tidak stabil (AC mode) - Free heap: " + String(currentFreeHeap));
        sendToBluetooth("⚠️ Power supply tidak stabil (AC mode) - Free heap: " + String(currentFreeHeap));
      } else {
        powerStable = true;
        Serial.println("✅ Power supply stabil (AC mode) - Free heap: " + String(currentFreeHeap));
        sendToBluetooth("✅ Power supply stabil (AC mode) - Free heap: " + String(currentFreeHeap));
      }
    } else {
      // Status mesin = 0 (Battery mode)
      if (currentFreeHeap < 25000) {
        powerStable = false;
        Serial.println("⚠️ Power supply tidak stabil (Battery mode) - Free heap: " + String(currentFreeHeap));
        sendToBluetooth("⚠️ Power supply tidak stabil (Battery mode) - Free heap: " + String(currentFreeHeap));
      } else {
        powerStable = true;
        Serial.println("✅ Power supply stabil (Battery mode) - Free heap: " + String(currentFreeHeap));
        sendToBluetooth("✅ Power supply stabil (Battery mode) - Free heap: " + String(currentFreeHeap));
      }
    }
    
    lastFreeHeap = currentFreeHeap;
  }
}

bool isPowerSupplyStable() {
  return powerStable;
}

void logPowerStatus() {
  bool statusMesin = digitalRead(STATUS_PIN);
  size_t currentFreeHeap = ESP.getFreeHeap();
  
  Serial.println("⚡ ======= POWER SUPPLY STATUS =======");
  Serial.println("🔌 Status Mesin: " + String(statusMesin ? "AC POWER ON" : "BATTERY MODE"));
  Serial.println("💾 Free Heap: " + String(currentFreeHeap) + " bytes");
  Serial.println("📊 Power Stable: " + String(powerStable ? "YES" : "NO"));
  Serial.println("🔋 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
  Serial.println("📈 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
  Serial.println("⚡ =================================");
  
  sendToBluetooth("⚡ ======= POWER SUPPLY STATUS =======");
  sendToBluetooth("🔌 Status Mesin: " + String(statusMesin ? "AC POWER ON" : "BATTERY MODE"));
  sendToBluetooth("💾 Free Heap: " + String(currentFreeHeap) + " bytes");
  sendToBluetooth("📊 Power Stable: " + String(powerStable ? "YES" : "NO"));
  sendToBluetooth("🔋 Total Heap: " + String(ESP.getHeapSize()) + " bytes");
  sendToBluetooth("📈 Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes");
  sendToBluetooth("⚡ =================================");
} 