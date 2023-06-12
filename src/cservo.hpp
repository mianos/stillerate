#pragma once
#include "panel.h"
#include "ui.h"

constexpr int R_MOTOR = 0;

class CServo {
public:
  static constexpr int max_speed = 255;
private:
  int speed = 255;    // initialise to max to it gets changed to 0 and displayed
  const char *dname;
  int pin;
  int minimum;
  int snum;
  bool inverse;
public:

  bool set_speed(int new_speed) {
    if (new_speed != speed) {
      if (snum == R_MOTOR) {
        char buffer[20];
        sprintf(buffer, "%d", new_speed);
        lv_label_set_text(ui_rspeed, buffer);
      }
      auto tw = this->inverse ? max_speed - new_speed : new_speed;
      analogWrite(pin, tw);
      speed = new_speed;
      return true;
    } else {
      return false;
    }
  }

  CServo(int pin, int minimum, int snum, int initial_speed, bool inverse=true) : pin(pin), minimum(minimum), snum(snum), inverse(inverse) {
    pinMode(pin, OUTPUT);
    set_speed(initial_speed);
  }

  void raw_full_speed() {
    set_speed(max_speed);
  }

  void raw_full_stop() {
    set_speed(0);
  }
};

extern void servo_init(int initial_speed);
extern int num_servos();
extern void set_speed(int snum, int speed);
