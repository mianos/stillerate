#pragma once

class TempSensor {
public:
  static constexpr float TooLow = -150.0;
  static constexpr float TooHigh = 200.0;
protected:
  int errors_m = 0;
  bool emulation_mode = false;
  double emulated_temp = TooLow;
public:
  virtual float getTemp() = 0;
  virtual bool requestTemp() = 0;
  virtual const char *getType() const = 0;
  void emulationMode(bool state=true) {
    emulation_mode = state;
  }
  void setEmulatedTemp(double temp) {
    emulated_temp = temp;
  }
  virtual double temp() {
    if (emulation_mode) {
      return emulated_temp;
    } else {
      return getTemp();
    }
  }
};

extern TempSensor **temp_sensors;
extern int temp_sensor_count;
