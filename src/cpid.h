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
  uint32_t sampletime = 1;
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
		doc["sampletime"] = sampletime;
		doc["reason"] = reason;
		doc["timestamp"] =  DateTime.now();
		String output;
		serializeJson(doc, output);

		String tele_topic = String("tele/") + dname + "/pid";
		client.publish(tele_topic.c_str(), output.c_str());
  }

  void ProcessUpdateJson(DynamicJsonDocument& jpl) {
    if (jpl.containsKey("setpoint")) {
      Setpoint = (double)jpl["setpoint"];
    }
    if (jpl.containsKey("run")) {
      run(jpl["run"].as<bool>());
    }
    if (jpl.containsKey("kp")) {
      auto xx = jpl["kp"].as<float>();
      Kp = xx;
      pid.SetTunings(Kp, Ki, Kd);
    }
    if (jpl.containsKey("ki")) {
      auto xx = jpl["ki"].as<float>();
      Ki = xx;
      pid.SetTunings(Kp, Ki, Kd);
    }
    if (jpl.containsKey("kd")) {
      auto xx = jpl["kd"].as<float>();
      Kd = xx;
      pid.SetTunings(Kp, Ki, Kd);
    }
    if (jpl.containsKey("sampletime")) {
      sampletime = jpl["sampletime"].as<int>();
      pid.SetSampleTimeUs(sampletime * 1000000);
    }
  } 
};
