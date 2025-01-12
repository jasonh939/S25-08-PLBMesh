/*
* Filename: Basestation.ino
* Author: S25-08
* Note: The python script takes in all the serial outputs.
*   Make sure to set the bool value of the initDebug function to false
    before using the python script.
*/

// includes
#include <TimeLib.h>
#include <TinyGPS++.h>
#include "Basestation.hpp"
#include "BSIO/BSIO.hpp"
#include "Debug.hpp"

// Radio configurations
const uint16_t MyAddress = 1;
byte recPacket[RH_MESH_MAX_MESSAGE_LEN];
uint8_t ackPacket[MAX_ACK_MESSAGE_LEN];

void setup() {
  // put your setup code here, to run once:
  initDebug(true); // Set argument to false when using python script

  serialLog("Setting up IO...");
  initIO();
  serialLog("IO setup complete");

  manager.setThisAddress(MyAddress);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (manager.waitAvailableTimeout(BS_TIMEOUT_INTERVAL))
	{
    uint8_t len = sizeof(recPacket);
    if (isMeshPacket()) {
      uint8_t from;
      if (manager.recvfromAck((uint8_t *)recPacket, &len, &from)) {
        serialLog("Mesh packet recieved");
        serialLogPacketBin(recPacket, len);
        serialLogPacketRead(recPacket, len);
        serialLog("Sending ACK...");

        sprintf((char *)ackPacket, "ACK to address: %d with message ID recieved: %d", from, getMessageID());
        if (manager.sendtoWait(ackPacket, sizeof(ackPacket), from) == RH_ROUTER_ERROR_NONE) {
          serialLog("Successfully sent ACK\n");
        }
        else {
          serialLog("ACK failed to send\n");
        }
      }

      else {
        turnOnLED(YEL_LED_PIN);
        delay(10);
        turnOffLED(YEL_LED_PIN); 
      }
    }

    else {
      // TODO: implement recieving old packet
      if (driver.recv((uint8_t *)recPacket, &len)) {
        serialLog("Legacy packet recieved.");
      }

      else {
        turnOnLED(YEL_LED_PIN);
        delay(10);
        turnOffLED(YEL_LED_PIN); 
      }
    }
	}

  else {
    turnOnLED(RED_LED_PIN);
    delay(10);
    turnOffLED(RED_LED_PIN);
  }
}

bool isMeshPacket() {
  // A header value of 255 indicates that the packet is legacy.
  return (driver.headerFrom() != 255) ? true : false;
}

uint16_t getMessageID() {
  const int msgIDIndex = 1;
  return ((recPacket[msgIDIndex ] & 0b01111111) << 8) | recPacket[msgIDIndex  + 1];
}