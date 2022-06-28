#pragma once
#include "tempsensor.h"
#include "mav.h"

class DallasTemp : public TempSensor {
  int index;
  DallasTemperature &sensors_m;
  uint8_t deviceAddress;
//  CMAverage *mav;
public:
  DallasTemp(DallasTemperature& sensors_a, int index_a) : sensors_m(sensors_a), index(index_a) {
    sensors_m.getAddress(&deviceAddress, index);
//    mav = new CMAverage(4);
  }

  ~DallasTemp() {
//    delete mav;
  }

  bool SetResolution(int resolution) {
    return sensors_m.setResolution(&deviceAddress, resolution);
  }

  bool requestTemp() {
    return sensors_m.requestTemperaturesByAddress(&deviceAddress);
  }

  float temp() {
   return sensors_m.getTempC(&deviceAddress);
//   return mav->avg(sensors_m.getTempC(&deviceAddress));
  }
};

