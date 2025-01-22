/*
* Filename: RangeExtender.ino
* Author: S25-08
*/

// includes
#include "RangeExtender.hpp"
#include "IO/IO.hpp"
#include "Debug.hpp"

// Address of basestation as well as all the PLBs
const uint8_t Basestation = 0;
const uint8_t LegacyBeacon = 1;
const uint8_t Beacon1 = 2;
const uint8_t Beacon2 = 3;
const uint8_t Beacon3 = 4;
const uint8_t Beacon4 = 5;

// Packet info
const uint8_t MyAddress = Beacon2; // NOTE: Change the address based on the PLB address
byte packet[PACKET_SIZE_BYTES];


void setup() {
  initDebug(true); // NOTE: Set parameter to false if not using Arduino IDE. 

  serialLog("RANGE-EXTENDER MODE");
  serialLog("Setting up IO...");
  initIO();
  serialLog("IO setup complete");

  manager.setThisAddress(MyAddress);
}

void loop() {
  updateLEDs();

  if (switchIsOn(ACTIVE_STANDBY_PIN)) {
    standbyMode();
  }
  
  else {
    activeMode();
  }
}

// Changes yellow (standby) if status changed
void updateLEDs() {
  switchIsOn(ACTIVE_STANDBY_PIN) ? turnOnLED(YEL_LED_PIN) : turnOffLED(YEL_LED_PIN);
}

// Standby mode sleeps radio module
void standbyMode() {
  driver.sleep();
}

/*
* This function deals with the PLB in active mode.
* Waits for any incoming packet. If the packet is mesh, it autoroutes.
* If the packet is legacy, repackage it as mesh and send it.
*/
void activeMode() {
  if (manager.waitAvailableTimeout(1)) {
    uint8_t len = sizeof(packet);
    if (isMeshPacket()) {
      uint8_t from;
      if (!manager.recvfromAck((uint8_t *)packet, &len, &from)) {
        serialLog("Mesh packet autorouted");
      }
    }

    else {
      if (driver.recv((uint8_t *)packet, &len)) {
        serialLog("Legacy packet recieved.\n");
        serialLogPacketBin(packet, len);
        serialLogPacketRead(packet, len);
        serialLog("Repacking and sending...");
        repackLegacy();
      }
    }
  }
}

// Function to repack legacy packets and send them to basestation.
void repackLegacy() {
  if (manager.sendtoWait(packet, sizeof(packet), Basestation) == RH_ROUTER_ERROR_NONE) {
    serialLog("Legacy packet successfully repackaged and sent to basestation");
  }

  else {
    serialLog("Legacy packet failed to be repackaged and sent to basestation");
  }
}

bool isMeshPacket() {
  // A header value of 255 indicates that the packet is legacy.
  return (driver.headerFrom() != 255) ? true : false;
}
