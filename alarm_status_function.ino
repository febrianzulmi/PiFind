// alarm_status_function.ino
#include "configure.h"


String determineAlarmStatus(int status, int alarmType, int alarmHigh, int alarmLow) {
  // Kombinasi khusus untuk status mesin
  if (alarmType == 1) {
    if (alarmHigh == 0 && alarmLow == 1) {
      // LOW jika status 0
      return (status == 0) ? "2" : "1"; // 2 = LOW, 1 = NORMAL
    }
    if (alarmHigh == 1 && alarmLow == 0) {
      // HIGH jika status 1
      return (status == 1) ? "3" : "1"; // 3 = HIGH, 1 = NORMAL
    }
    // Default fallback
    return (status == alarmLow) ? "1" : "2";  // Min Only
  }
  if (alarmType == 2) {
    return (status == alarmHigh) ? "1" : "3";  // Max Only
  }
  if (alarmType == 3) {
    if (status == alarmLow || status == alarmHigh) return "1";
    if (status < alarmLow) return "2";   // Too low
    if (status > alarmHigh) return "3";  // Too high
    return "3";                          // Optional: treat middle values as alarm
  }
  return "9";  // Realtime biasa
}

void subscribeStatusMesinAlarm() {
  mqttSubscribe(mqttSensorsTopic);
} 