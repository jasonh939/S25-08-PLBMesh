/*
* Filename: Radio.hpp
* Author: S25-08
*/

#ifndef RADIO_H
#define RADIO_H

// Includes
#include <RH_RF95.h>
#include <RHMesh.h>
#include "LED.hpp"

// These are the pins for the radio module on the Adafruit feather m0
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Radio configuration
const float Frequency = 915.0;
const int8_t TxPower = 7;

// Singletons
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHMesh manager(driver);

// Initializes radio and configure the frequency and tx power
void initRadio() {
  if (!manager.init()) {
    turnOnLED(LED_BUILTIN);
    while (true) {}
  }

  driver.setFrequency(Frequency);
  driver.setTxPower(TxPower);
}

#endif
