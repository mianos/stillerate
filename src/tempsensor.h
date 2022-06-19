#pragma once

class TempSensor {
public:
  virtual float temp() = 0;
  virtual bool requestTemp() = 0;
};

