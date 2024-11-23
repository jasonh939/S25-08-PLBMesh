// includes
#include <RH_RF95.h>
#include <RHMesh.h>
#include <TinyGPS++.h>
#include <TimeLib.h>

// Radio Pins
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3  

#define Console SerialUSB
#define SERIAL_DEBUG true

// I/O Pins
#define RED_LED_PIN 13

// Network defining the beacon ID's
const uint16_t Basestation = 1;
const uint16_t Beacon1 = 2;
const uint16_t Beacon2 = 3;
const uint16_t Beacon3 = 4;
const uint16_t Beacon4 = 5;

// Radio configurations
const uint16_t MyAddress = Basestation; // Change beacon number depending on beacon used.
const float Frequency = 915.0;
const int8_t TxPower = 7;

// Baud rate constants
#define DEBUG_BAUD 9600

// Singletons
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHMesh manager(driver, MyAddress);
TinyGPSPlus gps;


void setup() {
  // put your setup code here, to run once:
  // Serial setups
  if (SERIAL_DEBUG) {
    Console.begin(DEBUG_BAUD);
    while (!Console) {}

    serialLog("Serial Initialized");
    serialLogInteger("Address: ", MyAddress);
  }
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
}

// Dont put this on the stack:
char buf[RH_MESH_MAX_MESSAGE_LEN];

void loop() {
  // put your main code here, to run repeatedly:
  if (manager.waitAvailableTimeout (3000))
	{
		// Wait for a message addressed to us from the client
		uint8_t len = sizeof(buf);
		uint8_t from;
		if (manager.recvfromAck((uint8_t *)buf, &len, &from))
		{
			Console.print ("from @");
			Console.print (from, DEC);
			Console.print (" R[");
			Console.print (len);
			Console.print ("]<");
			//Console.print (buf);
      for (int i = 0; i < len; i++){
         Console.print(buf[i], BIN);
         Console.print(" ");
      }
      Console.println();
			Console.print (">(");
			Console.print (driver.lastRssi(), DEC);
			Console.print ("dBm) ---> send reply to @");
			Console.print (from, DEC);
			Console.println (" ---> ");
			Console.flush();
		}
    
	}
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

void serialLogDouble(String prefix, double decimalValue)
{
  if (SERIAL_DEBUG)
  {
    Console.print(prefix);
    Console.print(" ");
    Console.println(decimalValue);
  }
}

void toggleLED(int LED_PIN) {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

