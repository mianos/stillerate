#include <Arduino.h>
#include <ESPDateTime.h>

#include <lvgl.h>

#include "ui.h"
#include "panel.h"
#include "lwifi.h"

static unsigned long lastUpdate = 0; // to store the time of the last update

void setup()
{
  Serial.begin(115200);
  initLVGL();
  DateTime.setTimeZone("AEST-10AEDT,M10.1.0,M4.1.0/3");
  tzset();
  wifi_connect();
}

void SetTimes() {
	auto minutes = millis() / 60000; // Convert milliseconds to minutes
	auto seconds = (millis() % 60000) / 1000; // Calculate the remainder seconds

	char buffer[20];
	sprintf(buffer, "%02d:%02d", minutes, seconds);
	lv_label_set_text_fmt(ui_elapsed, buffer);
	lv_label_set_text_fmt(ui_clock, DateTime.format(DateFormatter::TIME_ONLY).c_str());
}

void loop() {
	if (millis() - lastUpdate > 100) { // If 5000ms have passed
      SetTimes();
			lastUpdate = millis();
	}
	lv_task_handler();
	delay(5);
}

