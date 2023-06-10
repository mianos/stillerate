#pragma once



class TempSensor {
  int errors_m = 0;
public:
  static constexpr float TooLow = -150.0;
  static constexpr float TooHigh = 200.0;
  virtual float getTemp() = 0;
  virtual bool requestTemp() = 0;
  virtual const char *getType() const = 0;
};

