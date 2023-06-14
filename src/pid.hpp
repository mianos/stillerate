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
  double setpoint = 78.4;
  double p = 0.1;
  double i = 0.04;
  double d = 0.0;

  PLoop(double output_limit=250) {
    apid.begin(&input, &output, &setpoint, p, i, d);
    set_output(0.0);
    stop();

    // apid.reverse()               // Uncomment if controller output is "reversed"
    // apid.setSampleTime(10);      // OPTIONAL - will ensure at least 10ms have past between successful compute() calls
    apid.setOutputLimits(0, output_limit);
    apid.setBias(0); // 255.0 / 2.0);
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
    return output;
  }

  void set_output(double new_output) {
    output = new_output;
  }

  void BuildSettingsToSend(StaticJsonDocument<200>& doc) {
      doc["run"] = apid.getMode();
      doc["setpoint"] = setpoint;
      doc["kp"] = p;
      doc["ki"] = i;
      doc["kd"] = d;
  }

  bool ProcessUpdateJson(DynamicJsonDocument& jpl) {
    if (jpl.containsKey("setpoint")) {
      setpoint = (double)jpl["setpoint"];
      taf("setpoint %g", setpoint);
    }
    if (jpl.containsKey("run")) {
      auto run = jpl["run"].as<bool>();
      if (run == apid.getMode()) {
        return false;
      }
      if (run) {
        start();
      } else {
        stop();
      }
    }
    if (jpl.containsKey("kp")) {
      auto tp = jpl["kp"].as<float>();
      if (p == tp) {
        return false;
      }
      p = tp;
      taf("set p to %g", p);
    }
    if (jpl.containsKey("ki")) {
      auto ti = jpl["ki"].as<float>();
      if (i == ti) {
        return false;
      }
      i = ti;
      taf("set i to %g", i);
    }
    if (jpl.containsKey("kd")) {
      auto td = jpl["kd"].as<float>();
      if (d == td) {
        return false;
      }
      d = td;
      taf("set d to %g", d);
    }
    return true;
  } 
};
