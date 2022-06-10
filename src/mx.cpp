#include <Arduino.h>
#include <ArduinoOTA.h>
#include <SPI.h>

#include <WiFiManager.h>
#include <ESPDateTime.h>
#include "MAX31865.h"

WiFiManager wifiManager;



MAX31865_RTD *rtd;

#define HSPI_MISO   26
#define HSPI_MOSI   27
#define HSPI_SCLK   25
#define HSPI_SS     33

static const int spiClk = 1000000; // 10khz // 1 MHz

SPIClass * hspi = NULL;

void setup() {
    Serial.begin(115200);
  Serial.println("Start");

  //wifiManager.setHostname("pt100");
  auto res = wifiManager.autoConnect("portal");
  //wifiManager.setHostname("pt100");
  if (!res) {
    Serial.printf("Failed to connect");
  }

  Serial.printf("connected");
  //WiFi.disconnect(false,true);

  
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
  DateTime.setTimeZone("AEST-10AEDT,M10.1.0,M4.1.0/3");
  tzset();
  DateTime.begin();
  if (!DateTime.isTimeValid()) {
    Serial.println("Failed to get time from server.");
  } else {
    Serial.printf("Date Now is %s\n", DateTime.toISOString().c_str());
    Serial.printf("Timestamp is %ld\n", DateTime.now());
  }


  hspi = new SPIClass(VSPI);

  hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS); //SCLK, MISO, MOSI, SS
  auto *spis = new SPISettings(spiClk, MSBFIRST, SPI_MODE3);
  rtd = new MAX31865_RTD(MAX31865_RTD::RTD_PT100, HSPI_SS, hspi, spis);

    rtd->configure(true, true, false, true, MAX31865_FAULT_DETECTION_NONE,
                   true, true, 0x0000, 0x7fff);
}


void loop() {
  ArduinoOTA.handle();

  rtd->read_all();
//  Serial.printf("Raw %g (%d)\n",  rtd->temperature(), rtd->status());
  Serial.printf("%s, %g\n", DateTime.toISOString().c_str(), rtd->temperature());
  // Serial.printf("Timestamp is %ld\n", DateTime.now());
  sleep(1);
}
