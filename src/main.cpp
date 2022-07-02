#include <Arduino.h>

#include <Button2.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <SPI.h>

//#include "display.h"
//#define USER_SETUP_LOADED

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <Wire.h>

#include <ESPDateTime.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <StringSplitter.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

#include "tempsensor.h"
#include "dallastemp.h"
#include "pt100temp.h"
#include "cservo.h"
#include "cpid.h"

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

#define ONE_WIRE_BUS GPIO_NUM_13
OneWire oneWire_g(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire_g);

TempSensor **temp_sensors;
int temp_sensor_count;


//PT100Temp *pt100_device;

const int N_PIDS = 1;
int input_for_output[N_PIDS];

CPid apid;

const char *mqtt_server = "mqtt2.mianos.com";
const char* ntpServer = "ntp.mianos.com";
WiFiClient espClient;
WiFiManager wifiManager;
PubSubClient client(espClient);

int servo_pins[] = {22, 21};
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
      Serial.printf("payload '%s'\n", output.c_str());
    }
    auto dest = splitter.getItemAtIndex(2);
    Serial.printf("dest '%s'\n", dest.c_str());
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
      servos[number]->set(speed, number);
    } else if (dest == "pid") {
      apid.ProcessUpdateJson(jpl);
    } else if (dest == "config") {
      auto cfg = jpl["config"].as<JsonObject>();
      if (cfg.containsKey("sensor")) {
        Serial.printf("config sensor %d\n", cfg["sensor"].as<int>());    
      }
      if (cfg.containsKey("servo")) {
          Serial.printf("config servo %d\n", cfg["servo"].as<int>());
      }
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

      String cmnd_topic = String("cmnd/") + dname + "/#";
      client.subscribe(cmnd_topic.c_str());

      DateTime.begin(/* timeout param */);
      if (!DateTime.isTimeValid()) {
        Serial.println("Failed to get time from server.");
      } 
      Serial.println("connected");
      // Once connected, publish an announcement...
      StaticJsonDocument<200> doc;
      doc["version"] = 1;
      doc["run"] = apid.GetMode();
      doc["time"] = DateTime.toISOString();
      for (auto ii = 0; ii < N_PIDS; ii++) {
        doc["config"]["servo"] = ii;
        doc["config"]["sensor"] = input_for_output[ii]; 
      }
      String status_topic = "tele/" + String(dname) + "/init";
      String output;
      serializeJson(doc, output);
      client.publish(status_topic.c_str(), output.c_str());
      Serial.printf("/init %s\n", output.c_str());
			apid.Publish(client, dname, "Reconnect");
 
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
  time_t receive_time_t;
public:
  DRow() : changed(false), temp(0.0), last_temp(NANTEMP), diff(0.0) {}
};

DRow **drows;

void setup() {
  Serial.begin(115200);
  Serial.println("Start");

  DateTime.setTimeZone("AEST-10AEDT,M10.1.0,M4.1.0/3");
  tzset();

#if 0
	ArduinoOTA
	.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
#endif
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

  auto res = wifiManager.autoConnect("portal");
  if (!res) {
    Serial.printf("Failed to connect");
  }
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  DeviceAddress dummyAddr;
  oneWire_g.search(dummyAddr);
  oneWire_g.reset_search();
  sensors.begin();
  auto dallas_count = sensors.getDeviceCount();

  temp_sensor_count = dallas_count + 1;
  temp_sensors = new TempSensor *[temp_sensor_count];

  int tx = 0;
  for (tx = 0; tx < dallas_count; tx++) {
    temp_sensors[tx] = new DallasTemp(sensors, tx);
  }
  temp_sensors[tx] = new PT100Temp();

  for (auto ii = 0; ii < temp_sensor_count; ii++) {
    Serial.printf("sensor %d is %s\n", ii, temp_sensors[ii]->getType());
  }
  drows = new DRow *[temp_sensor_count];
  for (auto ii = 0; ii < temp_sensor_count; ii++) {
    drows[ii] = new DRow();
  }

  servos = new CServo *[num_servos];
  for (auto ii = 0; ii < num_servos; ii++) {
    servos[ii] = new CServo(servo_pins[ii], dname, client);
  }
  for (auto ii = 0; ii < N_PIDS; ii++) {
    // note, all outputs are driven by input 1
    input_for_output[ii] = 1;
  }
}

const int period = 100;
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

    for (int snum = 0; snum < temp_sensor_count; snum++) {
        temp_sensors[snum]->requestTemp();
    }
    for (int snum = 0; snum < temp_sensor_count; snum++) {
      float temp = temp_sensors[snum]->temp();
      if (temp != DEVICE_DISCONNECTED_C) {
        float diff = drows[snum]->last_temp != NANTEMP
                         ? temp - drows[snum]->last_temp
                         : 0.0;
        //Serial.printf("sensor %d temp %f diff %f\n", snum, temp, diff);
        if (fabs(diff) > 0.01) {
          drows[snum]->changed = true;
          drows[snum]->temp = temp;
          drows[snum]->diff = diff;
          drows[snum]->receive_time_t = DateTime.now();
          changes = true;
        }
      } else {
        Serial.printf("sensor %d failed  %f\n", snum, temp);
      }
    }
    if (changes) {
      tft.fillScreen(TFT_BLACK);
      for (int snum = 0; snum < temp_sensor_count; snum++) {
        char buff[30];
        snprintf(buff, sizeof(buff), "%1d %0.2f %0.2f", snum, drows[snum]->temp,
                 drows[snum]->diff);

        tft.drawString(buff, 3, 35 * snum, 0);
        // Serial.printf("%s\n", buff);
      }
      tft.drawString(DateTime.format(DateFormatter::TIME_ONLY), 3, 70, 0);
    }
    if (client.connected()) {
      for (int snum = 0; snum < temp_sensor_count; snum++) {
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
          doc["timestamp"] = drows[snum]->receive_time_t;
          if (forced) {
            doc["forced"] = true;
          } else {
            doc["diff"] = drows[snum]->diff;
          }
          String output;
          serializeJson(doc, output);
          String tele_topic = String("tele/") + dname + "/temp";
          client.publish(tele_topic.c_str(), output.c_str());
          last_temp_send_time = now;
          Serial.printf("%s\n", output.c_str());
        }
      }
    }
    for (int snum = 0; snum < temp_sensor_count; snum++) {
      if (drows[snum]->temp != NANTEMP) {
        drows[snum]->last_temp = drows[snum]->temp;
      }
      drows[snum]->changed = false;
    }
  }
  for (int snum = 0; snum < temp_sensor_count; snum++) {
    for (auto outnum = 0; outnum <  N_PIDS; outnum++) {
      if (input_for_output[outnum] == snum) {
        auto nval = apid.Calculate(drows[snum]->temp);

        // if automatic pid or tuning set the output
        if (apid.GetMode()) {
          if (servos[outnum]->set(nval, snum)) {
            apid.Publish(client, dname, "Speed change");
          }
        }
      }
    }
  }
  for (int servo = 0; servo < num_servos; servo++) {
    servos[servo]->do_update_if_needed(servo);
  }
  client.loop();
//  ArduinoOTA.handle();
}
