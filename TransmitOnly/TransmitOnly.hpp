/*
* Filename: TransmitOnly.hpp
* Author: S25-08
*/

#ifndef TRANSMIT_ONLY_H
#define TRANSMIT_ONLY_H

// Enumerator for active mode states
enum _Active_State {
  TRANSMIT,
  ACK,
  IDLE
};
typedef enum _Active_State Active_State;

// Standby mode and handle functions
void standbyMode();

// Active mode and handle functions
void activeMode();
void handleTransmit();
void handleACK();
void handleIdle();

// Function to encode message
void encodeMessage();

#endif