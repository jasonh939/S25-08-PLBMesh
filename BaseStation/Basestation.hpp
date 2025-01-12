/*
* Filename: Basestation.hpp
* Author: S25-08
*/

#ifndef BASESTATION_H
#define BASESTATION_H

#define VALID_PACKET_SIZE 16
#define MAX_ACK_MESSAGE_LEN 100
#define BS_TIMEOUT_INTERVAL 5000

// Function to determine if packet recieved is driver or mesh
bool isMeshPacket();

// Function to parse out message ID from packet
uint16_t getMessageID();

#endif