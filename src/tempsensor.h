#pragma once
#include "mav.h"

class TempSensor {
  CMAverage<float> mav{10};
  int errors_m = 0;
public:
  static constexpr float TooLow = -100.0;
  virtual float getTemp() = 0;
  float temp() {
    auto temp = getTemp();
    if (temp > TooLow) {
      return mav.avg(temp);
    } else {
      errors_m++;
      return TooLow;
    }
  }
  int  errors() {
    return errors_m;
  }
  virtual bool requestTemp() = 0;
  virtual const char *getType() const = 0;
};

