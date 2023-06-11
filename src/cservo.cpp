#include <Arduino.h>

#include "cservo.hpp"

int servo_pins[] = {22, 21};
CServo **servos;
constexpr int NUM_SERVOS = sizeof(servo_pins) / sizeof(int);

int num_servos() {
  return NUM_SERVOS;
}

void servo_init() {
  servos = new CServo *[NUM_SERVOS];
  for (auto ii = 0; ii < NUM_SERVOS; ii++) {
    // 10 is the minimum servo value
    servos[ii] = new CServo(servo_pins[ii], 10, ii);
  }
}


void set_speed(int snum, int speed) {
  if (snum < 0 || snum >= NUM_SERVOS) {
    taf("Invalid pump number %d", snum);
    return;
  }
  if (speed < 0) {
    taf("Invalid speed snum %d", speed);
    return;
  }
  servos[snum]->set_speed(speed);
}
