#include "I2C.h"
#include <Arduino.h>
#include <Wire.h>

char SCLPin = 15;
char SDAPin = 7;

void SetI2CWires(char Type, char Input){
    if (Type == SCLPIN)
    {
        SDAPin = Input;
    }
    if (Type == SCLPIN)
    {
        SCLPin = Input;
    }
}
void I2CStart(){
    Wire.begin(SDAPin, SCLPin);
}

void I2CRun(){

}

void I2CwriteReg(char Address, char Reg){
  // Write to user register on ADDRESS
  Wire.beginTransmission(Address);
  Wire.write(Reg);
  Wire.endTransmission();
}

void I2CwriteValue(char Address, char Reg, char value){
  // Write to user register on ADDRESS
  Wire.beginTransmission(Address);
  Wire.write(Reg);
  Wire.write(value);
  Wire.endTransmission();
}

char I2CreadReg(char Address, char Reg){
  // Read from user register on ADDRESS
  Wire.beginTransmission(Address);
  Wire.write(Reg);
  Wire.endTransmission();
  Wire.requestFrom(Address,1);
  uint8_t regVal = Wire.read();
  return regVal;
}

char I2Cread(char Address, char ByteCount, char returnData[]){
  // Read from user register on ADDRESS
  char i = 0;
  Wire.requestFrom(Address,ByteCount);
  if(Wire.available() != ByteCount)
    return -1;
  for(i = 0; i < ByteCount; ++i){
    returnData[i] = Wire.read();
  }
  return 1;
}

char I2CReadWrite(char Address, char SendCount, char sendData[], char ByteCount, char returnData[]){
  // Read from user register on ADDRESS
  char i = 0;
  Wire.beginTransmission(Address);
  for(i = 0; i < SendCount; ++i){
    Wire.write(sendData[i]);
  }
  Wire.endTransmission();
  Wire.requestFrom(Address,ByteCount);
  if(Wire.available() != ByteCount)
    return -1;
  for(i = 0; i < ByteCount; ++i){
    returnData[i] = Wire.read();
  }
  return 1;
}

uint8_t I2CScanAddrs(uint8_t* addrs, uint8_t maxCount) {
    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 127 && found < maxCount; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
            addrs[found++] = addr;
    }
    return found;
}

const char* I2CDeviceName(uint8_t addr) {
    struct { uint8_t addr; const char* name; } known[] = {
        { 0x3C, "OLED SSD1306"   },
        { 0x3D, "OLED SSD1306"   },
        { 0x38, "AHT20"          },
        { 0x40, "HDC2080/SI7021" },
        { 0x44, "SHT31"          },
        { 0x45, "SHT31"          },
        { 0x48, "ADS1115"        },
        { 0x49, "ADS1115"        },
        { 0x4A, "ADS1115"        },
        { 0x4B, "ADS1115"        },
        { 0x57, "EEPROM AT24"    },
        { 0x60, "MCP4725 DAC"    },
        { 0x68, "DS3231/MPU6050" },
        { 0x76, "BME/BMP280"     },
        { 0x77, "BME/BMP280"     },
        { 0x20, "PCF8574/MCP23017" },
        { 0x21, "PCF8574/MCP23017" },
        { 0x22, "PCF8574/MCP23017" },
        { 0x23, "PCF8574/MCP23017" },
        { 0x24, "PCF8574/MCP23017" },
        { 0x25, "PCF8574/MCP23017" },
        { 0x26, "PCF8574/MCP23017" },
        { 0x27, "PCF8574/MCP23017" },
    };
    for (auto& e : known)
        if (e.addr == addr) return e.name;

    // Unknown — format into a static buffer
    static char _unk[8];
    snprintf(_unk, sizeof(_unk), "0x%02X", addr);
    return _unk;
}

void I2CScan(){
  byte error, address;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4) {
      Serial.print("Unknow error at address 0x");
      if (address<16) {
        Serial.print("0");
      }
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.println("done\n");
  }   
}