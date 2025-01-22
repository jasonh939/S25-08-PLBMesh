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
  
  /* 
  * After this point, the LED and switches should be functional.
  * The GPS lock might take a couple of minutes.
  */
  initGPS(); // NOTE: Comment out if using range-extender
}

#endif