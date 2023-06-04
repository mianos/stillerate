#include <Arduino.h>
#include <lvgl.h>

#include "ui.h"
#include "panel.h"

volatile int counter = 0;  // Counter variable
unsigned long lastUpdate = 0; // to store the time of the last update

void setup()
{
  Serial.begin(115200);
  initLVGL();
  lv_textarea_add_text(ui_messages, "LVGL Up");
//  lv_msgbox_set_text(msgbox, "");
}


void loop()
{
	if (millis() - lastUpdate > 1000) { // If 5000ms have passed
			counter++;
      lv_label_set_text_fmt(ui_clock, "12:34:%02d", counter);
      char buffer[100];
      sprintf(buffer, "counter %d\n", counter);
      lv_textarea_add_text(ui_messages, buffer);
			lastUpdate = millis();
	}
	lv_task_handler();
	delay(5);
}

