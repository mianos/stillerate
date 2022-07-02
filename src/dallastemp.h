#pragma once
#include "tempsensor.h"

class DallasTemp : public TempSensor {
  int index;
  DallasTemperature &sensors_m;
  uint8_t deviceAddress;
//  CMAverage<float> *mav;
public:
  DallasTemp(DallasTemperature& sensors_a, int index_a) : sensors_m(sensors_a), index(index_a) {
    sensors_m.getAddress(&deviceAddress, index);
  }

  bool SetResolution(int resolution) {
    return sensors_m.setResolution(&deviceAddress, resolution);
  }

  bool requestTemp() {
    return sensors_m.requestTemperaturesByAddress(&deviceAddress);
  }

  float getTemp() {
    return sensors_m.getTempC(&deviceAddress);
  }
  virtual const char *getType() const {
    return "ds18b20";
  }
};

