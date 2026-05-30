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