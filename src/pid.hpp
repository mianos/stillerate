#pragma once
#include "ArduPID.h"

#include "drow.hpp"

struct PLoop {
  ArduPID apid;

  double input;
  double output;
  double setpoint = 78.4;
  double p = 0.1;
  double i = 0.04;
  double d = 0.0;
  DRow *drow;

  PLoop(DRow *drow) : drow(drow) {
    apid.begin(&input, &output, &setpoint, p, i, d);

    // apid.reverse()               // Uncomment if controller output is "reversed"
    // apid.setSampleTime(10);      // OPTIONAL - will ensure at least 10ms have past between successful compute() calls
    apid.setOutputLimits(0, 250);
    apid.setBias(255.0 / 2.0);
    apid.setWindUpLimits(-10, 10); // Groth bounds for the integral term to prevent integral wind-up
    apid.start();
  }

  void start() {
    apid.start();
  }

  void stop() {
    apid.stop();
  }

  double handle() {
    if (!drow->isValid()) {
      return output;
    }
    input = drow->temp;
    apid.compute();
    return output;
  }
};
