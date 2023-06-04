#include <Arduino.h>

#include <ESPDateTime.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

#include "panel.h"
#include "lwifi.h"

WiFiManager wifiManager;

void configModeCallback (WiFiManager *myWiFiManager) {
  ta("Entered config mode");
  char buffer[80];

  sprintf(buffer, "AP IP %s", WiFi.softAPIP().toString().c_str());
  ta(buffer);
  sprintf(buffer, "portal ssid %s", myWiFiManager->getConfigPortalSSID().c_str());
  ta(buffer);
}

void wifi_connect() {
  ta("Wifi connect");
  wifiManager.setAPCallback(configModeCallback);
  auto res = wifiManager.autoConnect("stillcontroller");
  if (!res) {
    ta("Wifi failed to connect");
  } else {
    ta("Connected");
  }
  DateTime.begin(/* timeout param */);
  if (!DateTime.isTimeValid()) {
    ta("Failed to get time from server");
  } else {
    ta("Datetime set");
  }
}

