/*
* Filename: Switch.hpp
* Author: S25-08
*/

#ifndef SWITCH_H
#define SWITCH_H

// Includes
#include "LED.hpp"

// These are the pins for the switches
#define SWITCH_1_PIN 12
#define SWITCH_2_PIN 11
#define SWITCH_3_PIN 10

// These are ISR functions detecting switch toggles
void SWITCH_1_CHANGE() {
  toggleLED(RED_LED_PIN);
}

void SWITCH_2_CHANGE() {
  toggleLED(YEL_LED_PIN);
}

void SWITCH_3_CHANGE() {
  toggleLED(GRE_LED_PIN);
}

// Initializes and sets up the switches to use interrupts
void initSwitches() {
  pinMode(SWITCH_1_PIN, INPUT);
  pinMode(SWITCH_2_PIN, INPUT);
  pinMode(SWITCH_3_PIN, INPUT);
  digitalWrite(RED_LED_PIN, digitalRead(SWITCH_1_PIN));
  digitalWrite(YEL_LED_PIN, digitalRead(SWITCH_2_PIN));
  digitalWrite(GRE_LED_PIN, digitalRead(SWITCH_3_PIN));
  attachInterrupt(digitalPinToInterrupt(SWITCH_1_PIN), SWITCH_1_CHANGE, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SWITCH_2_PIN), SWITCH_2_CHANGE, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SWITCH_3_PIN), SWITCH_3_CHANGE, CHANGE);
}

#endif
