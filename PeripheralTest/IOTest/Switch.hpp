/*
* Filename: Switch.hpp
* Author: S25-08
*/

#ifndef SWITCH_H
#define SWITCH_H

// These are the pins for the buttons
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

#endif
