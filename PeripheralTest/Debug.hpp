/*
* Filename: Debug.hpp
* Author: S25-08
*/

#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_BAUD 9600
#define Console Serial

bool SerialDebug = true;

// Initializes the serial to print out debug messages
void initDebug(bool enableDebug) {
  if (enableDebug) {
    Console.begin(DEBUG_BAUD);
    while (!Console) {} 
    Console.println("Console Initialized");   
  }

  else {
    SerialDebug = false;
  }
}

// Allows console to print out generic message
void serialLog(String message) {
  if (SerialDebug) {
    Console.println(message);
  }
}

// Allows console to print out message with an int value
void serialLogInteger(String prefix, long intValue, String suffix = "")
{
  if (SerialDebug)
  {
    Console.print(prefix);
    Console.print(" ");
    Console.print(intValue);
    Console.println(suffix);
  }
}

// Allows console to print out message with a double value
void serialLogDouble(String prefix, double decimalValue, String suffix = "")
{
  if (SerialDebug)
  {
    Console.print(prefix);
    Console.print(" ");
    Console.print(decimalValue, 5);
    Console.println(suffix);
  }
}

// Allows console to print out message with a boolean value
void serialLogBool(String prefix, bool boolValue, String suffix = "")
{
  if (SerialDebug)
  {
    Console.print(prefix);
    Console.print(" ");
    Console.print(boolValue);
    Console.println(suffix);
  }
}

// Helper function for serialLogPacket
void serialLogByte(byte byteValue)
{
  if (SerialDebug)
  {
    for (int bit = 7; bit >= 0; bit--) {
      if (bitRead(byteValue, bit) == 0) {
        Console.print("0");
      }

      else {
        Console.print("1");
      }
    }
    Console.print(" ");
  }
}

// Allows console to print out the packet data
void serialLogPacketBin(byte packet[], int size) {
  if (SerialDebug) {
    Console.print("Packet Data: ");
    for (int i = 0; i < size; i++) {
      serialLogByte(packet[i]);
    }
  }
  Console.println();
}

// Allows console to print out packet data in a readable form. Doesn't print if packet has invalid size
void serialLogPacketRead(byte packet[], int size) {
  if (SerialDebug) {
    if (size != 16) {
      serialLog("Invalid packet size. Skipping print");
      return;
    }  
    
    int byteIndex = 0;

    uint8_t radioID;
    bool panic;
    uint16_t msgID;
    float gpsLat, gpsLng;
    uint8_t batteryPercent;
    uint32_t utc;

    // determine if the beacon packet is legacy or new
    bool isLegacy = (packet[byteIndex] & 0b10000000) ? false : true;
    if (isLegacy) { // decode radio ID, panic state, and msgID
      radioID = ((packet[byteIndex] & 0b01111111) << 8) | packet[byteIndex + 1];
      byteIndex += 2;

      msgID = packet[byteIndex];
      byteIndex++;     
    }

    else {
      radioID = packet[byteIndex] & 0b01111111;
      byteIndex++;

      panic = (packet[byteIndex] & 0b10000000) ? true : false;
      msgID = ((packet[byteIndex] & 0b01111111) << 8) | packet[byteIndex + 1];
      byteIndex += 2;
    }

    // decode latitude
    for (int i = 3; i >= 0; i--) {
      ((uint8_t*)&gpsLat)[i] = packet[byteIndex];
      byteIndex++;
    }

    // decode longtitude
    for (int i = 3; i >= 0; i--) {
      ((uint8_t*)&gpsLng)[i] = packet[byteIndex];
      byteIndex++;
    }

    // decode battery percent
    batteryPercent = packet[byteIndex];
    byteIndex++;

    // decode time
    for (int i = 3; i >= 0; i--) {
      ((uint8_t*)&utc)[i] = packet[byteIndex];
      byteIndex++;
    }

    serialLogInteger("Radio ID:", radioID);
    serialLogInteger("Panic State:", panic);
    serialLogInteger("Message ID:", msgID);
    serialLogDouble("GPS latitude:", gpsLat);
    serialLogDouble("GPS longitude:", gpsLng);
    serialLogInteger("Battery Percent:", batteryPercent);
    serialLogInteger("Timestamp:", utc);  
  }
}

#endif