#pragma once
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

class CServo : private Servo {
  int speed; 
  bool do_update;
  const char *dname;
  PubSubClient& client;
public:
  CServo(int pin, const char *dname_a, PubSubClient& client_a) : dname(dname_a), client(client_a), speed(0), do_update(true) {
    static bool timers_allocated = false;
    if (!timers_allocated) {
      for (auto ii = 0; ii < 4; ii++) {
        ESP32PWM::allocateTimer(ii);
      }
      timers_allocated = true;
    }
    setPeriodHertz(50);      // standard 50 hz servo
    attach(pin, 1000, 2000); //
    write(speed);
  }

  bool set(int new_speed, int snum) {
    if (new_speed != speed) {
      write(map(new_speed, 0, 100, 90, 180));
      speed = new_speed;
      do_update = true;
      return true;
    } else {
      return false;
    }
  }

  void do_update_if_needed(int snum) {
    if (do_update == false) {
      return;
    }
    StaticJsonDocument<200> doc;

    doc["number"] = snum;
    doc["speed"] = speed;

    String output;
    serializeJson(doc, output);
    String status_topic = "tele/" + String(dname) + "/pump";
    client.publish(status_topic.c_str(), output.c_str());
    do_update = false;
  }
};

