#pragma once
#include "panel.h"

class CServo {
  int speed = 0;
  const char *dname;
  int pin;
  int minimum;
  int snum;
public:

  bool set_speed(int new_speed) {
    if (new_speed != speed) {
      auto tw = map(new_speed, 0, 255, minimum, 255);
      taf("servo %d to raw %d", this->snum, tw);
      analogWrite(pin, tw);
      speed = new_speed;
      return true;
    } else {
      return false;
    }
  }

  CServo(int pin, int minimum, int snum) : pin(pin), minimum(minimum), snum(snum) {
    pinMode(pin, OUTPUT);
    set_speed(speed);
  }

  void full_speed() {
    set_speed(255);
  }

  void full_stop() {
    set_speed(0);
  }
};

extern void servo_init();
extern int num_servos();
extern void set_speed(int snum, int speed);
