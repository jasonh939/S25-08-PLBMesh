/*
* Filename: RangeExtender.hpp
* Author: S25-08
*/

#ifndef RANGE_EXTENDER_H
#define RANGE_EXTENDER_H

// Function to update status LEDs
void updateLEDs();

// Standby and active mode functions
void standbyMode();
void activeMode();

// Function to repackage legacy packets
void repackLegacy();

#endif