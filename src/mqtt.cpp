#include <Arduino.h>
#include <ESPDateTime.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <StringSplitter.h>

#include "drow.hpp"
#include "panel.h"
#include "cservo.hpp"
#include "pid.hpp"

WiFiClient espClient;
PubSubClient client(espClient);
PLoop *ploop;
static bool publish_settings;

const char *dname = "stillerator";


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    lv_obj_clear_state(ui_pidon, LV_STATE_CHECKED);
    ta("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "stillerator-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {

      String cmnd_topic = String("cmnd/") + dname + "/#";
      client.subscribe(cmnd_topic.c_str());

      ta("mqtt connected");
      // Once connected, publish an announcement...

      StaticJsonDocument<200> doc;
      doc["version"] = 2;
      doc["time"] = DateTime.toISOString();
      String status_topic = "tele/" + String(dname) + "/init";
      String output;
      serializeJson(doc, output);
      client.publish(status_topic.c_str(), output.c_str());
      publish_settings = true;
    } else {
      taf("failed, rc=%d, sleeping 5 seconds", client.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  lv_obj_add_state(ui_mqtton, LV_STATE_CHECKED); 
}



void mqtt_send(bool forced, DRow *dr, int snum) {
  StaticJsonDocument<200> doc;
  doc["sensor"] = snum;
  doc["temp"] = dr->temp;
  doc["timestamp"] = dr->temp_set_time;
  if (forced) {
    doc["forced"] = true;
  } else {
    doc["diff"] = dr->diff;
  }
  String output;
  serializeJson(doc, output);
  String topic = String("tele/") + dname + "/temp";
  client.publish(topic.c_str(), output.c_str());

  if (publish_settings) {
    StaticJsonDocument<200> sdoc;
    ploop->BuildSettingsToSend(sdoc);
    String o2;
    serializeJson(sdoc, o2);
    String t2 = String("tele/") + dname + "/pidconfig";
    client.publish(t2.c_str(), o2.c_str());
    publish_settings = false;
  }
}

void  publish_speed_change(int motor, int new_speed) {
  StaticJsonDocument<200> doc;
  doc["number"] = motor;
  doc["speed"] = new_speed;
  doc["timestamp"] = DateTime.now();
  String output;
  serializeJson(doc, output);
  String topic = String("tele/") + dname + "/pump";
  client.publish(topic.c_str(), output.c_str());
}

time_t *mqtt_sent;

void handle_mqtt(DRow **drows, const int temp_sensor_count) {
  if (!client.connected()) {
    reconnect();
  }
  for (auto snum = 0; snum < temp_sensor_count; snum++) {
    if (drows[snum]->temp <= TempSensor::TooLow || drows[snum]->temp > TempSensor::TooHigh) {
      continue;
    }
    auto time_since_last_mqtt_send = DateTime.now() - mqtt_sent[snum];
    if (time_since_last_mqtt_send > 20) {
      mqtt_send(true, drows[snum], snum);
      mqtt_sent[snum] = DateTime.now();
    } else if (drows[snum]->changed) {
      mqtt_send(false, drows[snum], snum);
      mqtt_sent[snum] = DateTime.now();
    }
  }
  client.loop();
}

void send_pid_info() {
  if (!client.connected()) {
    reconnect();
  }
  static double prev_output;
  if (ploop->getMode() && ploop->output != prev_output) {
    StaticJsonDocument<200> doc;

    doc["input"] = ploop->input;
    doc["output"] = ploop->output;
    doc["error"] = ploop->apid.getError();
    doc["timestamp"] = DateTime.now();
    prev_output = ploop->output;

    String status_topic = "tele/" + String(dname) + "/pid";
    String output;
    serializeJson(doc, output);
    client.publish(status_topic.c_str(), output.c_str());
  }
}
 
const char *mqtt_server = "mqtt2.mianos.com";

void callback(char *topic_str, byte *payload, unsigned int length) {
  //taf("Message arrived, topic '%s'", topic_str);
  auto topic = String(topic_str);

  auto splitter = StringSplitter(topic, '/', 4); // new StringSplitter(string_to_split, delimiter, limit)
  int itemCount = splitter.getItemCount();
  if (itemCount < 3) {
    taf("Item count less than 3 %d '%s'", itemCount, topic_str);
    return;
  }
#if 0
  for (int i = 0; i < itemCount; i++) {
    String item = splitter.getItemAtIndex(i);
    taf("Item '%s' index %d", item.c_str(), i);
  }
#endif
  if (splitter.getItemAtIndex(0) == "cmnd") {
    DynamicJsonDocument jpl(1024);
    auto err = deserializeJson(jpl, payload, length);
    if (err) {
      taf("deserializeJson() failed: '%s'", err.c_str());
    }
#if 0
    else {
      String output;
      serializeJson(jpl, output);
      taf("payload '%s'", output.c_str());
    }
#endif
    auto dest = splitter.getItemAtIndex(2);
    // Serial.printf("dest '%s'\n", dest.c_str());
    if (dest == "temp") {
      if (!jpl.containsKey("number")) {
        ta("emulated temp set does not  not contain a sensor number number");
        return;
      }
      auto number = jpl["number"].as<unsigned int>();
      if (number < 0 || number >= temp_sensor_count) {
        taf("temp no %d invalid", number);
        return;
      }
      if (jpl.containsKey("emulation")) {
        auto emulation = jpl["emulation"].as<bool>();
        temp_sensors[number]->emulationMode(emulation);
        taf("emulation mode %s", emulation ? "on" : "off");
      }
      if (jpl.containsKey("temp")) {
        auto temp = jpl["temp"].as<double>();
        temp_sensors[number]->setEmulatedTemp(temp);
      }
    } else if (dest == "pump") {
      if (!jpl.containsKey("number")) {
        ta("Does not contain a pump number");
        return;
      }
      if (!jpl.containsKey("speed")) {
        ta("Does not contain a pump speed");
        return;
      }
      auto speed = jpl["speed"].as<unsigned int>();
      auto number = jpl["number"].as<unsigned int>();
      set_speed(number, speed);
      if (number == R_MOTOR) {
        ploop->set_output((double)speed);
      }
    } else if (dest == "pid") {
      String output;
      serializeJson(jpl, output);
      Serial.printf("payload '%s'", output.c_str());
      if (ploop->ProcessUpdateJson(jpl)) {
        publish_settings = true;
      }
    } else if (dest = "report") {
        publish_settings = true;
    }
  }
}

void mqtt_init(int sensor_count, PLoop *pid) {
  mqtt_sent = new long int[sensor_count];
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  ploop = pid;
}

