/*
* Filename: Basestation.ino
* Author: S25-08
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
char ackPacket[MAX_ACK_MESSAGE_LEN];

void setup() {
  // put your setup code here, to run once:
  initDebug(true);

  serialLog("Setting up IO...");
  initIO();
  serialLog("IO setup complete");

  manager.setThisAddress(MyAddress);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (manager.waitAvailableTimeout(BS_TIMEOUT_INTERVAL))
	{
		// // Wait for a message addressed to us from the client
		// uint8_t len = sizeof(recPacket);
		// uint8_t from;
		// if (manager.recvfromAck((uint8_t *)recPacket, &len, &from))
		// {
		// 	Console.print ("from @");
		// 	Console.print (from, DEC);
		// 	Console.print (" R[");
		// 	Console.print (len);
		// 	Console.print ("]<");
		// 	//Console.print (buf);
    //   for (int i = 0; i < len; i++){
    //      Console.print(recPacket[i], BIN);
    //      Console.print(" ");
    //   }
    //   Console.println();
		// 	Console.print (">(");
		// 	Console.print (driver.lastRssi(), DEC);
		// 	Console.print ("dBm) ---> send reply to @");
		// 	Console.print (from, DEC);
		// 	Console.println (" ---> ");
		// 	Console.flush();
		// }
	}

  else {
    turnOnLED(RED_LED_PIN);
    delay(10);
    turnOffLED(RED_LED_PIN);
  }
}

bool isMeshPacket() {
  // TODO: implement
  return true;
}

