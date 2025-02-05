/*
* Filename: PeripheralTest.ino
* Author: S25-08
*/

// includes
#include "IOTest/IOTest.hpp"
#include "Debug.hpp"

#define DISABLE_GPS true // Set to false if testing GPS

// Initializes debug serial and IOs
void setup() {
  initDebug(true); // Set parameter to false if you don't want to enable console outputs or test GPS
  initIO();

  // Wait for GPS lock
  if (!DISABLE_GPS) {
    waitForLock();
  }
}

// Loop tests GPS functionality
void loop() {
  // Updates GPS
  if (!DISABLE_GPS) {
    while (Serial1.available() > 0)
    {
      gps.encode(Serial1.read());
    }

    if (gps.location.isValid()) {
      serialLogDouble("GPS latitude:", gps.location.lat());
      serialLogDouble("GPS longitude:", gps.location.lng());
      serialLog("\n");
    }
  }

  else {
    turnOnLED(LED_BUILTIN);
    delay(10);
    turnOffLED(LED_BUILTIN);
    delay(10);
  }

  toggleLED(GRE_LED_PIN);
  delay(5000);
}

// Function will block until a GPS lock is obtained. Also updates LED lights incase switch toggles
void waitForLock() {
  while (!gps.location.isValid()) {
    unsigned long start = millis();
    while ((millis() - start) < GPS_INTERVAL) {
      updateGPS();
    }
    toggleLED(GRE_LED_PIN);
  }

  turnOffLED(GRE_LED_PIN);
}