#include <Arduino.h>

#include <ESPDateTime.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>

#include "panel.h"
#include "lwifi.h"

WiFiManager wifiManager;

void configModeCallback (WiFiManager *myWiFiManager) {
  ta("Entered config mode\n");
  char buffer[80];

  sprintf(buffer, "AP IP %s\n", WiFi.softAPIP().toString().c_str());
  ta(buffer);
  sprintf(buffer, "portal ssid %s\n", myWiFiManager->getConfigPortalSSID().c_str());
  ta(buffer);
}

void wifi_connect() {
  ta("Wifi connect\n");
  wifiManager.setAPCallback(configModeCallback);
  auto res = wifiManager.autoConnect("stillcontroller");
  if (!res) {
    ta("Wifi failed to connect\n");
  } else {
    ta("Connected\n");
  }
  DateTime.begin(/* timeout param */);
  if (!DateTime.isTimeValid()) {
    ta("Failed to get time from server\n");
  } else {
    ta("Datetime set\n");
  }
}

