#include <Arduino.h>
#include <ESPDateTime.h>
#include <Wire.h>
#include <DallasTemperature.h>

#include <lvgl.h>

#include "ui.h"
#include "panel.h"
#include "lwifi.h"
#include "tempsensor.h"
#include "dallastemp.h"
#include "pt100temp.h"
#include "drow.hpp"
#include "mqtt.hpp"


#define ONE_WIRE_BUS GPIO_NUM_13
OneWire oneWire_g(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire_g);


static const int max_temp_count = 2;
TempSensor **temp_sensors;
int temp_sensor_count;
DRow **drows;

void init_temp_sensors() {
  DeviceAddress dummyAddr;
  oneWire_g.search(dummyAddr);
  oneWire_g.reset_search();

  sensors.begin();
  auto dallas_count = sensors.getDeviceCount();

  temp_sensors = new TempSensor *[max_temp_count];
  for (auto tx = 0; tx < dallas_count; tx++) {
    if (temp_sensor_count > max_temp_count) {
      ta("max sensor count exceeded");
      return;
    }
    temp_sensors[temp_sensor_count++] = new DallasTemp(sensors, tx);
  }
  if (temp_sensor_count > max_temp_count) {
    ta("max sensor count exceeded");
    return;
  }
  temp_sensors[temp_sensor_count++] = new PT100Temp();

  drows = new DRow *[temp_sensor_count];
  for (auto ii = 0; ii < temp_sensor_count; ii++) {
    drows[ii] = new DRow();
  }
}

void setup()
{
  Serial.begin(115200);
  initLVGL();
  DateTime.setTimeZone("AEST-10AEDT,M10.1.0,M4.1.0/3");
  tzset();
  wifi_connect();
  init_temp_sensors();
}

void SetTimes() {
	auto minutes = millis() / 60000; // Convert milliseconds to minutes
	auto seconds = (millis() % 60000) / 1000; // Calculate the remainder seconds

	char buffer[20];
	sprintf(buffer, "%02d:%02d", minutes, seconds);
	lv_label_set_text_fmt(ui_elapsed, buffer);
	lv_label_set_text_fmt(ui_clock, DateTime.format(DateFormatter::TIME_ONLY).c_str());
}

static unsigned long lastUpdate;
static unsigned int lcount;

void display() {
    char temp_b[10];
    char diff_b[10];
    char time_at_temp_b[10];
    for (auto snum = 0; snum < temp_sensor_count; snum++) {
      if (drows[snum]->temp == TempSensor::TooLow) {
        continue;
      }
#if 0
      Serial.printf("change sensor %d temp %.2f diff %+.2f\n", 
          snum,
          drows[snum]->temp,
          drows[snum]->diff);
#endif
      sprintf(temp_b, "%.2f", drows[snum]->temp);
      sprintf(diff_b, "%+.2f", drows[snum]->diff);
      sprintf(time_at_temp_b, "%lu", drows[snum]->time_at_this_temp);
      switch (snum) {
      case 0:
        lv_label_set_text(ui_btemp, temp_b);
        lv_label_set_text(ui_bdiff, diff_b);
        lv_label_set_text(ui_btime, time_at_temp_b);
        break;
      case 1:
        lv_label_set_text(ui_rtemp, temp_b);
        lv_label_set_text(ui_rdiff, diff_b);
        lv_label_set_text(ui_rtime, time_at_temp_b);
        break;
      }
    }
}
void loop() {
	if (millis() - lastUpdate > 100) {
      SetTimes();

      auto changes = false;
      for (auto snum = 0; snum < temp_sensor_count; snum++) {
        if (!(lcount & 1)) {
          temp_sensors[snum]->requestTemp();
        } else {
          changes = drows[snum]->Update(temp_sensors[snum]->getTemp(), snum);
        }
      }
      display();
      handle_mqtt(changes, drows, temp_sensor_count);
      for (auto snum = 0; snum < temp_sensor_count; snum++) {
         drows[snum]->ResetChanged();
      }
      lcount++;
			lastUpdate = millis();
	}
	lv_task_handler();
	delay(5);
}

