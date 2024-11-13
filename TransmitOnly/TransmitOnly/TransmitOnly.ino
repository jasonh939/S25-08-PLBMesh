// includes
#include <RH_RF95.h>
#include <RHMesh.h>
#include <TinyGPS++.h>
#include <TimeLib.h>

// Radio Pins
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3  

// LED Pins
#define RED_LED_PIN A0
#define GREEN_LED_PIN A1

// Packet constants
#define PACKET_SIZE_BYTES 16
#define SLEEP_TIME 10000            // time between transmissions
#define SLEEP_TIME_VARIANCE 2000    // randomness in sleep time to prevent two transmitters from consistently colliding
#define GPS_TIME_ALLOWABLE_AGE 500  // how old (in ms) the GPS time is allowed to be when syncing with the system clock

// Baud rate constants
#define DEBUG_BAUD 9600
#define GPS_BAUD 38400 // This baud rate is variable depending on the GPS module

// Debug
#define SERIAL_DEBUG true
#define Console SerialUSB
#define DISABLE_STANDBY true
#define SIMULATE_PACKET false

// Network defining the beacon ID's
const uint8_t Basestation = 1;
const uint8_t Beacon1 = 2;
const uint8_t Beacon2 = 3;
const uint8_t Beacon3 = 4;
const uint8_t Beacon4 = 5;

// Radio configurations
const uint8_t MyAddress = Beacon1; // Change beacon number depending on beacon used.
const float Frequency = 915.0;
const int8_t TxPower = 7;

// Singletons
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHMesh manager(driver, MyAddress);
TinyGPSPlus gps;

void setup() {
  // Serial setups
  Serial1.begin(GPS_BAUD);
  if (SERIAL_DEBUG) {
    Console.begin(DEBUG_BAUD);
    while (!Console) {}

    serialLog("Serial Initialized");
    serialLogInteger("Address: ", MyAddress);
  }

  // Pin setups
  pinMode(RFM95_RST, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Reset pin outputs
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);

  // manually reset rf95
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);  

  serialLog("Initializing radio...");
  if (!manager.init()) {
    serialLog("Radio initializing failed");
    while (true) {
      toggleLED(RED_LED_PIN);
      delay(1000);
    }
  }

  driver.setFrequency(Frequency);
	driver.setTxPower(TxPower);

  if (!SIMULATE_PACKET) {
    serialLog("Waiting for GPS lock...");
    while (!gps.location.isValid()) {
      while (Serial1.available() > 0) {
        gps.encode(Serial1.read());
      }

      toggleLED(GREEN_LED_PIN);
      delay(1000);
    }
    serialLog("GPS obtained lock");
  }

  else {
    serialLog("Skipping GPS lock");
  }
}

uint8_t message[PACKET_SIZE_BYTES];
int8_t messageID = 0;

void loop() {
  toggleLED(GREEN_LED_PIN);
  delay(500);
}

void toggleLED(int LED_PIN) {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

void serialLog(String message) {
  if (SERIAL_DEBUG) {
    Console.println(message);
  }
}

void serialLogInteger(String prefix, long intValue)
{
  if (SERIAL_DEBUG)
  {
    Console.print(prefix);
    Console.print(" ");
    Console.println(intValue);
  }
}
