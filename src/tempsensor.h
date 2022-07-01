#pragma once
#include "mav.h"

class TempSensor {
  CMAverage<float> mav{20};
public:
  virtual float getTemp() = 0;
  float temp() {
    return mav.avg(getTemp());
  }
  virtual bool requestTemp() = 0;
};

