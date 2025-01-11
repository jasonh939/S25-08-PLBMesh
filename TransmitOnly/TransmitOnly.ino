/*
* Filename: TransmitOnly.ino
* Author: S25-08
*/

// includes
#include <TimeLib.h>
#include "TransmitOnly.hpp"
#include "IO/IO.hpp"
#include "Debug.hpp"

// Packet constants
#define PACKET_SIZE_BYTES 16
#define SLEEP_TIME 10000            // time between transmissions
#define SLEEP_TIME_VARIANCE 2000    // randomness in sleep time to prevent two transmitters from consistently colliding
#define GPS_TIME_ALLOWABLE_AGE 500  // how old (in ms) the GPS time is allowed to be when syncing with the system clock

// Battery constants
/*
* The IDE doesn't recognize A7 as a pin value. 
* The pin's other name '9' freezes the program when doing analogRead. (Possible due to faulty battery port??)
* We will have to attach VBAT pin to pin A5 to get the voltage reading
*/
#define VBATPIN A5 
// TODO: might need to change threshold values if we switch batteries
#define BATTERY_MIN_THRESHOLD 134
#define BATTERY_MAX_THRESHOLD 511

#define SIMULATE_PACKET false
#define NOISE_SEED_PIN A4

// Address of basestation as well as PLB
const uint8_t Basestation = 1;
const uint8_t MyAddress = 3; // NOTE: Change the address based on the PLB address

// Packet info
byte message[PACKET_SIZE_BYTES];
int16_t messageID = 0;

// Active mode state
Active_State currState = TRANSMIT;

void setup() {
  initDebug(true); // Set parameter to false if you don't want to enable console outputs or test GPS 

  serialLog("Setting up IO...");
  initIO();
  serialLog("IO setup complete\n");

  manager.setThisAddress(MyAddress);

  // RNG will be used to change the interval of transmissions.
  uint32_t noise = analogRead(NOISE_SEED_PIN);
  randomSeed(noise);
  serialLogInteger("Setting random seed with noise:", noise);
  serialLogBool("Initial standby: ", standby);
  serialLogBool("Initial panic: ", panic);

  pinMode(VBATPIN, INPUT);
}

void loop() {
  updateLEDs();

  if (standby) {
    standbyMode();
  }
  
  else {
    activeMode();
  }
}

// Changes yellow (standby) and/or red (panic) LED if status changed
void updateLEDs() {
  panic ? turnOnLED(RED_LED_PIN) : turnOffLED(RED_LED_PIN);
  standby ? turnOnLED(YEL_LED_PIN) : turnOffLED(YEL_LED_PIN);
}

// Standby mode keeps gps lock and sleeps radio module
void standbyMode() {
  driver.sleep();
  smartDelay(1);  
}

/*
* This function deals with the PLB in active mode.
* 20 second cycle: Transmit -> ACK (5s) -> Downtime (15 +- 2s) -> Repeat
*/
void activeMode() {
  switch(currState) {
    case TRANSMIT:
      handleTransmit();
      break;
    
    case ACK:
      handleACK();
      break;
    
    case IDLE:
      handleIdle();
      break;
  }
}

// Transmits a single packet to the base station
void handleTransmit() {
  serialLog("Transmit State");
  encodeMessage();
  serialLogPacket(message, PACKET_SIZE_BYTES);

  // senttoWait is slightly blocking
  serialLog("Sending packet...");
  if (manager.sendtoWait((uint8_t *)message, PACKET_SIZE_BYTES, Basestation) == RH_ROUTER_ERROR_NONE) {
    serialLog("Packet successfully sent");
    turnOnLED(GRE_LED_PIN);
    delay(10);
    turnOffLED(GRE_LED_PIN);
  }

  else {
    serialLog("Packet failed to send\n");
    turnOnLED(YEL_LED_PIN);
    delay(10);
    turnOffLED(YEL_LED_PIN);
  }

  currState = ACK;
}

