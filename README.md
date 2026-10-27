# GPS EG25-G Tracker

A GPS tracker system using an ESP32 and a Quectel EG25-G module for vehicle location monitoring, featuring online/offline capabilities.

## Main Features

- **GPS Tracking**: Utilizes the EG25-G module to obtain real-time GPS data.
- **MQTT Communication**: Publishes GPS data and machine status to an MQTT broker.
- **Offline Storage**: Saves GPS data to an SD Card / SPIFFS when offline.
- **Status Monitoring**: Real-time monitoring of machine and adapter status.
- **Bluetooth Debug**: Debugging interface via Bluetooth for system monitoring.
- **Fallback GPS Data**: Uses the last known GPS data with a real-time GSM timestamp when the GPS does not have a fix.
- **Smart GPS Fallback**: When the machine status = 1 (ACTIVE) and GPS does not have a fix, the system will retrieve the latest data from `last_data.json` and update the timestamp with the real-time GSM time.

## GPS Fallback Workflow

### When Machine Status = 1 (ACTIVE)
1. The system attempts to get real-time GPS data.
2. If GPS has a fix: it publishes real-time GPS data with the original timestamp.
3. If GPS does not have a fix: 
   - Retrieves the last data from `last_data.json`.
   - Validates the data (must not be `0.0,0.0`).
   - Retrieves the real-time GSM timestamp.
   - Updates the last data with the real-time GSM timestamp.
   - Publishes the data with status "9" (real-time).
   - Saves the updated data back to `last_data.json`.

### When Machine Status = 0 (OFF)
1. The system saves the GPS data to offline storage.
2. When the offline timer expires, it publishes the stored data.
3. If the GPS does not have a fix, it uses the last known data with a real-time GSM timestamp.

## New Feature: Publish All Stored Data

### `publishAllStoredDataWhenMachineOn()`

This function will publish all data stored in SPIFFS when `statusMesin` (machine status) is 1, in the following order:

1. **Publish Machine Status** - Uses the configured alarm status.
2. **Publish GPS Status** - GPS connection status (connected/disconnected).
3. **Publish Stored Data** - All GPS data currently stored in SPIFFS.

## New Feature: Bluetooth Monitoring

### Bluetooth Interface

The system now supports real-time monitoring via Bluetooth, featuring:

1. **Real-time Notifications** - System status changes are sent via Bluetooth.
2. **Command Interface** - Control the system using Bluetooth commands.
3. **Status Monitoring** - Check GPS, MQTT, and system status via Bluetooth.
4. **Remote Control** - Remotely publish data and test connections.

### How to Use Bluetooth:

1. **Connection**: Connect to the device named "GPS_EG25G_Tracker" via Bluetooth.
2. **Commands**: Send commands through a Bluetooth serial terminal.
3. **Monitoring**: Receive real-time system status notifications.

### Bluetooth Notifications:

- 🚀 Startup system
- 🌐 Internet connection status
- 📡 MQTT connection status
- 🛰️ GPS status
- ⚙️ Machine status changes
- 📤 Data publishing status

### How it Works

- The function is called automatically when the machine turns on and MQTT is connected.
- Successfully published data is immediately deleted from SPIFFS.
- Uses batch processing with a delay between publishes.
- **Interrupt Protection**: The process will halt if `statusMesin` changes to 0.
- Provides a complete summary of the publish results (even if interrupted).

### Debug Command

```text
publishall - Force publish all stored data (Machine Status + GPS Status + Stored Data)
```

### Configuration Parameters

```cpp
const unsigned long PUBLISH_INTERVAL = 3000;  // 3 seconds between each publish
const unsigned long BATCH_DELAY = 5000;       // 5 seconds delay per batch
const int BATCH_SIZE = 5;                     // Number of data points per batch
```

### Interrupt Features

- **Status Check**: Checks the machine status before every publish action.
- **Auto Interrupt**: The process is stopped if the machine status changes to 0.
- **Graceful Exit**: Provides a full summary even if the process is interrupted.
- **Data Safety**: Only data that has been successfully published is deleted from SPIFFS.

## Configuration

### Pin Configuration
- `STATUS_PIN`: Pin for monitoring machine status (GPIO34).
- Serial AT: Pin 26 (RX) and 27 (TX) for communication with the EG25-G module.

### MQTT Configuration
- Host: ``
- Port: ``
- Username: ``
- Password: ``

### Topics
- Data Topic: `node/dd37fa764acb440c9fa800644026c525/data`
- Interval Topic: `node/dd37fa764acb440c9fa800644026c525`
- Sensors Topic: `node/dd37fa764acb440c9fa800644026c525/sensors`

## Debug Commands

The system provides various debug commands accessible via the Serial Monitor and Bluetooth:

### Serial Monitor Commands:
- `status` - Display system status
- `offline` - Display offline data information
- `test` - Test MQTT connection
- `publish` - Manually publish offline data
- `publishall` - Force publish all stored data
- `publishstatus` - Force publish machine status
- `publishgps` - Force publish GPS status
- `interval` - Show current interval info
- `help` - Show all available commands

### Bluetooth Commands:
- `status` - System status
- `gps` - GPS status and current coordinates
- `mqtt` - MQTT connection status
- `offline` - Offline data information
- `test` - Test MQTT connection
- `publish` - Manually publish offline data
- `publishall` - Force publish all stored data
- `help` - Show available Bluetooth commands

## File Structure

- `GPS_QUECTEL_Tracker.ino` - Main program
- `configure.h` - Configuration variables and function declarations
- `gsm_function.ino` - GSM/Internet connectivity functions
- `mqtt_function.ino` - MQTT communication functions
- `gps_function.ino` - GPS operations and data parsing functions
- `file_manager_function.ino` - SPIFFS file management
- `status_machine_function.ino` - Machine status monitoring
- `alarm_status_function.ino` - Alarm status functions
- `debug_function.ino` - Serial debug functions
- `bluetooth_function.ino` - Bluetooth monitoring and command functions

## Workflow

1. **Startup**: Initializes GPS, GSM, and SPIFFS.
2. **Status Check**: Continuously monitors the machine's status.
3. **Online Mode**: If the machine is on, it activates the internet and MQTT connection.
4. **Publish Stored Data**: Publishes all saved data when the machine turns on.
5. **Continuous Publishing**: Continuously publishes GPS and status data.
6. **Offline Mode**: If the machine is off, it saves GPS data to SPIFFS.
7. **Timer Management**: Publishes offline data based on configured intervals.

## Error Handling

- Auto-reconnects to MQTT when disconnected.
- Retry mechanism for failed publish attempts.
- Fallback to the last known GPS data if the GPS loses its fix.
- **GPS Auto Recovery**: Automatically reactivates the GPS if it becomes inactive or errors out.
- **Timer Offline**: Automatically runs the offline timer when the machine turns off.
- **Status Change Detection**: Real-time detection of machine status changes.
- Periodic connection monitoring.
- Automatic recovery upon system startup.
