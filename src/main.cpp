#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <WiFi.h>
#include <Wire.h>
#include <Button2.h>
#include <ESP32Servo.h> 

#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>



Servo myservo;  // create servo object to control a servo


#define AA_FONT_SMALL "NotoSansBold15"
#define AA_FONT_LARGE "NotoSansBold36"

// Font files are stored in SPIFFS, so load the library
#include <FS.h>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

#define ADC_EN          14
#define ADC_PIN         34
#define BUTTON_1        35
#define BUTTON_2        0


// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS GPIO_NUM_33

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

int servoPin = 2;
int pos = 0; 

const char* mqtt_server = "mqtt2.mianos.com";

WiFiClient espClient;
PubSubClient client(espClient);


WiFiManager wifiManager;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
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
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
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
    while(1) 
      yield();
  } else {
    Serial.println("\r\nFonts found OK.");
  }
  tft.loadFont(AA_FONT_LARGE); // Must load the font first
 
  wifiManager.autoConnect("portal");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  sensors.begin();

	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);    // standard 50 hz servo
	myservo.attach(servoPin, 1000, 2000); //
}

double last_value = 0.0;

void loop()
{
  sensors.requestTemperatures(); // Send the command to get temperatures
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float tempC = sensors.getTempCByIndex(0);
  char buff[30];
  // Check if reading was successful
  if (tempC != DEVICE_DISCONNECTED_C) {
//    Serial.print("Temperature for the device 1 (index 0) is: ");
//    Serial.println(tempC);

//    tft.drawFloat(tempC, tft.width()/4, tft.height() / 2, 4);
    char symbol = ' ';
    if(last_value != 0.0) {
        if (tempC > last_value) {
            symbol = '+';  
        } else {
            symbol = '-';
        }
    }
    snprintf(buff, sizeof(buff), "%0.3f %c", tempC, symbol);
    tft.fillScreen(TFT_BLACK);
    tft.drawString(buff, 3, 0, 0);
    if (client.connected()) {
      client.publish("stillerator/status/temp0", buff);
    }
    last_value = tempC;


  //  tft.fillScreen(TFT_BLACK);
  } else {
    Serial.println("Error: Could not read temperature data");
  }
//  mqttClient.loop();
//  espDelay(1000);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
delay(3000);
}


