#pragma once
#include "panel.h"

class CServo {
  int speed = 0;
  const char *dname;
  int pin;
  int minimum;
  int snum;
  bool inverse;
  static constexpr int max_speed = 255;
public:

  bool set_speed(int new_speed) {
    if (new_speed != speed) {
      auto tw = this->inverse ? max_speed - new_speed : new_speed;
      taf("servo %d to raw %d", this->snum, tw);
      analogWrite(pin, tw);
      speed = new_speed;
      return true;
    } else {
      return false;
    }
  }

  CServo(int pin, int minimum, int snum, bool inverse=true) : pin(pin), minimum(minimum), snum(snum), inverse(inverse) {
    pinMode(pin, OUTPUT);
    if (inverse) {
      set_speed(max_speed);
    } else {
      set_speed(0);
    }
  }

  void raw_full_speed() {
    set_speed(255);
  }

  void raw_full_stop() {
    set_speed(0);
  }
};

extern void servo_init();
extern int num_servos();
extern void set_speed(int snum, int speed);
