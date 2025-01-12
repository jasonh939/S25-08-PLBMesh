/*
* Filename: Basestation.hpp
* Author: S25-08
*/

#ifndef BASESTATION_H
#define BASESTATION_H

#define MAX_ACK_MESSAGE_LEN 10
#define BS_TIMEOUT_INTERVAL 5000

// Function to determine if packet recieved is driver or mesh
bool isMeshPacket();

#endif