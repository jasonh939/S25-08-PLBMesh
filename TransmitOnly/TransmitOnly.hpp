/*
* Filename: TransmitOnly.hpp
* Author: S25-08
*/

#ifndef TRANSMIT_ONLY_H
#define TRANSMIT_ONLY_H

// Packet constants
#define PACKET_SIZE_BYTES 16
#define ACK_SIZE_BYTES 3

// Time intervals for various timers
#define GPS_INTERVAL 1000
#define ACK_INTERVAL 5000
#define IDLE_INTERVAL 15000
#define IDLE_VARIANCE 1000

// Testing lng and lat values
#define TEST_LAT_MIN 37.22
#define TEST_LAT_MAX 37.24
#define TEST_LNG_MIN -80.43
#define TEST_LNG_MAX -80.41

// Enumerator for active mode states
enum _Active_State {
  TRANSMIT,
  ACK,
  IDLE
};
typedef enum _Active_State Active_State;

// Function to update status LEDs
void updateLEDs();

// Function to obtain a GPS lock
void waitForLock();

// Standby mode function
void standbyMode();

// Active mode and handle functions
void activeMode();
void handleTransmit();
void handleACK();
void handleIdle();

// Function to encode message
void encodePacket();

#endif