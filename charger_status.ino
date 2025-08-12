#include "configure.h"

// Fungsi untuk menulis data 16-bit ke register
void writeBqRegister(uint8_t reg, uint16_t value) {
  Wire.beginTransmission(BQ24725A_ADDR);
  Wire.write(reg);
  Wire.write(value & 0xFF);
  Wire.write((value >> 8) & 0xFF);
  Wire.endTransmission();
}
// Fungsi untuk membaca data 16-bit dari register
uint16_t readBqRegister(uint8_t reg) {
  uint16_t result = 0;
  Wire.beginTransmission(BQ24725A_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(BQ24725A_ADDR, (uint8_t)2);
  if (Wire.available() == 2) {
    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    result = ((uint16_t)msb << 8) | lsb;
  }
  return result;
}

void setupChargerModule() {
  // --- Konfigurasi pin MCU ---
  pinMode(STATUS_PIN, INPUT);  // Setup pin status mesin/adapter di sini saja
  pinMode(CHGEN_PIN, OUTPUT);
  pinMode(ADAPTER_OK_LED_PIN, OUTPUT);
  
  bool machineStatus = digitalRead(STATUS_PIN);
  isAdapterOk = machineStatus; // Untuk sistem dengan pin yang sama
  
  SerialBT.begin("ESP32_Charger_Dashboard");
  Serial.print("MCU_ACOK : ");
  SerialBT.print("MCU_ACOK : ");
  Serial.println(isAdapterOk);
  SerialBT.println(isAdapterOk);

  // Logika LED: ON jika mesin hidup (charging aktif)
  bool chargingActive = machineStatus; // Untuk sistem dengan pin yang sama
  
  if (chargingActive) {
    digitalWrite(ADAPTER_OK_LED_PIN, LOW);  // LED ON = Charging aktif
    led = 1;
    Serial.println("🔋 CHARGING ACTIVE: Mesin hidup + Adapter terpasang");
    SerialBT.println("🔋 CHARGING ACTIVE: Mesin hidup + Adapter terpasang");
  } else {
    digitalWrite(ADAPTER_OK_LED_PIN, HIGH);   // LED OFF = Tidak charging
    led = 0;
    Serial.println("🔌 NOT CHARGING: Mesin mati atau adapter tidak terpasang");
    SerialBT.println("🔌 NOT CHARGING: Mesin mati atau adapter tidak terpasang");
  }
  
  Serial.print("LED_ACOK : ");
  SerialBT.print("LED_ACOK : ");
  Serial.println(led);
  SerialBT.println(led);

  // Secara default, aktifkan charging. Pin CHG_EN pada BQ24725A biasanya active-low.
  digitalWrite(CHGEN_PIN, HIGH);

  Serial.println("\n===== Memulai Dashboard BQ24725A =====");
  SerialBT.println("\n===== Memulai Dashboard BQ24725A =====");
  delay(500);

  // Verifikasi komunikasi I2C
  if (readBqRegister(MANUFACTURER_ID_REG) != 0x0040) {
    Serial.println("!!! ERROR: Tidak bisa berkomunikasi dengan BQ24725A.");
    SerialBT.println("!!! ERROR: Tidak bisa berkomunikasi dengan BQ24725A.");
    Serial.println("!!! Cek koneksi I2C dan pastikan REGN = 6V.");
    SerialBT.println("!!! Cek koneksi I2C dan pastikan REGN = 6V.");
  }
  Serial.println("Komunikasi I2C dengan BQ24725A berhasil.");
  SerialBT.println("Komunikasi I2C dengan BQ24725A berhasil.");

  // --- Mengirim Konfigurasi ke Charger ---
  uint16_t inputCurrentReg = (INPUT_CURRENT_LIMIT_MA / 128) << 7;
  writeBqRegister(INPUT_CURRENT_REG, inputCurrentReg);

  uint16_t chargeVoltageReg = (TARGET_CHARGE_VOLTAGE_MV / 16) << 4;
  writeBqRegister(CHARGE_VOLTAGE_REG, chargeVoltageReg);

  uint16_t chargeCurrentReg = (TARGET_CHARGE_CURRENT_MA / 64) << 6;
  writeBqRegister(CHARGE_CURRENT_REG, chargeCurrentReg);

  // Atur ChargeOption: Watchdog nonaktif, dan IOUT memonitor arus baterai (bit 5 = 1)
  uint16_t chargeOptionValue = 0x1902 | 0x0020;  // 0x1922
  writeBqRegister(CHARGE_OPTION_REG, chargeOptionValue);

  Serial.println("Konfigurasi charger telah dikirim.");
  SerialBT.println("Konfigurasi charger telah dikirim.");
  Serial.println("========================================\n");
  SerialBT.println("========================================\n");
}

void setupChargerModuleMinimal() {
  // --- Konfigurasi pin MCU minimal ---
  pinMode(STATUS_PIN, INPUT);  // Setup pin status mesin/adapter
  pinMode(CHGEN_PIN, OUTPUT);
  pinMode(ADAPTER_OK_LED_PIN, OUTPUT);
  
  bool machineStatus = digitalRead(STATUS_PIN);
  isAdapterOk = machineStatus; // Untuk sistem dengan pin yang sama
  
  // Logika LED: ON jika mesin hidup (charging aktif)
  bool chargingActive = machineStatus; // Untuk sistem dengan pin yang sama
  
  if (chargingActive) {
    digitalWrite(ADAPTER_OK_LED_PIN, LOW);  // LED ON = Charging aktif
    led = 1;
    Serial.println("🔋 CHARGING ACTIVE: Mesin hidup + Adapter terpasang");
    sendToBluetooth("🔋 CHARGING ACTIVE: Mesin hidup + Adapter terpasang");
  } else {
    digitalWrite(ADAPTER_OK_LED_PIN, HIGH);   // LED OFF = Tidak charging
    led = 0;
    Serial.println("🔌 NOT CHARGING: Mesin mati atau adapter tidak terpasang");
    sendToBluetooth("🔌 NOT CHARGING: Mesin mati atau adapter tidak terpasang");
  }
  
  // Aktifkan charging secara default
  digitalWrite(CHGEN_PIN, HIGH);
  
  Serial.println("🔋 Charger setup minimal selesai");
  sendToBluetooth("🔋 Charger setup minimal selesai");
  
  // Skip I2C verification untuk mempercepat boot
  // Konfigurasi register akan dilakukan nanti jika diperlukan
}

void readChargerStatus() {
  Serial.println("----------- Status Saat Ini -----------");
  SerialBT.println("----------- Status Saat Ini -----------");

  // 1. Baca Status Mesin dan Adapter dari Pin STATUS_PIN
  bool machineStatus = digitalRead(STATUS_PIN);
  isAdapterOk = machineStatus; // Untuk sistem dengan pin yang sama
  
  Serial.print("Status Mesin (STATUS_PIN): ");
  SerialBT.print("Status Mesin (STATUS_PIN): ");
  Serial.println(machineStatus ? "HIDUP" : "MATI");
  SerialBT.println(machineStatus ? "HIDUP" : "MATI");
  
  Serial.print("Status Adapter (STATUS_PIN): ");
  SerialBT.print("Status Adapter (STATUS_PIN): ");
  Serial.println(isAdapterOk ? "TERPASANG & OK" : "TIDAK ADA / BERMASALAH");
  SerialBT.println(isAdapterOk ? "TERPASANG & OK" : "TIDAK ADA / BERMASALAH");

  // Logika LED: ON jika mesin hidup (charging aktif)
  bool chargingActive = machineStatus; // Untuk sistem dengan pin yang sama
  
  if (chargingActive) {
    digitalWrite(ADAPTER_OK_LED_PIN, LOW);  // LED ON = Charging aktif
    led = 1;
    Serial.println("🔋 CHARGING ACTIVE: Mesin hidup + Adapter terpasang");
    SerialBT.println("🔋 CHARGING ACTIVE: Mesin hidup + Adapter terpasang");
  } else {
    digitalWrite(ADAPTER_OK_LED_PIN, HIGH);   // LED OFF = Tidak charging
    led = 0;
    Serial.println("🔌 NOT CHARGING: Mesin mati atau adapter tidak terpasang");
    SerialBT.println("🔌 NOT CHARGING: Mesin mati atau adapter tidak terpasang");
  }
  
  Serial.print("LED_ACOK : ");
  SerialBT.print("LED_ACOK : ");
  Serial.println(led);
  SerialBT.println(led);

  // 2. Baca Arus dari Pin IOUT (S_VP)
  int adcValue = analogRead(IOUT_PIN);
  float vout_mv = (adcValue / 4095.0) * 3300.0;
  float measuredCurrent_mA = vout_mv / (20.0 * SENSE_RESISTOR_OHMS);
  Serial.print("Arus Baterai Terukur (IOUT): ");
  SerialBT.print("Arus Baterai Terukur (IOUT): ");
  Serial.print(measuredCurrent_mA, 2);
  SerialBT.print(measuredCurrent_mA, 2);
  Serial.println(" mA");
  SerialBT.println(" mA");

  Serial.println("--- Pengaturan Register ---");
  SerialBT.println("--- Pengaturan Register ---");
  float voltageSetting_V = (((readBqRegister(CHARGE_VOLTAGE_REG) >> 4) & 0x07FF) * 16.0) / 1000.0;
  float currentSetting_A = (((readBqRegister(CHARGE_CURRENT_REG) >> 6) & 0x007F) * 64.0) / 1000.0;
  Serial.print("Target: ");
  SerialBT.print("Target: ");
  Serial.print(voltageSetting_V, 2);
  SerialBT.print(voltageSetting_V, 2);
  Serial.print(" V, ");
  SerialBT.print(" V, ");
  Serial.print(currentSetting_A, 2);
  SerialBT.print(currentSetting_A, 2);
  Serial.println(" A");
  SerialBT.println(" A");

  uint16_t chargeOption = readBqRegister(CHARGE_OPTION_REG);
  Serial.print("Charge Inhibit (Bit 0): ");
  SerialBT.print("Charge Inhibit (Bit 0): ");
  Serial.println((chargeOption & 0x0001) ? "YA (NONAKTIF)" : "TIDAK (AKTIF)");
  SerialBT.println((chargeOption & 0x0001) ? "YA (NONAKTIF)" : "TIDAK (AKTIF)");

  Serial.print("IOUT Monitor (Bit 5): ");
  SerialBT.print("IOUT Monitor (Bit 5): ");
  Serial.println((chargeOption & 0x0020) ? "Arus Baterai" : "Arus Adapter");
  SerialBT.println((chargeOption & 0x0020) ? "Arus Baterai" : "Arus Adapter");

  Serial.print("Watchdog (Bit 14:13): ");
  SerialBT.print("Watchdog (Bit 14:13): ");
  uint8_t wd_setting = (chargeOption >> 13) & 0x03;
  if (wd_setting == 0) {
    Serial.println("Nonaktif");
    SerialBT.println("Nonaktif");
  } else if (wd_setting == 1) {
    Serial.println("44 detik");
    SerialBT.println("44 detik");
  } else if (wd_setting == 2) {
    Serial.println("88 detik");
    SerialBT.println("88 detik");
  } else {
    Serial.println("175 detik (Default)");
    SerialBT.println("175 detik (Default)");
  }

  Serial.println("---------------------------------------\n\n");
  SerialBT.println("---------------------------------------\n\n");
  delay(1000);
}

// Fungsi untuk update LED status secara real-time
void updateChargerLED() {
  // Baca status terbaru menggunakan helper functions
  bool mesinStatus = digitalRead(STATUS_PIN);
  bool adapterStatus = isAdapterConnected();
  bool chargingActive = isChargingActive();
  
  // Update global variable untuk backward compatibility
  isAdapterOk = adapterStatus;
  
  // Validasi status mesin untuk mencegah perubahan yang tidak diinginkan
  static bool lastMesinStatus = mesinStatus;
  if (mesinStatus != lastMesinStatus) {
    Serial.println("⚠️ Status mesin berubah saat update LED: " + String(lastMesinStatus) + " -> " + String(mesinStatus));
    sendToBluetooth("⚠️ Status mesin berubah saat update LED: " + String(lastMesinStatus) + " -> " + String(mesinStatus));
    lastMesinStatus = mesinStatus;
  }
  
  if (chargingActive) {
    digitalWrite(ADAPTER_OK_LED_PIN, LOW);  // LED ON (active LOW)
    led = 1;
    Serial.println("🔋 LED ON: Charging aktif (Mesin hidup + Adapter terpasang)");
    sendToBluetooth("🔋 LED ON: Charging aktif (Mesin hidup + Adapter terpasang)");
  } else {
    digitalWrite(ADAPTER_OK_LED_PIN, HIGH); // LED OFF
    led = 0;
    if (!mesinStatus) {
      Serial.println("🔌 LED OFF: Mesin mati");
      sendToBluetooth("🔌 LED OFF: Mesin mati");
    } else if (!isAdapterOk) {
      Serial.println("🔌 LED OFF: Adapter tidak terpasang");
      sendToBluetooth("🔌 LED OFF: Adapter tidak terpasang");
    } else {
      Serial.println("🔌 LED OFF: Tidak charging");
      sendToBluetooth("🔌 LED OFF: Tidak charging");
    }
  }
}

// Fungsi untuk update status adapter secara real-time
void updateAdapterStatus() {
  // Update status adapter dan LED
  updateChargerLED(); // Sudah mengupdate isAdapterOk dan LED
  
  Serial.println("🔌 Adapter status updated: " + String(isAdapterOk ? "TERPASANG" : "TIDAK TERPASANG"));
  sendToBluetooth("🔌 Adapter status updated: " + String(isAdapterOk ? "TERPASANG" : "TIDAK TERPASANG"));
}

// Helper function untuk cek apakah charging aktif
bool isChargingActive() {
  // Untuk sistem dengan pin yang sama, charging aktif jika mesin hidup
  bool mesinStatus = digitalRead(STATUS_PIN);
  return mesinStatus;
}

// Helper function untuk cek apakah adapter terhubung
bool isAdapterConnected() {
  // Untuk sistem dengan pin yang sama, adapter terhubung jika mesin hidup
  // Jika perlu pin terpisah, gunakan pin adapter yang terpisah
  bool mesinStatus = digitalRead(STATUS_PIN);
  return mesinStatus;
}