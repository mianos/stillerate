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
    this->changed = false;
  }
  bool  Update(const float temp, int snum) {
    if (temp <= TempSensor::TooLow || temp > TempSensor::TooHigh) {
          return false;
    }
    if (this->temp_set_time) {
      this->time_at_this_temp = DateTime.now() - this->temp_set_time;
    } else {
      this->time_at_this_temp = 0;
    }
    // Serial.printf("temp id %d is %.2f %ul\n", snum, temp, this->time_at_this_temp);
    this->temp = temp;
    this->receive_time_t = DateTime.now();

    if (this->last_temp_sent == TempSensor::TooLow || fabs(temp - this->last_temp_sent) > delta) {
      auto diff = temp - this->last_temp_sent;
      if (diff < 10.0) {
        this->diff = diff;
      }
      this->last_temp_sent = temp;
      this->temp_set_time = DateTime.now();
      this->changed = true;
      return true;
    } else {
      this->changed = false;
      return false;
    }
  }
};


