/*
* Filename: IOTest.hpp
* Author: S25-08
*/

#ifndef IO_TEST_H
#define IO_TEST_H

#include "LED.hpp"
#include "Switch.hpp"

// Initializes all the IO
void initIO() {
  // LED setups
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YEL_LED_PIN, OUTPUT);
  pinMode(GRE_LED_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YEL_LED_PIN, LOW);
  digitalWrite(GRE_LED_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);

  // Switch setups
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