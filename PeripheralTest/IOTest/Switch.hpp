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
// #define POWER_SWITCH_PIN EN

// These are ISR functions detecting switch toggles
void PANIC_TOGGLE() {
  toggleLED(RED_LED_PIN);
}

void ACTIVE_STANDBY_TOGGLE() {
  toggleLED(YEL_LED_PIN);
}

// Initializes and sets up the switches to use interrupts
void initSwitches() {
  pinMode(PANIC_SWITCH_PIN, INPUT);
  pinMode(ACTIVE_STANDBY_PIN, INPUT);
  digitalWrite(RED_LED_PIN, digitalRead(PANIC_SWITCH_PIN));
  digitalWrite(YEL_LED_PIN, digitalRead(ACTIVE_STANDBY_PIN));
  attachInterrupt(digitalPinToInterrupt(PANIC_SWITCH_PIN), PANIC_TOGGLE, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ACTIVE_STANDBY_PIN), ACTIVE_STANDBY_TOGGLE, CHANGE);
}

#endif