// Wait for an ACK from the base station
// Early function exit if standby is pressed
// NOTE: There's a small bug when spam toggling the standby switch it restarts the arduino.
//  Not sure if that is bad connections, software bug, or faulty switch.
void handleACK() {
  // TODO: implement ACK
  serialLog("ACK State");
  unsigned long start = millis();
  while ((millis() - start) < ACK_INTERVAL) {
    updateLEDs();
    if (standby) {
      serialLog("Standby detected, early ACK exit");
      return;
   }
   smartDelay(1);
  }
  currState = IDLE;
}

// Downtime between ACK and next packet transmit. Keeps a GPS lock
// Early function exit if standby is pressed
void handleIdle() {
  serialLog("Idle State");
  unsigned long start = millis();
  uint16_t waitTime = random(IDLE_INTERVAL - IDLE_VARIANCE, IDLE_INTERVAL + IDLE_VARIANCE);
  while ((millis() - start) < waitTime) {
    updateLEDs();
    if (standby) {
      serialLog("Standby detected, early idle exit.");
      return;
    }
    smartDelay(1);
  }

  currState = TRANSMIT;
}

void encodeMessage() {
  /*
  NEW packet structure:
  - 8 bit radio ID
  - 1 bit panic state
  - 15 bit message ID
  - 32 bit latitude
  - 32 bit longitude
  - 8 bit battery life
  - 32 bit UTC

  Total size: 128 bits or 16 bytes
  */

  int byteIndex = 0;

  float gpsLat = 100.;
  float gpsLng = 200.;
  uint8_t batteryPercent = 50;
  uint32_t utc = now();

  if (!SIMULATE_PACKET) {
    gpsLat = gps.location.lat();
    gpsLng = gps.location.lng();

    // TODO: fix battery reading issue
    float batteryReading = analogRead(VBATPIN);
    batteryReading *= 2;
    batteryReading *= 3.3;
    batteryReading /= 1024;
    serialLogDouble("VBAT:", batteryReading);
    // batteryPercent = map(batteryReading, BATTERY_MIN_THRESHOLD, BATTERY_MAX_THRESHOLD, 0, 100);
    // batteryPercent = max(batteryReading, 0);
    // batteryPercent = min(batteryReading, 100);

    // TODO: implement time sync
  }

  // Log packet before sending
  serialLogInteger("Radio ID:", MyAddress);
  serialLogInteger("Panic State:", panic);
  serialLogInteger("Message ID:", messageID);
  serialLogDouble("GPS latitude:", gpsLat);
  serialLogDouble("GPS longitude:", gpsLng);
  serialLogInteger("Battery Percent:", batteryPercent);
  serialLogInteger("Timestamp:", utc);

  // encode radio ID
  for (int i = sizeof(MyAddress)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&MyAddress)[i];
    byteIndex++;
  }

  // encode panicState and messageID
  uint16_t panicMask = messageID | (panic ? 0x8000 : 0x0000);
  for (int i = sizeof(panicMask)-1; i>=0; i--) {
    message[byteIndex] = ((uint8_t*)&panicMask)[i];
    byteIndex++;
  } 

  // encode gpsLat
  for (int i = sizeof(gpsLat)-1; i>=0; i--)
  {
    message[byteIndex] = ((uint8_t*)&gpsLat)[i];
    byteIndex++;
  }

  // encode gpsLong
  for (int i = sizeof(gpsLng)-1; i>=0; i--)
  {
    message[byteIndex] = ((uint8_t*)&gpsLng)[i];
    byteIndex++;
  }

  // encode batteryPercent
  message[byteIndex] = batteryPercent;
  byteIndex++;

  // encode utc
  for (int i = sizeof(utc)-1; i>=0; i--)
  {
    message[byteIndex] = ((uint8_t*)&utc)[i];
    byteIndex++;
  }

  messageID++;
}
