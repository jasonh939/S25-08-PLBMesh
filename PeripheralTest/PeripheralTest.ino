/*
* Filename: PeripheralTest.ino
* Author: S25-08
*/

// includes
#include "IOTest/IOTest.hpp"


// Baud rate constants
#define DEBUG_BAUD 9600
#define GPS_BAUD 38400 // This baud rate is variable depending on the GPS module

// Debug
#define SKIP_GPS_TEST true
#define Console SerialUSB

void setup() {
  // Serial setups
  Serial1.begin(GPS_BAUD);
  Console.begin(DEBUG_BAUD);
  while (!Console) {}
  Console.println("Serials Initialized");

  // GPS setup
  if (!SKIP_GPS_TEST) {
    // TODO: setup GPS test and obtain lock
  }

  initIO();
}

void loop() {
}