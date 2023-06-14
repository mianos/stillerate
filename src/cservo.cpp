#include <Arduino.h>

#include "cservo.hpp"

int servo_pins[] = {22, 21};
CServo **servos;
constexpr int NUM_SERVOS = sizeof(servo_pins) / sizeof(int);

int num_servos() {
  return NUM_SERVOS;
}

void servo_init(int initial_speed) {
  servos = new CServo *[NUM_SERVOS];
  for (auto ii = 0; ii < NUM_SERVOS; ii++) {
    // 10 is the minimum servo value
    if (ii != R_MOTOR) {
      initial_speed = CServo::max_speed;
    }
    servos[ii] = new CServo(servo_pins[ii], 10, ii, initial_speed);
  }
}


bool set_speed(int snum, int speed) {
  if (snum < 0 || snum >= NUM_SERVOS) {
    taf("Invalid pump number %d", snum);
    return false;
  }
  if (speed < 0) {
    taf("Invalid speed snum %d", speed);
    return false;
  }
  return servos[snum]->set_speed(speed);
}
