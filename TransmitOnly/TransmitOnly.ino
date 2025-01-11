/*
* Filename: TransmitOnly.ino
* Author: S25-08
*/

// includes
#include <TimeLib.h>
#include "TransmitOnly.hpp"
#include "IO/IO.hpp"
#include "Debug.hpp"

// Packet constants
#define PACKET_SIZE_BYTES 16
#define SLEEP_TIME 10000            // time between transmissions
#define SLEEP_TIME_VARIANCE 2000    // randomness in sleep time to prevent two transmitters from consistently colliding
#define GPS_TIME_ALLOWABLE_AGE 500  // how old (in ms) the GPS time is allowed to be when syncing with the system clock

#define SIMULATE_PACKET true
#define NOISE_SEED_PIN A4

// Address of basestation as well as PLB
const uint8_t Basestation = 1;
const uint8_t MyAddress = 3; // NOTE: Change the address based on the PLB address

// Packet info
byte message[PACKET_SIZE_BYTES];
int16_t messageID = 0;

void setup() {
  initDebug(true); // Set parameter to false if you don't want to enable console outputs or test GPS 

  serialLog("Setting up IO...");
  initIO();
  serialLog("IO setup complete\n");

  manager.setThisAddress(MyAddress);

  // RNG will be used to change the interval of transmissions.
  uint32_t noise = analogRead(NOISE_SEED_PIN);
  randomSeed(noise);
  serialLogInteger("Setting random seed with noise:", noise);
  serialLogBool("Initial standby: ", standby);
  serialLogBool("Initial panic: ", panic);
}

void loop() {
  if (standby) {
    standbyMode();
  }
  
  else {
    activeMode();
  }
}

// Standby mode keeps gps lock and sleeps radio module
void standbyMode() {
  driver.sleep();
  smartDelay(1000);  
}

/*
* This function deals with the PLB in active mode.
* 20 second cycle: Transmit -> ACK (5s) -> Downtime (15 +- 2s) -> Repeat
*/
void activeMode() {
  encodeMessage();
  serialLogPacket(message, PACKET_SIZE_BYTES);
  manager.sendtoWait((uint8_t *)message, PACKET_SIZE_BYTES, Basestation);
  messageID++;
  toggleLED(GRE_LED_PIN);
  delay(10);
  toggleLED(GRE_LED_PIN);
  uint16_t waitTime =  random(SLEEP_TIME - SLEEP_TIME_VARIANCE, SLEEP_TIME + SLEEP_TIME_VARIANCE);
  serialLogInteger("Waiting", waitTime, "ms");
  smartDelay(waitTime);
}

// Transmits a single packet to the base station
void handleTransmit() {

}

// Wait for an ACK from the base station
void handleACK() {

}

// Downtime between ACK and next packet transmit. Keeps a GPS lock
void handleIdle() {

}

void encodeMessage() {
  /*
  NEW packet structure:
  - 8 bit radio ID
  - 1 bit panic state
  - 15 bit message ID
  - 32 bit latitude
  - 32 bit longitude
  - 8 bit battery life
  - 32 bit UTC

  Total size: 128 bits or 16 bytes
  */

  int byteIndex = 0;

  float gpsLat = 100.;
  float gpsLong = 200.;
  uint8_t batteryPercent = 50;
  int32_t utc = now();

  if (!SIMULATE_PACKET) {
    // TODO: override simulated packet numbers here if using GPS
  }

  // Log packet before sending
  serialLogInteger("Radio ID:", MyAddress);
  serialLogInteger("Panic State:", panic);
  serialLogInteger("Message ID:", messageID);
  serialLogDouble("GPS latitude:", gpsLat);
  serialLogDouble("GPS longitude:", gpsLong);
  serialLogInteger("Battery Percent:", batteryPercent);
  serialLogInteger("Timestamp:", utc);

  // encode radio ID
  for (int i = sizeof(MyAddress)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&MyAddress)[i];
    byteIndex++;
  }

  // encode panicState and messageID
  uint16_t panicMask = messageID | (panic ? 0x8000 : 0x0000);
  for (int i = sizeof(panicMask)-1; i>=0; i--) {
    message[byteIndex] = ((uint8_t*)&panicMask)[i];
    byteIndex++;
  } 

  // encode gpsLat
  for (int i = sizeof(gpsLat)-1; i>=0; i--)
  {
    message[byteIndex] = ((uint8_t*)&gpsLat)[i];
    byteIndex++;
  }

  // encode gpsLong
  for (int i = sizeof(gpsLong)-1; i>=0; i--)
  {
    message[byteIndex] = ((uint8_t*)&gpsLong)[i];
    byteIndex++;
  }

  // encode batteryPercent
  message[byteIndex] = batteryPercent;
  byteIndex++;

  // encode utc
  for (int i = sizeof(utc)-1; i>=0; i--)
  {
    message[byteIndex] = ((uint8_t*)&utc)[i];
    byteIndex++;
  }
}
