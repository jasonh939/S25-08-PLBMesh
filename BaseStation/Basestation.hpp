/*
* Filename: Basestation.hpp
* Author: S25-08
*/

#ifndef BASESTATION_H
#define BASESTATION_H

#define VALID_PACKET_SIZE 16
#define ACK_SIZE_BYTES 3
#define BS_TIMEOUT_INTERVAL 5000

// Function to determine if packet recieved is driver or mesh
bool isMeshPacket();

// Functions to parse out data from packet
uint8_t getRadioID();
uint16_t getMessageID();

// Function to encode the ACK packet
void encodeAck();

#endif