/*
* Filename: GPS.hpp
* Author: S25-08
*/

#ifndef GPS_H
#define GPS_H

// Includes
#include <TinyGPS++.h>
#include "LED.hpp"

#define GPS_BAUD 38400 // This baud rate is variable depending on the GPS module

TinyGPSPlus gps;

// Delay function that can read incoming GPS information during the delay time
void smartDelay(uint16_t ms)
{
  unsigned long start = millis();
  while ((millis() - start) < ms)
  {
    //get data from GPS
    while (Serial1.available() > 0)
    {
      gps.encode(Serial1.read());
    }
  }
}

// Waits for a GPS lock
void waitForLock() {
  while (!gps.location.isValid()) {
    smartDelay(1000);
    toggleLED(GRE_LED_PIN);
  }

  turnOffLED(GRE_LED_PIN);
}

/*
* Initializes the GPS
* Note: The GPS uses Serial1 on the arduino which pertain to the default
*   Rx and Tx pins.
*/
void initGPS() {
  Serial1.begin(GPS_BAUD);
  waitForLock();
}

#endif
