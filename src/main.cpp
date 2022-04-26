#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <max6675.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include "esp_adc_cal.h"

#include "driver/mcpwm.h"

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

int thermoDO = GPIO_NUM_25;
int thermoCS = GPIO_NUM_26;
int thermoCLK = GPIO_NUM_27;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS GPIO_NUM_33

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
 


//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms) //use-> espDelay(6000);
{   
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void espSpeed(int speed) {
    mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, speed);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0); //call this each time, if operator was previously in low/high state

}

gpio_num_t motor_enable_pin = GPIO_NUM_13;

void init_motor(gpio_num_t pwm_pin, gpio_num_t enable_pin) {
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, pwm_pin);

    mcpwm_config_t pwm_config;
    pwm_config.frequency = 1000;    //frequency,
    pwm_config.cmpr_a = 0;    		//duty cycle of PWMxA = 0
    pwm_config.cmpr_b = 0;    		//duty cycle of PWMxb = 0
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings

    motor_enable_pin = enable_pin;
    gpio_set_direction(enable_pin, GPIO_MODE_OUTPUT);

}
void setup()
{
    Serial.begin(115200);
    Serial.println("Start");
    sensors.begin();

    init_motor(GPIO_NUM_12, GPIO_NUM_13);
    tft.init();
    tft.fontHeight(4);
    tft.setRotation(1);
     if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nSPIFFS available!");

  // ESP32 will crash if any of the fonts are missing
  bool font_missing = false;
  if (SPIFFS.exists("/NotoSansBold15.vlw")    == false) font_missing = true;
  if (SPIFFS.exists("/NotoSansBold36.vlw")    == false) font_missing = true;

  if (font_missing)
  {
    Serial.println("\r\nFont missing in SPIFFS, did you upload it?");
    while(1) yield();
  }
  else Serial.println("\r\nFonts found OK.");
  tft.loadFont(AA_FONT_LARGE); // Must load the font first
}

double last_value = 0.0;

void loop()
{
#if 0
    gpio_set_level(motor_enable_pin, 1);
    espSpeed(100);
    tft.fillScreen(TFT_BLACK);
    tft.drawString("Start", tft.width()/4, tft.height() / 2, 4);  //string,start x,start y, font weight {1;2;4;6;7;8}
    espDelay(1000);
    gpio_set_level(motor_enable_pin, 0);
    espSpeed(80); 
    tft.fillScreen(TFT_BLACK);
    tft.drawString("stop", tft.width()/4, tft.height() / 2, 4);  //string,start x,start y, font weight {1;2;4;6;7;8}
   Serial.printf("temp %02.2f\n", thermocouple.readCelsius());

#endif
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  float tempC = sensors.getTempCByIndex(0);

  // Check if reading was successful
  if(tempC != DEVICE_DISCONNECTED_C) {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(tempC);
    tft.fillScreen(TFT_BLACK);
    tft.drawFloat(tempC, 3, 0, 0);
//    tft.drawFloat(tempC, tft.width()/4, tft.height() / 2, 4);
    const char *symbol = " ";
    if(last_value != 0.0) {
        if (tempC > last_value) {
            symbol = "+";  
        } else {
            symbol = "-";
        }
        tft.drawString(symbol, tft.width() / 2, 4);
    }
    last_value = tempC;
  //  tft.fillScreen(TFT_BLACK);
  } 
  else
  {
    Serial.println("Error: Could not read temperature data");
  }
   espDelay(1000);

}


