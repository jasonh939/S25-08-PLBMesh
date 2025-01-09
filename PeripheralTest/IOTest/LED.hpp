/*
* Filename: LED.hpp
* Author: S25-08
*/

#ifndef LED_H
#define LED_H

// These are the pins for the LEDs
#define RED_LED_PIN A0
#define YEL_LED_PIN A1
#define GRE_LED_PIN A2

// Turns on a specific LED
void turnOnLED(int LED_PIN) {
  digitalWrite(LED_PIN, HIGH);
}

// Turns off a specific LED
void turnOffLED(int LED_PIN) {
  digitalWrite(LED_PIN, LOW);
}

// Toggles a specific LED
void toggleLED(int LED_PIN) {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

#endif
