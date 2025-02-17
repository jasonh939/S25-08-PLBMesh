Personal Locator Beacon Mesh Network



Synopsis 

Cell service in remote areas can be difficult to find. In the case where an individual must communicate their location with a base of operations, this project sets up a suitable network of wireless communication.
DIFFERENT MODES
////

Installation


Hardware

The necessary hardware that is required to run this software is the following. 1: Arduino Feather M0 with the RFM95C Lora Module. 2: Beitian BE-880 GPS. 3: 3.7V 2500mAh battery. 4: 900 MHz to 935 MHz Stubby Antenna, Monopole, SMA Male Connector.

Software

Arduino IDE
Libraries within Arduino IDE:
RadioHead by MikeMcCauley
Time by Michael Margolis
TinyGPSPlus by Mikal Hart
// GUI DEPENDENCIES // (Python3+, PyQt5)

Descriptions of Hardware
//


Usage
There are several configurations within this repository. Here's how to use them.

Peripheral Test: Flash Arduino with PeripheralTest.ino.
BaseStation: Flash Arduino with BaseStation.ino. Leave BaseStation plugged into computer.
GUI: Run GUI.py.

Modes (and what they mean)

LegacyTransmitOnly: Flash Arduino with LegacyTransmitOnly.ino.
Mesh: Flash Arduino with Mesh.ino.
RangeExtender: Flash Arduino with RangeExtender.ino.
TransmitOnly: Flash Arduino with TransmitOnly.ino.

As you can see, to run any one of the configurations, simply flash an Arduino with the desired mode. Once each Arduino has been flashed (besides the BaseStation), they can be unplugged and used wirelessly.
The GUI is coincident to the BaseStation, so running the GUI and the BaseStation from the same device is necessary to get real-time updates from PLBs.


Demo.