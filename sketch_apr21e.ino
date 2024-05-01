#include <EEPROM.h>
#include <Wire.h>

const int PAGE_SIZE = 16; // Define the page size (number of bytes per page)
const int NUM_PAGES = 16; // Define the number of pages
const int EEPROM_SIZE = PAGE_SIZE * NUM_PAGES; // Calculate the total EEPROM size

// EEPROM addresses
const int EEPROM_ADDR_1 = 0x50;
const int EEPROM_ADDR_2 = 0x00;
const int EEPROM_ADDR_3 = 0x00;

void clearEEPROM(int addr) {
  for (int i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(addr + i, 0);
  }
}

void writeTextToEEPROM(const char* text, int startAddr, int addr) {
  int eepromAddr = addr + startAddr;
  while (*text) {
    EEPROM.write(eepromAddr++, *text++);
  }
}

void dumpEEPROM(int addr) {
  Serial.println("Dumping EEPROM:");
  for (int page = 0; page < NUM_PAGES; page++) {
    Serial.print("0x");
    if (page < 16) Serial.print("0"); // add leading zero for single digit page numbers
    Serial.print(page * PAGE_SIZE, HEX); // print page start address in 0x0000 format
    Serial.print(" ");
    for (int offset = 0; offset < PAGE_SIZE; offset++) {
      int eepromAddr = addr + page * PAGE_SIZE + offset;
      byte value = EEPROM.read(eepromAddr);
      if (value < 0x10) {
        Serial.print("0");
      }
      Serial.print(value, HEX);
      Serial.print(" ");
    }

    // Print ASCII representation on the right side
    Serial.print("   "); // Add spacing
    for (int offset = 0; offset < PAGE_SIZE; offset++) {
      int eepromAddr = addr + page * PAGE_SIZE + offset;
      byte value = EEPROM.read(eepromAddr);
      char asciiChar = (value >= 32 && value <= 126) ? (char)value : '.';
      Serial.print(asciiChar);
    }
    
    Serial.println();
  }
}

void i2cScan(int bus) {
  Serial.print("Scanning I2C bus ");
  Serial.print(bus);
  Serial.println("...");
  Serial.println("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f");
  Wire.begin(bus);
  for (byte i = 0; i < 128; i += 16) {
    if (i < 16) Serial.print("0");
    Serial.print(i, HEX);
    Serial.print(":  ");
    for (byte j = i; j < i + 16; j++) {
      Wire.beginTransmission(j);
      byte error = Wire.endTransmission();
      if (error == 0) {
        Serial.print(j < 16 ? "0" : "");
        Serial.print(j, HEX);
      } else {
        Serial.print("--");
      }
      Serial.print(" ");
    }
    Serial.println();
  }
}

void i2cSet(int bus, int address, byte value) {
  Wire.beginTransmission(address);
  Wire.write(value);
  Wire.endTransmission();
  Serial.println("Value set successfully.");
}

void i2cGet(int bus, int address, int reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(address, 1);
  if (Wire.available()) {
    byte value = Wire.read();
    Serial.print("Value at address ");
    Serial.print(address, HEX);
    Serial.print(": ");
    Serial.println(value, HEX);
  } else {
    Serial.println("Error: No response from device.");
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Initialize default I2C bus

  while (!Serial); // Wait for serial port to connect

  // Clear EEPROMs
  clearEEPROM(EEPROM_ADDR_1);
  clearEEPROM(EEPROM_ADDR_2);
  clearEEPROM(EEPROM_ADDR_3);

  // Write text to EEPROMs
  const char* textToWrite = "your name";
  int startAddr = 0x0010;
  writeTextToEEPROM(textToWrite, startAddr, EEPROM_ADDR_1);
  //writeTextToEEPROM(textToWrite, startAddr, EEPROM_ADDR_2);
  //writeTextToEEPROM(textToWrite, startAddr, EEPROM_ADDR_3);

  Serial.println("Ready to receive command...");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    if (command.startsWith("i2cdetect -y ")) {
      int spaceIndex = command.indexOf(' ');
      if (spaceIndex != -1) {
        String busString = command.substring(spaceIndex + 1);
        int bus = busString.toInt();
        i2cScan(bus);
      }
    } else if (command.startsWith("i2cdump -y ")) {
      int spaceIndex = command.indexOf(' ');
      if (spaceIndex != -1) {
        int addrIndex = command.indexOf("0x", spaceIndex + 1);
        if (addrIndex != -1) {
          String addressString = command.substring(addrIndex);
          int address = strtol(addressString.c_str(), NULL, 16);
          dumpEEPROM(address);
        }
      }
    } else if (command.startsWith("i2cset -y ")) {
      int spaceIndex = command.indexOf(' ');
      if (spaceIndex != -1) {
        int addrIndex = command.indexOf("0x", spaceIndex + 1);
        if (addrIndex != -1) {
          int valueIndex = command.indexOf("0x", addrIndex + 1);
          if (valueIndex != -1) {
            int bus = command.substring(spaceIndex + 1, addrIndex).toInt();
            int address = strtol(command.substring(addrIndex, valueIndex).c_str(), NULL, 16);
            byte value = strtol(command.substring(valueIndex).c_str(), NULL, 16);
            i2cSet(bus, address, value);
          }
        }
      }
    } else if (command.startsWith("i2cget -y ")) {
      int spaceIndex = command.indexOf(' ');
      if (spaceIndex != -1) {
        int addrIndex = command.indexOf("0x", spaceIndex + 1);
        if (addrIndex != -1) {
          int regIndex = command.indexOf("0x", addrIndex + 1);
          if (regIndex != -1) {
            int bus = command.substring(spaceIndex + 1, addrIndex).toInt();
            int address = strtol(command.substring(addrIndex, regIndex).c_str(), NULL, 16);
            int reg = strtol(command.substring(regIndex).c_str(), NULL, 16);
            i2cGet(bus, address, reg);
          }
        }
      }
    } else {
      Serial.println("Invalid command.");
    }
  }
}
