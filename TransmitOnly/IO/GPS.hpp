/*
* Filename: GPS.hpp
* Author: S25-08
*/

#ifndef GPS_H
#define GPS_H

// Includes
#include <TinyGPS++.h>
#include <TimeLib.h>
#include "LED.hpp"

#define GPS_BAUD 38400 // This baud rate is variable depending on the GPS module
#define GPS_TIME_ALLOWABLE_AGE 500  // how old (in ms) the GPS time is allowed to be when syncing with the system clock

TinyGPSPlus gps;

// Updates the GPS data.
// Also syncs arduino system time on a successful gps encode.
void updateGPS()
{
  //get data from GPS
  while (Serial1.available() > 0)
  {
    if (gps.encode(Serial1.read())) {
      if (gps.time.age() < GPS_TIME_ALLOWABLE_AGE) {
        setTime(gps.time.hour(), gps.time.minute(), gps.time.second(), gps.date.day(), gps.date.month(), gps.date.year());
      }
    }
  }
}

/*
* Initializes the GPS
* Note: The GPS uses Serial1 on the arduino which pertain to the default
*   Rx and Tx pins.
*/
void initGPS() {
  Serial1.begin(GPS_BAUD);
  while(!Serial1) {}
}

#endif
