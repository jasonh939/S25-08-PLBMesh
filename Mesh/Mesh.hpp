/*
* Filename: Mesh.hpp
* Author: S25-08
*/

#ifndef MESH_H
#define MESH_H

// Packet constants
#define PACKET_SIZE_BYTES 16
#define ACK_SIZE_BYTES 3

// Time intervals for various timers
#define GPS_INTERVAL 1000
#define ACK_INTERVAL 5000
#define RANGE_EXTENDER_INTERVAL 5000
#define IDLE_INTERVAL 5000
#define IDLE_VARIANCE 1000

// Enumerator for active mode states
enum _Active_State {
  TRANSMIT,
  ACK,
  RANGE_EXTENDER,
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
void handleRangeExtender();
void handleACK();
void handleIdle();

// Function to encode message
void encodeMessage();

// Function to repackage legacy packets
void repackLegacy();

#endif