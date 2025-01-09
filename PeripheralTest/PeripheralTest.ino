/*
* Filename: PeripheralTest.ino
* Author: S25-08
*/

// includes
#include <RH_RF95.h>
#include <RHMesh.h>
#include "IOTest/IOTest.hpp"

// Radio Pins
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3  

// Baud rate constants
#define DEBUG_BAUD 9600
#define GPS_BAUD 38400 // This baud rate is variable depending on the GPS module

// Debug
#define SKIP_GPS_TEST true
#define Console SerialUSB

// Radio configurations
const uint16_t MyAddress = 1;
const float Frequency = 915.0;
const int8_t TxPower = 7;
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHMesh manager(driver, MyAddress);

void setup() {
  // Serial setups
  Serial1.begin(GPS_BAUD);
  Console.begin(DEBUG_BAUD);
  while (!Console) {}
  Console.println("Serials Initialized");

  // Radio setup
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10); 

  // Solid built in LED indicates failed radio initialization
  Console.println("Initializing radio...");
  if (!manager.init()) {
    Console.println("Radio initializing failed");
    digitalWrite(LED_BUILTIN, HIGH);
    while (true) {}
  }
  Console.println("Radio setup complete");

  // GPS setup
  if (!SKIP_GPS_TEST) {
    // TODO: setup GPS test and obtain lock
  }

  initIO();
}

void loop() {
}