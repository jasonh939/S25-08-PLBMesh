/*
* Filename: IOTest.hpp
* Author: S25-08
*/

#ifndef IO_H
#define IO_H


// Includes
#include "LED.hpp"
#include "Switch.hpp"
#include "Radio.hpp"
#include "GPS.hpp"

// Initializes the IO used for the PLB
void initIO() {
  initLEDs();
  initRadio();
  initSwitches();
  initGPS();
}

#endif