#pragma once
#include <math.h>

#include "tempsensor.h"

struct DRow {
  float temp = TempSensor::TooLow;
  
  unsigned long receive_time_t = 0;

  float last_temp_sent = TempSensor::TooLow;;
  unsigned long temp_set_time = 0;

  bool changed = false;
  float diff = 0.0;
  static constexpr float delta = 0.1;

  unsigned long time_at_this_temp;

  DRow() {}
  void ResetChanged() {
    changed = false;
  }

  bool isValid() {
    if (temp <= TempSensor::TooLow || temp > TempSensor::TooHigh) {
          return false;
    }
    return true;
  }

  bool  Update(const float in_temp, int snum) {
    if (in_temp <= TempSensor::TooLow || in_temp > TempSensor::TooHigh) {
          return false;
    }
    if (temp_set_time) {
      time_at_this_temp = DateTime.now() - temp_set_time;
    } else {
      time_at_this_temp = 0;
    }
    //Serial.printf("temp id %d is %.2f %ul\n", snum, in_temp, time_at_this_temp);
    temp = in_temp;
    receive_time_t = DateTime.now();

    if (last_temp_sent == TempSensor::TooLow || fabs(temp - last_temp_sent) > delta) {
      auto diff = temp - last_temp_sent;
      if (diff < 10.0) {
        diff = diff;
      }
      last_temp_sent = temp;
      temp_set_time = DateTime.now();
      changed = true;
      return true;
    } else {
      changed = false;
      return false;
    }
  }
};


