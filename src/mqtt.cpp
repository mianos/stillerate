#include <Arduino.h>
#include <ESPDateTime.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "drow.hpp"
#include "panel.h"

WiFiClient espClient;
PubSubClient client(espClient);

const char *dname = "stillerator";


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
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
#if 1
      StaticJsonDocument<200> doc;
      doc["version"] = 2;
//      doc["run"] = apid.GetMode();
      doc["time"] = DateTime.toISOString();
//      for (auto ii = 0; ii < N_PIDS; ii++) {
//        doc["config"]["servo"] = ii;
//        doc["config"]["sensor"] = input_for_output[ii]; 
//      }
      String status_topic = "tele/" + String(dname) + "/init";
      String output;
      serializeJson(doc, output);
      client.publish(status_topic.c_str(), output.c_str());
      ta("/init ");
      ta(output.c_str());
//			apid.Publish(client, dname, "Reconnect");
#endif
    } else {
      taf("failed, rc=%d, sleeping 5 seconds", client.state());
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
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
  String tele_topic = String("tele/") + dname + "/temp";
  client.publish(tele_topic.c_str(), output.c_str());
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
}
 
const char *mqtt_server = "mqtt2.mianos.com";

void callback(char *topic_str, byte *payload, unsigned int length) {
  auto topic = String(topic_str);

  taf("Message arrived, topic '%s'\n", topic_str);
}

void mqtt_init(int sensor_count) {
  mqtt_sent = new long int[sensor_count];
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

