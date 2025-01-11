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

// volatile variables for changing PLB states
volatile bool standby = false;
volatile bool panic = false;

// These are ISR functions detecting switch toggles
// NOTE: there's a possibility that these malfunction if the voltage randomly shifts without touching the switch.
// This will cause the switches to have inverted behaviour. Not sure if they will be negligible problems or will have to change.
void ACTIVE_STANDBY_TOGGLE() {
  toggleLED(YEL_LED_PIN);
  standby = !standby;
}

void PANIC_TOGGLE() {
  toggleLED(RED_LED_PIN);
  panic = !panic;
}

// Initializes and sets up the switches to use interrupts
// Also sets the intial standby/active state and panic state
void initSwitches() {
  pinMode(ACTIVE_STANDBY_PIN, INPUT);
  pinMode(PANIC_SWITCH_PIN, INPUT);

  if (digitalRead(ACTIVE_STANDBY_PIN) == HIGH) {
    turnOnLED(YEL_LED_PIN);
    standby = true;
  }

  if (digitalRead(PANIC_SWITCH_PIN) == HIGH) {
    turnOnLED(RED_LED_PIN);
    panic = true;
  }
  
  attachInterrupt(digitalPinToInterrupt(ACTIVE_STANDBY_PIN), ACTIVE_STANDBY_TOGGLE, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PANIC_SWITCH_PIN), PANIC_TOGGLE, CHANGE);
}

#endif