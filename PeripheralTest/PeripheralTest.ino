/*
* Filename: PeripheralTest.ino
* Author: S25-08
*/

// includes
#include "IOTest/IOTest.hpp"
#include "Debug.hpp"

// Initializes debug serial and IOs
void setup() {
  initDebug(true); // Set parameter to false if you don't want to enable console outputs or test GPS
  initIO();
}

// Loop tests GPS functionality
void loop() {
  // Updates GPS
  while (Serial1.available() > 0)
  {
    gps.encode(Serial1.read());
  }

  if (gps.location.isValid()) {
    serialLogDouble("GPS latitude:", gps.location.lat());
    serialLogDouble("GPS longitude:", gps.location.lng());
    serialLog("\n");
  }

  else {
    turnOnLED(LED_BUILTIN);
    delay(10);
    turnOffLED(LED_BUILTIN);
    delay(10);
  }

  delay(5000);
}