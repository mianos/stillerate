#include <Arduino.h>
#include <ESPDateTime.h>

#include "drow.hpp"

int mqtt_sent;

void mqtt_send(bool forced, DRow **drows, const int temp_sensor_count) {
  for (auto snum = 0; snum < temp_sensor_count; snum++) {
    if (drows[snum]->temp == TempSensor::TooLow) {
      continue;
    }
    auto dtc = DateTimeClass(drows[snum]->temp_set_time);
    Serial.printf( "mqtt send %.2f %+.3f %c %s\n", drows[snum]->temp, drows[snum]->diff, forced ? 'F' : 'N', dtc.toISOString().c_str());
  }
}

void handle_mqtt(const bool changes, DRow **drows, const int temp_sensor_count) {
  for (auto snum = 0; snum < temp_sensor_count; snum++) {
    auto time_since_last_mqtt_send = DateTime.now() - mqtt_sent;
    if (time_since_last_mqtt_send > 20) {
      mqtt_send(true, drows, temp_sensor_count);
      mqtt_sent = DateTime.now();
    } else if (changes) {
      mqtt_send(false, drows, temp_sensor_count);
      mqtt_sent = DateTime.now();
    }
  }
}

