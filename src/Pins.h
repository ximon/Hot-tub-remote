#ifndef PINS_H
#define PINS_H

//D1 Mini Pins
const int LED = D4;
//#define LED_BIT 1 << LED
const int DATA_IN = D1;
const int DATA_ENABLE = D2;
const int DATA_OUT = 3; // GPIO3 = I2S data out
//#define DATA_OUT_BIT 1 << DATA_OUT
const int DBG = D8;
const int DBG_BIT = 1 << DBG;
const int BUTTON = D5;

#endif
