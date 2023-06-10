#pragma once
#include "tempsensor.h"

class DallasTemp : public TempSensor {
  int index;
  DallasTemperature &sensors_m;
  uint8_t deviceAddress;
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
    auto tempC = sensors_m.getTempC(&deviceAddress);
    if (tempC < -100.0) {
      return TooLow;
    } else {
      return tempC;
    }
  }
  virtual const char *getType() const {
    return "ds18b20";
  }
};

