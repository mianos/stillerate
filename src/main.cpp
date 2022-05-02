#include <Arduino.h>

#include <Button2.h>
#include <DallasTemperature.h>
#include <ESP32Servo.h>
#include <OneWire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <Wire.h>

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <StringSplitter.h>
#include <WiFiManager.h>

const char *dname = "stillerator";

#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_LARGE "NotoSansBold36"

// Font files are stored in SPIFFS, so load the library
#include <FS.h>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN 0x10
#endif

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

#define ADC_EN 14
#define ADC_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0

// Data wire is plugged into port 2 on the Arduino
//#define ONE_WIRE_BUS GPIO_NUM_33
#define ONE_WIRE_BUS GPIO_NUM_13

// Setup a oneWire instance to communicate with any OneWire devices (not just
// Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int device_count = 0;
DeviceAddress *sensor_addresses;
float *last_values;
const long NANTEMP = -100.0;


const char *mqtt_server = "mqtt2.mianos.com";
WiFiClient espClient;
WiFiManager wifiManager;
PubSubClient client(espClient);

class CServo : private Servo {
  int speed;
  bool do_update;
public:
  CServo(int pin) : speed(90), do_update(true) {
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
  void set(int new_speed) {
    write(new_speed);
    speed = new_speed;
    do_update = true;
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

int servo_pins[] = {26, 27};
CServo **servos;
const int num_servos = sizeof(servo_pins) / sizeof(int);

void callback(char *topic_str, byte *payload, unsigned int length) {
  auto topic = String(topic_str);

  //Serial.printf("Message arrived, topic '%s'\n", topic_str);

  auto splitter = StringSplitter(
      topic, '/', 4); // new StringSplitter(string_to_split, delimiter, limit)
  int itemCount = splitter.getItemCount();
  if (itemCount < 3) {
    Serial.printf("Item count less than 3 %d '%s'", itemCount, topic_str);
    return;
  }

#if 0
  for (int i = 0; i < itemCount; i++) {
    String item = splitter.getItemAtIndex(i);
    Serial.println("Item @ index " + String(i) + ": " + String(item));
  }
#endif
  if (splitter.getItemAtIndex(0) == "cmnd") {
    DynamicJsonDocument jpl(1024);
    auto err = deserializeJson(jpl, payload, length);
    if (err) {
      Serial.printf("deserializeJson() failed: '%s'\n", err.c_str());
    } else {
      String output;
      serializeJson(jpl, output);
      //Serial.printf("payload '%s'\n", output.c_str());
    }
    auto dest = splitter.getItemAtIndex(2);
    if (dest == "pump") {
      if (!jpl.containsKey("number")) {
        Serial.printf("Does not contain a pump number");
        return;
      }
      if (!jpl.containsKey("speed")) {
        Serial.printf("Does not contain a pump speed");
        return;
      }
      auto speed = jpl["speed"].as<unsigned int>();
      auto number = jpl["number"].as<unsigned int>();
      if (number < 0 || number > num_servos) {
        Serial.printf("Invalid pump number %d", number);
        return;
      }
      if (speed < 0) { //  || speed > 100) {
        Serial.printf("Invalid speed number %ld", speed);
        return;
      }
      servos[number]->set(speed);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "stillerator-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      String status_topic = String(dname) + "/status";
      client.publish(status_topic.c_str(), "starting");

      String cmnd_topic = String("cmnd/") + dname + "/#";
      client.subscribe(cmnd_topic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

struct DRow {
  bool changed;
  float temp;
  float last_temp;
  float diff;
  unsigned long last_temp_send_time;
public:
  DRow() : changed(false), temp(0.0), last_temp(NANTEMP), diff(0.0) {}
};
// device_count
DRow **drows;

void setup() {
  Serial.begin(115200);
  Serial.println("Start");

  tft.init();
  tft.fontHeight(4);
  tft.setRotation(1);
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nSPIFFS available!");

  // ESP32 will crash if any of the fonts are missing
  bool font_missing = false;
  if (SPIFFS.exists("/NotoSansBold15.vlw") == false)
    font_missing = true;
  if (SPIFFS.exists("/NotoSansBold36.vlw") == false)
    font_missing = true;

  if (font_missing) {
    Serial.println("\r\nFont missing in SPIFFS, did you upload it?");
    while (1)
      yield();
  }
  tft.loadFont(AA_FONT_LARGE); // Must load the font first
  tft.fillScreen(TFT_BLACK);

  wifiManager.autoConnect("portal");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  DeviceAddress dummyAddr;
  oneWire.search(dummyAddr);
  oneWire.reset_search();

  sensors.begin();
  Serial.print("Locating devices...");
  device_count = sensors.getDeviceCount();
  Serial.printf("count %d devices\n", device_count);
  sensor_addresses = new DeviceAddress[device_count];
  drows = new DRow *[device_count];
  for (int ii = 0; ii < device_count; ii++) {
    if (!sensors.getAddress(sensor_addresses[ii], ii))
      Serial.printf("Unable to find address for Device %d", ii);
    else
      Serial.printf("got device %d address %d\n", ii, sensor_addresses[ii]);
    sensors.setResolution(sensor_addresses[ii], 12);
    drows[ii] = new DRow();
  }
  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode())
    Serial.println("ON");
  else
    Serial.println("OFF");

  servos = new CServo *[num_servos];
  for (auto ii = 0; ii < num_servos; ii++) {
    servos[ii] = new CServo(servo_pins[ii]);
  }
}

const int period = 2000;
unsigned long last_loop_time;
const int forced_send_period = 30000;

void loop() {
  unsigned long now = millis();

  if (now >= last_loop_time + period) {
    last_loop_time = now;
    if (!client.connected()) {
      reconnect();
    }
    bool changes = false;
    sensors.requestTemperatures();
    for (int snum = 0; snum < device_count; snum++) {
      float temp = sensors.getTempCByIndex(snum);

      if (temp != DEVICE_DISCONNECTED_C) {
        float diff = drows[snum]->last_temp != NANTEMP
                         ? temp - drows[snum]->last_temp
                         : 0.0;
        // Serial.printf("sensor %d temp %f diff %f\n", snum, temp, diff);
        if (diff != 0.0) {
          drows[snum]->changed = true;
          drows[snum]->temp = temp;
          drows[snum]->diff = diff;
          changes = true;
        }
      } else {
        Serial.printf("sensor %d failed  %f\n", snum, temp);
      }
    }
    if (changes) {
      tft.fillScreen(TFT_BLACK);
      for (int snum = 0; snum < device_count; snum++) {
        char buff[30];
        snprintf(buff, sizeof(buff), "%1d %0.2f %0.2f", snum, drows[snum]->temp,
                 drows[snum]->diff);

        tft.drawString(buff, 3, 35 * snum, 0);
        // Serial.printf("%s\n", buff);
      }
    }
    if (client.connected()) {
      for (int snum = 0; snum < device_count; snum++) {
        if (drows[snum]->temp == NANTEMP) {
          continue;
        }
        auto &last_temp_send_time = drows[snum]->last_temp_send_time;
        auto forced = now > last_temp_send_time + forced_send_period;
        if (last_temp_send_time == 0 || drows[snum]->changed ||
            !last_temp_send_time || forced) {
          StaticJsonDocument<200> doc;
          doc["sensor"] = snum;
          doc["temp"] = drows[snum]->temp;
          doc["diff"] = drows[snum]->diff;

          if (forced) {
            doc["forced"] = true;
          }
          String output;
          serializeJson(doc, output);
          String tele_topic = String("tele/") + dname + "/temp";
          client.publish(tele_topic.c_str(), output.c_str());
          last_temp_send_time = now;
        }
      }
      for (int servo = 0; servo < num_servos; servo++) {
        servos[servo]->do_update_if_needed(servo);
      }
    }
    for (int snum = 0; snum < device_count; snum++) {
      if (drows[snum]->temp != NANTEMP)
        drows[snum]->last_temp = drows[snum]->temp;
      drows[snum]->changed = false;
    }
  }
  client.loop();
}
