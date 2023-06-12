#pragma once
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ArduPID.h>

#include "panel.h"

class PidWithError : public ArduPID {
public:
  double getError() {
    return curError;
  }
  bool getMode() {
    return modeType == ON ? true : false;
  }
};

struct PLoop {
  PidWithError  apid;

  double input;
  double output;
  double prev_output;
  double setpoint = 78.4;
  double p = 0.1;
  double i = 0.04;
  double d = 0.0;

  PLoop() {
    apid.begin(&input, &output, &setpoint, p, i, d);
    set_output(0.0);
    stop();

    // apid.reverse()               // Uncomment if controller output is "reversed"
    // apid.setSampleTime(10);      // OPTIONAL - will ensure at least 10ms have past between successful compute() calls
    apid.setOutputLimits(0, 250);
    apid.setBias(255.0 / 2.0);
    apid.setWindUpLimits(-10, 10); // Groth bounds for the integral term to prevent integral wind-up
  }

  bool getMode() {
    return apid.getMode();
  }

  void start() {
    apid.start();
    lv_obj_add_state(ui_pidon, LV_STATE_CHECKED); 
    ta("PID start");
  }

  void stop() {
    apid.stop();
    lv_obj_clear_state(ui_pidon, LV_STATE_CHECKED);
    ta("PID stop");
  }

  double handle(double temp) {
    input = temp;
    apid.compute();
    if (prev_output != output) {
      Serial.printf("change %g to %g\n", output, prev_output);
      prev_output = output;
    }
    return output;
  }

  void set_output(double new_output) {
    output = new_output;
    prev_output = output;
  }

  void ProcessUpdateJson(DynamicJsonDocument& jpl) {
    if (jpl.containsKey("setpoint")) {
      setpoint = (double)jpl["setpoint"];
    }
    if (jpl.containsKey("mode")) {
      auto mode =jpl["mode"].as<bool>();
      if (mode) {
        start();
      } else {
        stop();
      }
    }
    if (jpl.containsKey("kp")) {
      p = jpl["kp"].as<float>();
    }
    if (jpl.containsKey("ki")) {
      i = jpl["ki"].as<float>();
    }
    if (jpl.containsKey("kd")) {
      d = jpl["kd"].as<float>();
    }
  } 
};
