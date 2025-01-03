/*
* Filename: PeripheralTest.ino
* Author: S25-08
* Description: This file is used to test the peripherals 
* for the beacons.
*/

// includes
#include <RH_RF95.h>
#include <RHMesh.h>

// Radio Pins
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3  

// LED Pins
#define RED_LED_PIN A0
#define YEL_LED_PIN A1
#define GRE_LED_PIN A2

// Button Pins
#define BUTTON_1_PIN 12
#define BUTTON_2_PIN 11
#define BUTTON_3_PIN 10

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

  // LED setups
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YEL_LED_PIN, OUTPUT);
  pinMode(GRE_LED_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(YEL_LED_PIN, LOW);
  digitalWrite(GRE_LED_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);

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

  // Button setups
  pinMode(BUTTON_1_PIN, INPUT);
  digitalWrite(RED_LED_PIN, digitalRead(BUTTON_1_PIN));
  digitalWrite(YEL_LED_PIN, digitalRead(BUTTON_2_PIN));
  digitalWrite(GRE_LED_PIN, digitalRead(BUTTON_3_PIN));
  attachInterrupt(digitalPinToInterrupt(BUTTON_1_PIN), BUTTON_1_CHANGE, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_2_PIN), BUTTON_2_CHANGE, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_3_PIN), BUTTON_3_CHANGE, CHANGE);
}

void loop() {
}

void toggleLED(int LED_PIN) {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

void BUTTON_1_CHANGE() {
  toggleLED(RED_LED_PIN);
}

void BUTTON_2_CHANGE() {
  toggleLED(YEL_LED_PIN);
}

void BUTTON_3_CHANGE() {
  toggleLED(GRE_LED_PIN);
}