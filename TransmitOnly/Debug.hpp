/*
* Filename: Debug.hpp
* Author: S25-08
*/

#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_BAUD 9600
#define Console SerialUSB

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

#endif