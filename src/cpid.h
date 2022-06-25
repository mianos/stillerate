#pragma once

#include <QuickPID.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

float *last_values;
const long NANTEMP = -100.0;

class CPid {
  QuickPID pid;
  float Input, Output;
  float Kp=2, Ki=5, Kd=0;
public:
  float Setpoint = 78.4;
  CPid(float outputSpan=100) : pid(&Input, &Output, &Setpoint) {
//    QuickPID myPID(&Input, &Output, &Setpoint);
    //turn the PID on
    pid.SetOutputLimits(0, outputSpan);
    // myPID.SetMode(myPID.Control::automatic); // the PID is turned on
    pid.SetMode(pid.Control::manual); // the PID is turned off
    pid.SetProportionalMode(pid.pMode::pOnError);
    pid.SetDerivativeMode(pid.dMode::dOnMeas);
    pid.SetAntiWindupMode(pid.iAwMode::iAwClamp);
    pid.SetTunings(Kp, Ki, Kd); // set PID gains
    pid.SetControllerDirection(pid.Action::reverse);

  }

  float Calculate(float input) {
    Input = input;
    pid.Compute();
    return Output;
  }

/*  void Compute() {
    pid.Compute();
  }
*/
  void run(bool run) {
      pid.SetMode(run ? pid.Control::automatic : pid.Control::manual);
  }

  bool GetMode() {
    return pid.GetMode() ? true : false;
  }


  void Publish(PubSubClient &client, String dname, String reason) {
		StaticJsonDocument<200> doc;
		doc["number"] = 0;
		doc["input"] = Input;
		doc["output"] = Output;
		doc["kp"] = Kp;
		doc["ki"] = Ki;
		doc["kd"] = Kd;
		doc["setpoint"] = Setpoint;
		doc["reason"] = reason;
		doc["timestamp"] =  DateTime.now();
		String output;
		serializeJson(doc, output);

		String tele_topic = String("tele/") + dname + "/pid";
		client.publish(tele_topic.c_str(), output.c_str());
  }

  void ProcessUpdateJson(DynamicJsonDocument& jpl) {
    if (jpl.containsKey("setpoint")) {
      Serial.printf("setpoint %f\n", jpl["setpoint"].as<float>());
      Setpoint = (double)jpl["setpoint"];
    }
    if (jpl.containsKey("run")) {
      Serial.printf("pid control state %s\n", jpl["run"]  ? "on" : "off");
      run(jpl["run"].as<bool>());
    }
    if (jpl.containsKey("kp")) {
      auto xx = jpl["kp"].as<float>();
      Serial.printf("setting Kp %f\n", xx);
      Kp = xx;
    }
    if (jpl.containsKey("ki")) {
      auto xx = jpl["ki"].as<float>();
      Serial.printf("setting Ki %f\n", xx);
      Ki = xx;
    }
    if (jpl.containsKey("kd")) {
      auto xx = jpl["kd"].as<float>();
      Serial.printf("setting Kd %f\n", xx);
      Kd = xx;
    }
    pid.SetTunings(Kp, Ki, Kd);
  } 
};
