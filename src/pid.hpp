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
  double windupMax = 1000;
  double windupMin = -1000;
  double sampleTime = 10000;
  int   run_state = false;

  PLoop(double output_limit=250) {
    apid.begin(&input, &output, &setpoint, p, i, d);
    set_output(0.0);
    stop();

    // apid.reverse()               // Uncomment if controller output is "reversed"
    apid.setSampleTime(sampleTime);      // OPTIONAL - will ensure at least 10ms have past between successful compute() calls
    apid.setOutputLimits(0, output_limit);
    apid.setBias(0); // 255.0 / 2.0);
    apid.setWindUpLimits(windupMin, windupMax); // Groth bounds for the integral term to prevent integral wind-up
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
    auto old_output = output;
    apid.compute();
    if (output != old_output) {
      Serial.printf("change input %g out %g\n", input, output);
		 apid.debug(&Serial, "myController", PRINT_INPUT    | // Can include or comment out any of these terms to print
                                              PRINT_OUTPUT   | // in the Serial plotter
                                              PRINT_SETPOINT |
                                              PRINT_BIAS     |
                                              PRINT_P        |
                                              PRINT_I        |
                                              PRINT_D);
    }
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
      doc["windup_min"] = windupMin;
      doc["windup_max"] = windupMax;
      doc["sample_time"] = sampleTime;;
  }

  int ProcessUpdateJson(DynamicJsonDocument& jpl) {
    auto changes = 0;
    if (jpl.containsKey("setpoint")) {
     auto sp = (double)jpl["setpoint"];
      if (setpoint != sp) {
        setpoint = sp;
        changes++;
      }
    }
    if (jpl.containsKey("run")) {
      auto run = jpl["run"].as<bool>();
      if (run != apid.getMode()) {
        if (run) {
          start();
        } else {
          stop();
        }
        changes++;
      }
    }
    if (jpl.containsKey("kp")) {
      auto tp = jpl["kp"].as<float>();
      if (p != tp) {
        p = tp;
        apid.setCoefficients(p, i, d);
        changes++;
      }
    }
    if (jpl.containsKey("ki")) {
      auto ti = jpl["ki"].as<float>();
      if (i != ti) {
        i = ti;
        apid.setCoefficients(p, i, d);
				Serial.printf("set to %g\n", i);
        changes++;
      } 
    }
    if (jpl.containsKey("kd")) {
      auto td = jpl["kd"].as<float>();
      if (d != td) {
        d = td;
        apid.setCoefficients(p, i, d);
        changes++;
      }
    }
    if (jpl.containsKey("windup_min")) {
      auto wum = jpl["windup_min"].as<float>();
      if (windupMin != wum) {
        windupMin = wum;
        apid.setWindUpLimits(windupMin, windupMax); 
        changes++;
      }
    }
    if (jpl.containsKey("windup_max")) {
      auto wux = jpl["windup_max"].as<float>();
      if (windupMax != wux) {
        windupMax = wux;
        apid.setWindUpLimits(windupMin, windupMax); 
        changes++;
      }
    }
    if (jpl.containsKey("sample_time")) {
      auto st = jpl["sample_time"].as<float>();
      if (sampleTime != st) {
        sampleTime = st;
        apid.setSampleTime(sampleTime);
        changes++;
      }
    }
    return changes;
  } 
};
