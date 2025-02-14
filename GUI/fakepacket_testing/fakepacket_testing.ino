// Packet format structures
struct MeshPacket {
  uint8_t radioId;          // 8 bits
  uint16_t messageIdWithPanic; // 16 bits (1 bit panic + 15 bits message ID)
  uint32_t latitude;        // 32 bits
  uint32_t longitude;       // 32 bits
  uint8_t batteryLife;      // 8 bits
  uint32_t utcTime;         // 32 bits
};

struct LegacyPacket {
  uint16_t radioId;         // 16 bits
  uint8_t messageIdWithPanic; // 8 bits (1 bit panic + 7 bits message ID)
  uint32_t latitude;        // 32 bits
  uint32_t longitude;       // 32 bits
  uint8_t batteryLife;      // 8 bits
  uint32_t utcTime;         // 32 bits
};

// Union to share memory between both packet formats
union PacketUnion {
  MeshPacket mesh;
  LegacyPacket legacy;
  uint8_t raw[16]; // Maximum size between both formats
};

// Class to quickly generate a packet of either format
class PacketGenerator {
private:
  PacketUnion packet;
  bool Format1;
  
public:
  PacketGenerator() : Format1(true) {
    memset(&packet, 0, sizeof(PacketUnion));
  }
  
  void setFormat(bool format) {
    Format1 = format;
  }
  
  void generatePacket(
    uint16_t radioId,
    bool panicState,
    uint16_t messageId,
    float latitude,
    float longitude,
    uint8_t batteryLife,
    uint32_t utcTime
  ) {
    if (Format1) {
      // Format 1: Mesh Packet : 8-bit Radio ID, 1-bit panic + 15-bit message ID
      packet.mesh.radioId = radioId & 0xFF;
      packet.mesh.messageIdWithPanic = (panicState ? 0x8000 : 0) | (messageId & 0x7FFF);
      packet.mesh.latitude = *((uint32_t*)&latitude);
      packet.mesh.longitude = *((uint32_t*)&longitude);
      packet.mesh.batteryLife = batteryLife;
      packet.mesh.utcTime = utcTime;
    } else {
      // Format 2: Legacy Packet : 16-bit Radio ID, 1-bit panic + 7-bit message ID
      packet.legacy.radioId = radioId;
      packet.legacy.messageIdWithPanic = (panicState ? 0x80 : 0) | (messageId & 0x7F);
      packet.legacy.latitude = *((uint32_t*)&latitude);
      packet.legacy.longitude = *((uint32_t*)&longitude);
      packet.legacy.batteryLife = batteryLife;
      packet.legacy.utcTime = utcTime;
    }
  }
  
  // Get pointer to raw packet data
  const uint8_t* getRawPacket() const {
    return packet.raw;
  }
  
  // Get packet size based on current format
  size_t getPacketSize() const {
    return Format1 ? sizeof(MeshPacket) : sizeof(LegacyPacket);
  }
  
  // Print packet for debugging
  void printPacket() {
    Serial.println("Packet Contents:");
    for (size_t i = 0; i < getPacketSize(); i++) {
      if (packet.raw[i] < 0x10) Serial.print("0");
      Serial.print(packet.raw[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
};

// Example usage
PacketGenerator packetGen;
unsigned long lastPacketTime = 0;
const unsigned long PACKET_INTERVAL = 5000; // Send a packet every 5 seconds

// Helper function to generate random float between min and max
float randomFloat(float min, float max) {
  return min + (random(0, 1000000) / 1000000.0) * (max - min);
}

// Helper function to generate random radio ID based on MSB
uint16_t generateRadioId(bool format1) {
  if (format1) {
    // For Mesh Packet: Generate 8-bit ID starting with 1
    return 0x80 | random(0x80); // 1xxxxxxx
  } else {
    // For Legacy Packet: Generate 16-bit ID starting with 0
    return random(0x8000); // 0xxxxxxxxxxxxxxx
  }
}

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0)); // Initialize random number generator
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastPacketTime >= PACKET_INTERVAL) {
    // Randomly choose format
    bool useFormat = random(2) == 1;

    // Generate random radio ID based on MSB
    uint16_t radioId = generateRadioId(useFormat);

    // Set format of packet based on MSB of radioId
    packetGen.setFormat(radioId & 0x8000);
    
    // Generate random test data
    bool panicState = random(2) == 1;
    uint16_t messageId = useFormat ? random(32768) : random(128); // 15 or 7 bits based on format
    float latitude = randomFloat(37.0, 38.0);
    float longitude = randomFloat(-81.0, -80.0);
    uint8_t batteryLife = random(101); // 0-100%
    uint32_t utcTime = random(); // Random timestamp
    
    // Generate and send packet
    packetGen.generatePacket(
      radioId,
      panicState,
      messageId,
      latitude,
      longitude,
      batteryLife,
      utcTime
    );
    
     // Send the byte array over serial
    Serial.write(packetGen.getRawPacket(), 17);

    Serial.flush();

    
    lastPacketTime = currentTime;
  }
}