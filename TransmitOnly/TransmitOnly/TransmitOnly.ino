// includes
#include <RH_RF95.h>
#include <RHMesh.h>
#include <TinyGPS++.h>
#include <TimeLib.h>

// Radio Pins
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3  

// I/O Pins
#define RED_LED_PIN A0
#define GREEN_LED_PIN A1
#define VBAT_PIN A7
#define NOISE_SEED_PIN A4

// Packet constants
#define PACKET_SIZE_BYTES 16
#define SLEEP_TIME 10000            // time between transmissions
#define SLEEP_TIME_VARIANCE 2000    // randomness in sleep time to prevent two transmitters from consistently colliding
#define GPS_TIME_ALLOWABLE_AGE 500  // how old (in ms) the GPS time is allowed to be when syncing with the system clock

// Baud rate constants
#define DEBUG_BAUD 9600
#define GPS_BAUD 38400 // This baud rate is variable depending on the GPS module

// Debug
#define SERIAL_DEBUG true
#define Console SerialUSB
#define DISABLE_STANDBY true
#define SIMULATE_PACKET true

// Network defining the beacon ID's
const uint16_t Basestation = 1;
const uint16_t Beacon1 = 2;
const uint16_t Beacon2 = 3;
const uint16_t Beacon3 = 4;
const uint16_t Beacon4 = 5;

// Radio configurations
const uint16_t MyAddress = Beacon1; // Change beacon number depending on beacon used.
const float Frequency = 915.0;
const int8_t TxPower = 7;

// Singletons
RH_RF95 driver(RFM95_CS, RFM95_INT);
RHMesh manager(driver, MyAddress);
TinyGPSPlus gps;

void setup() {
  // Serial setups
  Serial1.begin(GPS_BAUD);
  if (SERIAL_DEBUG) {
    Console.begin(DEBUG_BAUD);
    while (!Console) {}

    serialLog("Serial Initialized");
    serialLogInteger("Address: ", MyAddress);
  }

  // Pin setups
  pinMode(RFM95_RST, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Reset pin outputs
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);

  // manually reset rf95
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);  

  serialLog("Initializing radio...");
  if (!manager.init()) {
    serialLog("Radio initializing failed");
    while (true) {
      toggleLED(RED_LED_PIN);
      delay(1000);
    }
  }

  driver.setFrequency(Frequency);
	driver.setTxPower(TxPower);

  if (!SIMULATE_PACKET) {
    serialLog("Waiting for GPS lock...");
    while (!gps.location.isValid()) {
      waitingForLock();
    }
    serialLog("GPS obtained lock");
  }

  else {
    serialLog("Skipping GPS lock");
  }

  //set random seed using noise from analoge
  uint32_t noise = analogRead(NOISE_SEED_PIN);
  randomSeed(noise);
  serialLogInteger("Setting random seed with noise:", noise);

  serialLog("Setup Complete\n");
}

uint8_t message[PACKET_SIZE_BYTES];
int8_t messageID = 0;

void loop() {
  encodeMessage();
  manager.sendto((uint8_t *)message, PACKET_SIZE_BYTES, Basestation);
  toggleLED(GREEN_LED_PIN);
  delay(10);
  toggleLED(GREEN_LED_PIN);
  uint16_t waitTime =  random(SLEEP_TIME - SLEEP_TIME_VARIANCE, SLEEP_TIME + SLEEP_TIME_VARIANCE);
  serialLogInteger("Waiting for", waitTime);
  smartDelay(waitTime);
}

// Delay function that can read incoming GPS information during the delay time
static void smartDelay(uint16_t ms)
{
  unsigned long start = millis();
  while ((millis() - start) < ms)
  {
    //get data from GPS
    while (Serial1.available() > 0)
    {
      gps.encode(Serial1.read());
    }
  }
}

void waitingForLock() {
  toggleLED(GREEN_LED_PIN);
  smartDelay(1000);
  toggleLED(GREEN_LED_PIN);
  smartDelay(1000);
}

void encodeMessage() {
  /*
  packet structure:
  - 16 bit radio ID
  - 1 bit panic state
  - 7 bit message ID
  - 32 bit latitude
  - 32 bit longitude
  - 8 bit battery life
  - 32 bit UTC

  Total size: 128 bits or 16 bytes
  */

  int byteIndex = 0;

  bool panicState = true;
  float gpsLat = 100.;
  float gpsLong = 200.;
  uint8_t batteryPercent = 50;
  int32_t utc = now();

  if (!SIMULATE_PACKET) {
    // TODO: override simulated packet numbers here if using GPS
  }

  // Log packet before sending
  serialLogInteger("Radio ID:", MyAddress);
  serialLogInteger("Panic State:", panicState);
  serialLogInteger("Message ID:", messageID);
  serialLogDouble("GPS latitude:", gpsLat);
  serialLogDouble("GPS longitude:", gpsLong);
  serialLogInteger("Battery Percent:", batteryPercent);
  serialLogInteger("Timestamp:", utc);

  // encode radio ID
  for (int i=sizeof(MyAddress)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&MyAddress)[i];
    byteIndex++;
  }

  // encode panicState and messageID
  uint8_t panicMask = panicState ? 0b10000000 : 0b00000000;
  message[byteIndex] = messageID | panicMask;
  byteIndex++;

  // encode gpsLat
  for (int i = sizeof(gpsLat)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&gpsLat)[i];
    byteIndex++;
  }

  // encode gpsLong
  for (int i = sizeof(gpsLong)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&gpsLong)[i];
    byteIndex++;
  }

  // encode batteryPercent
  message[byteIndex] = batteryPercent;
  byteIndex++;

  // encode utc
  for (int i = sizeof(utc)-1; i>=0; i--)
  {
    //cast data to byte array then get byte by index
    message[byteIndex] = ((uint8_t*)&utc)[i];
    byteIndex++;
  }
}

void toggleLED(int LED_PIN) {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

void serialLog(String message) {
  if (SERIAL_DEBUG) {
    Console.println(message);
  }
}

void serialLogInteger(String prefix, long intValue)
{
  if (SERIAL_DEBUG)
  {
    Console.print(prefix);
    Console.print(" ");
    Console.println(intValue);
  }
}

void serialLogDouble(String prefix, double decimalValue)
{
  if (SERIAL_DEBUG)
  {
    Console.print(prefix);
    Console.print(" ");
    Console.println(decimalValue);
  }
}
