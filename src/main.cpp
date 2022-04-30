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
#include <WiFiManager.h> 
#include <StringSplitter.h>

 const char *dname = "stillerator";

Servo myservo; // create servo object to control a servo

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
#define ONE_WIRE_BUS GPIO_NUM_33

// Setup a oneWire instance to communicate with any OneWire devices (not just
// Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int device_count = 0;
DeviceAddress *sensor_addresses;
float *last_values;
const long NANTEMP = -100.0;

int servoPin = 2;
int pos = 0;

const char *mqtt_server = "mqtt2.mianos.com";

WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;

void callback(char *topic_str, byte *payload, unsigned int length) {
  auto topic = String(topic_str);

  Serial.printf("Message arrived, topic '%s'\n", topic_str);
  
  auto splitter = StringSplitter(topic, '/', 3);  // new StringSplitter(string_to_split, delimiter, limit)
  int itemCount = splitter.getItemCount();
  Serial.println("Item count: " + String(itemCount));


  for(int i = 0; i < itemCount; i++){
    String item = splitter.getItemAtIndex(i);
    Serial.println("Item @ index " + String(i) + ": " + String(item));
  }
  DynamicJsonDocument doc(1024);
  auto err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.printf("deserializeJson() failed: '%s'\n", err.c_str());
  } else {
    String output;
    serializeJson(doc, output);
    Serial.printf("payload '%s'\n", output.c_str());
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
#if 0
      String stat_topic = String("stat/") + dname; 
      String tele_topic = String("tele/") + dname;
      client.subscribe(stat_topic.c_str());
      client.subscribe(tele_topic.c_str());
#endif
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

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

  wifiManager.autoConnect("portal");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  DeviceAddress dummyAddr;
	oneWire.search(dummyAddr);
	oneWire.reset_search();

  sensors.begin();
  Serial.print("Locating devices...");
  Serial.print("Found ");
  device_count = sensors.getDeviceCount();
  Serial.printf("count %d ", device_count);
  Serial.println(" devices.");
	sensor_addresses = new DeviceAddress[device_count];
  last_values = new float[device_count];
  for (int ii = 0; ii < device_count; ii++) {
      if (!sensors.getAddress(sensor_addresses[ii], ii)) 
        Serial.printf("Unable to find address for Device %d", ii);
      else
        Serial.printf("got device %d address %d\n", ii, sensor_addresses[ii]);
      sensors.setResolution(sensor_addresses[ii], 12);
      last_values[ii] = NANTEMP;
  }
  // report parasite power requirements
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode())
		Serial.println("ON");
  else
		Serial.println("OFF");

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);           // standard 50 hz servo
  myservo.attach(servoPin, 1000, 2000); //
}

const int period = 1000;
unsigned long last_loop_time;
const int forced_send_period = 30000;
unsigned long last_temp_send_time;

void loop() {
  unsigned long now = millis();

  if (now >= last_loop_time + period) {
    last_loop_time = now;
    if (!client.connected()) {
      reconnect();
    }

    sensors.requestTemperatures();
    for (int snum; snum < device_count; snum++) {
      float tempC = sensors.getTempCByIndex(snum);

      if (tempC != DEVICE_DISCONNECTED_C) {
        float diff = last_values[snum] != NANTEMP ? tempC - last_values[snum] : 0.0;

        if (diff != 0.0) {
          char buff[30];
          snprintf(buff, sizeof(buff), "%1d %0.2f %0.2f", snum, tempC, diff);

          tft.fillScreen(TFT_BLACK);
          tft.drawString(buff, 3, 10 * snum, 0);
        }
        if (client.connected()) {
          auto forced = now > last_temp_send_time + forced_send_period;
          if (last_temp_send_time == 0 ||diff != 0 || !last_temp_send_time || forced) {
            StaticJsonDocument<200> doc;
            doc["sensor"] = snum;
            doc["temp"] = tempC;
            doc["diff"] = diff;

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
        last_values[snum] = tempC;
      } else {
        Serial.println("Error: Could not read temperature data");
      }
  }
  }
  client.loop();
}
