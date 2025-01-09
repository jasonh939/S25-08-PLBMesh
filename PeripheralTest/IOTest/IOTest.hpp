/*
* Filename: IOTest.hpp
* Author: S25-08
*/

#ifndef IO_TEST_H
#define IO_TEST_H

#include "LED.hpp"
#include "Switch.hpp"
#include "Radio.hpp"

// Initializes the IO used for the PLB
void initIO() {
  initLEDs();
  initRadio();
  initSwitches();
}

#endif