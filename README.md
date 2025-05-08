S25-08 Software
===============

This repository contains the Personal Locator Beacons (PLB) code for all 4 modes (Transmit only, Range Extender, Mesh, and Base Station) and the GUI code for the Major Design Experience Project.

(It also contains a copy of the LegacyTransmitOnly mode)


Hardware Overview
---
### Operation
- The power switch is located on the bottom.
- The panic switch is located in the middle.
- The standby switch is located at the top.
- A flashing Green LED signifies the GPS trying to obtain a lock.
- A solid Yellow LED signifies the PLB is currently in standby mode.
- A solid Red LED signifies that panic mode is currently on.
- A quick blink from the Green LED signifies a successfully sent packet.
- A quick blink from the Yellow LED signifies an unsuccessful packet transmission.
- PLB must be outside for GPS to obtain a lock


Software Overview
--------------
### Setting up Arduino IDE
1. Libraries
    - RadioHead by Mike McCauley
    - Time by Michael Margolis
    - TinyGPSPlus by Mikal Hart
2. Helping Arduino see the Featherboard
    - In IDE, go to file -> preferences -> Additional boards manager URLs -> Copy and Paste: https://adafruit.github.io/arduino-board-index/package_adafruit_index.json into field.

### How to flash a beacon as a specific mode
All beacons are able to be flashed in 1 of 4 modes: Transmit-only, Range-extender, Mesh-mode, and the Base Station. Each mode has a seperate folder for its respective code. To flash a beacon as a specific mode:
1. Connect the board to your machine via micro-usb and switch on the beacon.
2. Open file of respective mode you want to switch. i.e mesh.ino, TransmitOnly.ino, Basestation.ino, etc..
3. Select board and flash code onto it.
4. Wait for successful response

### How to set up GUI
1. Connect beacon flashed as the BaseStation to a machine via micro-usb
2. Open GUI folder in IDE that is able to execute Python code
3. Run gui_manager.py and select the Arduino from the serial port drop down menu.
4. Wait for PLB beacons to obtain a lock on. Beacons will automatically begin transmission of location to base station.
