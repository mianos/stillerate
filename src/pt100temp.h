#pragma once
#include "MAX31865.h"
#include "mav.h"

#define HSPI_MISO   26
#define HSPI_MOSI   27
#define HSPI_SCLK   25
#define HSPI_SS     33


class PT100Temp : public TempSensor {
  SPIClass *hspi;
  int ss;
  SPISettings *spis;
  MAX31865_RTD *rtd;
  CMAverage<float> *mav;
public:
  PT100Temp(int ss_a=HSPI_SS, int spi_clock=1000000) : ss(ss_a) {
    hspi = new SPIClass(VSPI);
    hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, ss); //SCLK, MISO, MOSI, SS
    spis = new SPISettings(spi_clock, MSBFIRST, SPI_MODE3);
    rtd = new MAX31865_RTD(MAX31865_RTD::RTD_PT100, ss, hspi, spis);
    rtd->configure(true, true, false, true, MAX31865_FAULT_DETECTION_NONE, true, true, 0x0000, 0x7fff);
    mav = new CMAverage<float>(10);
  }
  ~PT100Temp() {
    delete mav;
  }
  bool requestTemp() {
    return rtd->read_all();
  }

  float temp() {
    return mav->avg(rtd->temperature());
    //return rtd->temperature();
  }
};

