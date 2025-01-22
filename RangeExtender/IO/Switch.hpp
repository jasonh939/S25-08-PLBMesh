/*
* Filename: Switch.hpp
* Author: S25-08
*/

#ifndef SWITCH_H
#define SWITCH_H

// Includes
#include "LED.hpp"

// These are the pins for the switches
#define ACTIVE_STANDBY_PIN 11
#define PANIC_SWITCH_PIN 12
// The power switch is attached to the "en" pin on the arduino
//#define POWER_SWITCH_PIN EN

bool switchIsOn(int SWITCH_PIN) {
  return digitalRead(SWITCH_PIN) == HIGH;
}

// Initializes and sets up the switches to use interrupts
// Also sets the intial standby/active state and panic state
void initSwitches() {
  pinMode(ACTIVE_STANDBY_PIN, INPUT);
  pinMode(PANIC_SWITCH_PIN, INPUT);
}

#endif