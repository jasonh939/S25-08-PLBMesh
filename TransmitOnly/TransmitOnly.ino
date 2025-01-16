/*
* Filename: TransmitOnly.ino
* Author: S25-08
*/

// includes
#include <TimeLib.h>
#include <TinyGPS++.h>
#include "TransmitOnly.hpp"
#include "IO/IO.hpp"
#include "Debug.hpp"

// Battery constants
#define VBATPIN A7 
// TODO: might need to change threshold values if we switch batteries
#define BATTERY_MIN_THRESHOLD 134
#define BATTERY_MAX_THRESHOLD 511

#define SIMULATE_PACKET false
#define NOISE_SEED_PIN A4

// Address of basestation as well as all the PLBs
const uint8_t Basestation = 1;
const uint8_t Beacon1 = 2;
const uint8_t Beacon2 = 3;
const uint8_t Beacon3 = 4;
const uint8_t Beacon4 = 5;

// Packet info
const uint8_t MyAddress = Beacon3; // NOTE: Change the address based on the PLB address
byte message[PACKET_SIZE_BYTES];
int16_t messageID = 0;

// Active mode state
Active_State currState = TRANSMIT;

void setup() {
  initDebug(true); // NOTE: Set parameter to false if not using Arduino IDE. 

  serialLog("TRANSMIT-ONLY MODE");
  serialLog("Setting up IO...");
  initIO();
  serialLog("IO setup complete");

  manager.setThisAddress(MyAddress);

  // RNG will be used to change the interval of transmissions.
  uint32_t noise = analogRead(NOISE_SEED_PIN);
  randomSeed(noise);
  serialLogInteger("Setting random seed with noise:", noise);
  serialLog("");

  pinMode(VBATPIN, INPUT);
}

void loop() {
  updateLEDs();

  if (standby) {
    standbyMode();
  }
  
  else {
    activeMode();
  }
}

// Changes yellow (standby) and/or red (panic) LED if status changed
void updateLEDs() {
  panic ? turnOnLED(RED_LED_PIN) : turnOffLED(RED_LED_PIN);
  standby ? turnOnLED(YEL_LED_PIN) : turnOffLED(YEL_LED_PIN);
}

// Standby mode keeps gps lock and sleeps radio module
void standbyMode() {
  driver.sleep();
  smartDelay(1);  
}

/*
* This function deals with the PLB in active mode.
* 20 second cycle: Transmit -> ACK (5s) -> Downtime (15 +- 2s) -> Repeat
*/
void activeMode() {
  switch(currState) {
    case TRANSMIT:
      handleTransmit();
      break;
    
    case ACK:
      handleACK();
      break;
    
    case IDLE:
      handleIdle();
      break;
  }
}

// Transmits a single packet to the base station
void handleTransmit() {
  serialLog("Transmit State");
  serialLog("");
  encodeMessage();
  serialLogPacketBin(message, PACKET_SIZE_BYTES);
  serialLogPacketRead(message, PACKET_SIZE_BYTES);
  serialLog("");

  // senttoWait is slightly blocking
  serialLog("Sending packet...");
  if (manager.sendtoWait((uint8_t *)message, PACKET_SIZE_BYTES, Basestation) == RH_ROUTER_ERROR_NONE) {
    serialLog("Packet successfully sent\n");
    turnOnLED(GRE_LED_PIN);
    delay(10);
    turnOffLED(GRE_LED_PIN);
    currState = ACK;
  }

  else {
    serialLog("Packet failed to send\n");
    turnOnLED(YEL_LED_PIN);
    delay(10);
    turnOffLED(YEL_LED_PIN);
    currState = IDLE;
  }
}

// Wait for an ACK from the base station
// Early function exit if standby is pressed
// NOTE: There's a small bug when spam toggling the standby switch it restarts the arduino.
//  Not sure if that is bad connections, software bug, or faulty switch.
void handleACK() {
  serialLog("ACK State. Waiting for ACK...");
  
  byte ackPacket[MAX_ACK_MESSAGE_LEN];
  uint8_t len = sizeof(ackPacket);
  uint8_t from;

  unsigned long start = millis();
  while ((millis() - start) < ACK_INTERVAL) {
    updateLEDs();
    if (standby) {
      serialLog("Standby detected, early ACK exit");
      return;
    }
    smartDelay(1);

    if (manager.recvfromAckTimeout((uint8_t *)ackPacket, &len, 1, &from)) {
      // TODO: implement visual ACK
      serialLog("ACK recieved");
      serialLogCharArray("Packet message:", (char *)ackPacket);
      serialLog("");
      currState = IDLE;
      return;
    }
  }

  serialLog("No ACK recieved\n");
  currState = IDLE;
}

// Downtime between ACK and next packet transmit. Keeps a GPS lock
// Early function exit if standby is pressed
void handleIdle() {
  serialLog("Idle State");
  unsigned long start = millis();
  uint16_t waitTime = random(IDLE_INTERVAL - IDLE_VARIANCE, IDLE_INTERVAL + IDLE_VARIANCE);
  while ((millis() - start) < waitTime) {
    updateLEDs();
    if (standby) {
      serialLog("Standby detected, early idle exit.");
      return;
    }
    smartDelay(1);
  }

  currState = TRANSMIT;
}

// Function to encode the packet. Return of true indicates all packets are valid.
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

  // Default packet values. If gps data is invalid, they won't be updated.
  float gpsLat = 0.;
  float gpsLng = 0.;
  uint8_t batteryPercent = 0;
  uint32_t utc = 0;

  if (!SIMULATE_PACKET) {
    if (gps.location.isValid()) {
      gpsLat = gps.location.lat();
      gpsLng = gps.location.lng();
    }
    else {
      serialLog("GPS location is invalid");
    }

    if (timeStatus() != timeNotSet) {
      utc = now();
    }

    else {
      serialLog("System clock has not been set");   
    }

    // TODO: Implement battery reading. Old group code doesn't work becuase pin A7 is not recognized.
  }

  // encode radio ID
  for (int i = sizeof(MyAddress)-1; i>=0; i--)
  {
    // MSB is to determine if packet is legacy (0) or new (1)
    message[byteIndex] = ((uint8_t*)&MyAddress)[i] | 0b10000000;
    byteIndex++;
  }

  // encode panicState and messageID
  uint16_t panicMask = messageID | (panic ? 0x8000 : 0x0000);
  for (int i = sizeof(messageID)-1; i>=0; i--) {
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
  for (int i = sizeof(gpsLng)-1; i>=0; i--)
  {
    message[byteIndex] = ((uint8_t*)&gpsLng)[i];
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

  messageID++;
}