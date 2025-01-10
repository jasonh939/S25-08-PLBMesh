// includes
#include <RH_RF95.h>
#include <RHMesh.h>
#include <TinyGPS++.h>
#include <TimeLib.h>
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
const uint16_t Basestation = 1;
const uint16_t MyAddress = 2; // NOTE: Change the address based on the PLB address

void setup() {
  initDebug(false); // Set parameter to false if you don't want to enable console outputs or test GPS 

  serialLog("Setting up IO...");
  initIO();
  serialLog("IO setup complete\n");

  // RNG will be used to change the interval of transmissions.
  uint32_t noise = analogRead(NOISE_SEED_PIN);
  randomSeed(noise);
  serialLogInteger("Setting random seed with noise:", noise);
  serialLogBool("Initial standby: ", standby);
  serialLogBool("Initial panic: ", panic);
}

byte message[PACKET_SIZE_BYTES];
int8_t messageID = 0;

void loop() {
  encodeMessage();
  for (int i = 0; i < sizeof(message); i++) {
    Console.print(message[i], BIN);
    Console.print(" ");
  }
  Console.println();
  manager.sendtoWait((uint8_t *)message, PACKET_SIZE_BYTES, Basestation);
  messageID++;
  toggleLED(GRE_LED_PIN);
  delay(10);
  toggleLED(GRE_LED_PIN);
  uint16_t waitTime =  random(SLEEP_TIME - SLEEP_TIME_VARIANCE, SLEEP_TIME + SLEEP_TIME_VARIANCE);
  serialLogInteger("Waiting for", waitTime);
  smartDelay(waitTime);
}

// Delay function that can read incoming GPS information during the delay time
static void smartDelay(uint16_t ms)
{
  unsigned long start = millis();
  while ((millis() - start) < ms)
  {
    //get data from GPS
    while (Serial1.available() > 0)
    {
      gps.encode(Serial1.read());
    }
  }
}

void waitingForLock() {
  toggleLED(GRE_LED_PIN);
  smartDelay(1000);
  toggleLED(GRE_LED_PIN);
  smartDelay(1000);
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

  // NOTE: The new packet structure hasn't been implemented. The current packet structure is still 16 bit Radio ID and 7 bit message ID

  int byteIndex = 0;

  bool panicState = true;
  float gpsLat = 100.;
  float gpsLong = 200.;
  uint8_t batteryPercent = 50;
  int32_t utc = now();

  if (!SIMULATE_PACKET) {
    // TODO: override simulated packet numbers here if using GPS
  }

  // Log packet before sending
  serialLogInteger("Radio ID:", MyAddress);
  serialLogInteger("Panic State:", panicState);
  serialLogInteger("Message ID:", messageID);
  serialLogDouble("GPS latitude:", gpsLat);
  serialLogDouble("GPS longitude:", gpsLong);
  serialLogInteger("Battery Percent:", batteryPercent);
  serialLogInteger("Timestamp:", utc);

  // encode radio ID
  for (int i=sizeof(MyAddress)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&MyAddress)[i];
    byteIndex++;
  }

  // encode panicState and messageID
  uint8_t panicMask = panicState ? 0b10000000 : 0b00000000;
  message[byteIndex] = messageID | panicMask;
  byteIndex++;

  // encode gpsLat
  for (int i = sizeof(gpsLat)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&gpsLat)[i];
    byteIndex++;
  }

  // encode gpsLong
  for (int i = sizeof(gpsLong)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&gpsLong)[i];
    byteIndex++;
  }

  // encode batteryPercent
  message[byteIndex] = batteryPercent;
  byteIndex++;

  // encode utc
  for (int i = sizeof(utc)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&utc)[i];
    byteIndex++;
  }
}
