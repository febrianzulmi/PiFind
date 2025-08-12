# GPS EG25-G Tracker

Sistem GPS tracker menggunakan ESP32 dan modul Quectel EG25-G untuk monitoring lokasi kendaraan dengan fitur online/offline.

## Fitur Utama

- **GPS Tracking**: Menggunakan modul EG25-G untuk mendapatkan data GPS real-time
- **MQTT Communication**: Publish data GPS dan status mesin ke MQTT broker
- **Offline Storage**: Menyimpan data GPS di SD Card saat offline
- **Status Monitoring**: Monitor status mesin dan adapter secara real-time
- **Bluetooth Debug**: Interface debug melalui Bluetooth untuk monitoring
- **Fallback GPS Data**: Menggunakan data GPS terakhir dengan timestamp GSM realtime saat GPS tidak fix
- **Smart GPS Fallback**: Saat status mesin = 1 (AKTIF) dan GPS tidak fix, sistem akan mengambil data terakhir dari `last_data.json` dan memperbarui timestamp dengan waktu GSM realtime

## Alur Kerja GPS Fallback

### Saat Status Mesin = 1 (AKTIF)
1. Sistem mencoba mendapatkan data GPS real-time
2. Jika GPS fix: publish data GPS real-time dengan timestamp asli
3. Jika GPS tidak fix: 
   - Ambil data terakhir dari `last_data.json`
   - Validasi data (tidak boleh 0.0,0.0)
   - Ambil timestamp GSM realtime
   - Update data terakhir dengan timestamp GSM realtime
   - Publish data dengan status "9" (real-time)
   - Simpan data yang sudah diupdate ke `last_data.json`

### Saat Status Mesin = 0 (MATI)
1. Sistem menyimpan data GPS ke offline storage
2. Saat timer offline expired, publish data tersimpan
3. Jika GPS tidak fix, gunakan data terakhir dengan timestamp GSM realtime

## Fitur Baru: Publish All Stored Data

### `publishAllStoredDataWhenMachineOn()`

Fungsi ini akan publish seluruh data tersimpan di SPIFFS saat statusMesin 1 dengan urutan:

1. **Publish StatusMesin** - Menggunakan alarm status yang dikonfigurasi
2. **Publish StatusGPS** - Status koneksi GPS (connected/disconnected)  
3. **Publish Stored Data** - Seluruh data GPS yang tersimpan di SPIFFS

## Fitur Baru: Bluetooth Monitoring

### Bluetooth Interface

Sistem sekarang mendukung monitoring real-time melalui Bluetooth dengan fitur:

1. **Real-time Notifications** - Status perubahan sistem dikirim ke Bluetooth
2. **Command Interface** - Kontrol sistem melalui Bluetooth commands
3. **Status Monitoring** - Cek status GPS, MQTT, dan sistem melalui Bluetooth
4. **Remote Control** - Publish data dan test koneksi dari jarak jauh

### Cara Menggunakan Bluetooth:

1. **Koneksi**: Hubungkan ke device "GPS_EG25G_Tracker" via Bluetooth
2. **Commands**: Kirim command melalui terminal Bluetooth
3. **Monitoring**: Terima notifikasi real-time status sistem

### Notifikasi Bluetooth:

- 🚀 Startup system
- 🌐 Internet connection status
- 📡 MQTT connection status
- 🛰️ GPS status
- ⚙️ Machine status changes
- 📤 Data publishing status

### Cara Kerja

- Fungsi dipanggil otomatis saat mesin hidup dan MQTT terkoneksi
- Data yang berhasil dipublish langsung dihapus dari SPIFFS
- Menggunakan batch processing dengan delay antar publish
- **Interrupt Protection**: Proses akan dihentikan jika statusMesin berubah menjadi 0
- Summary lengkap hasil publish (termasuk jika diinterrupt)

### Debug Command

```
publishall - Force publish all stored data (statusMesin + statusGPS + stored data)
```

### Parameter Konfigurasi

```cpp
const unsigned long PUBLISH_INTERVAL = 3000;  // 3 detik antara setiap publish
const unsigned long BATCH_DELAY = 5000;       // 5 detik setiap batch (setiap 5 data)
const int BATCH_SIZE = 5;                     // Jumlah data per batch
```

### Fitur Interrupt

- **Status Check**: Cek status mesin sebelum setiap publish
- **Auto Interrupt**: Proses dihentikan jika statusMesin berubah menjadi 0
- **Graceful Exit**: Summary lengkap meskipun proses diinterrupt
- **Data Safety**: Data yang sudah dipublish tetap dihapus dari SPIFFS

## Konfigurasi

### Pin Configuration
- `STATUS_PIN`: Pin untuk monitoring status mesin (GPIO34)
- Serial AT: Pin 26 (RX) dan 27 (TX) untuk komunikasi dengan EG25-G

### MQTT Configuration
- Host: mqtt-nearon.synapsis.id
- Port: 32100
- Username: synapsis
- Password: Rasp8266!

### Topics
- Data Topic: `node/dd37fa764acb440c9fa800644026c525/data`
- Interval Topic: `node/dd37fa764acb440c9fa800644026c525`
- Sensors Topic: `node/dd37fa764acb440c9fa800644026c525/sensors`

## Debug Commands

Sistem menyediakan berbagai command debug melalui Serial Monitor dan Bluetooth:

### Serial Monitor Commands:
- `status` - Display system status
- `offline` - Display offline data info
- `test` - Test MQTT connection
- `publish` - Manual publish offline data
- `publishall` - Force publish all stored data
- `publishstatus` - Force publish status mesin
- `publishgps` - Force publish statusGPS
- `interval` - Show current interval info
- `help` - Show all available commands

### Bluetooth Commands:
- `status` - System status
- `gps` - GPS status and current coordinates
- `mqtt` - MQTT connection status
- `offline` - Offline data info
- `test` - Test MQTT connection
- `publish` - Manual publish offline data
- `publishall` - Force publish all stored data
- `help` - Show available Bluetooth commands

## File Structure

- `GPS_QUECTEL_Tracker.ino` - Main program
- `configure.h` - Configuration and function declarations
- `gsm_function.ino` - GSM/Internet functions
- `mqtt_function.ino` - MQTT communication functions
- `gps_function.ino` - GPS functions
- `file_manager_function.ino` - SPIFFS file management
- `status_machine_function.ino` - Machine status monitoring
- `alarm_status_function.ino` - Alarm status functions
- `debug_function.ino` - Debug functions
- `bluetooth_function.ino` - Bluetooth monitoring functions

## Alur Kerja

1. **Startup**: Inisialisasi GPS, GSM, dan SPIFFS
2. **Status Check**: Monitor status mesin
3. **Online Mode**: Jika mesin hidup, aktifkan internet dan MQTT
4. **Publish Stored Data**: Publish seluruh data tersimpan saat mesin hidup
5. **Continuous Publishing**: Publish GPS dan status secara kontinyu
6. **Offline Mode**: Jika mesin mati, simpan data GPS di SPIFFS
7. **Timer Management**: Publish data offline berdasarkan interval

## Error Handling

- Auto reconnect MQTT saat terputus
- Retry mechanism untuk publish gagal
- Fallback ke data GPS terakhir jika GPS tidak fix
- **GPS Auto Recovery**: Otomatis aktifkan GPS jika tidak aktif atau error
- **Timer Offline**: Otomatis jalankan timer offline saat mesin mati
- **Status Change Detection**: Deteksi perubahan status mesin secara real-time
- Monitoring koneksi berkala
- Recovery otomatis saat startup