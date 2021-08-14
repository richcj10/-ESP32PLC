#include "DeviceConfig.h"
#include "DeviceConfigPvt.h"
#include <Arduino.h>
#include "Com/RS485.h"
#include "Com/I2C.h"

int DeviceType = 0;
String TypeName = "";

int GetDeviceType(){
  return DeviceType;
}

void SetDeviceType(int value){
  DeviceType = value;
}

char QueryLocalDevice(){
  //Find the local device using board ID, this will most likly be one wire device. 
  DeviceType = 1; //Place holder till code is done. 

  return 1;
}

   String name;
   char IO4;
   char IO5;
   char IO12;
   char IO13;
   char IO14;
   char IO15;
   char IO16;
   char IO17;
   char IO18;
   char IO19;
   char IO21;
   char IO22;
   char IO23;
   char IO25;
   char IO26;
   char IO27;
   char IO32;
   char IO34;
   char IO35;
   char IO36;
   char IO39;

char GPIOStart(){
  int Type = GetDeviceType() - 1;
  int IOCount = 0;
  if(Type < HOW_MANY_IO_TYPES){
    TypeName = inv.ESP32PLCIOCountInvent[Type].name;
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO4, IOCount);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO5, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO12, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO13, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO14, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO15, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO16, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO17, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO18, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO19, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO21, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO22, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO23, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO25, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO26, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO27, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO32, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO34, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO35, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO36, IOCount++);
    SetIOType(inv.ESP32PLCIOCountInvent[Type].IO39, IOCount++);
    return 1;
  }
  else{
    //Got board ID out of range, Don't try to get info
    return -1;
  }
}

void SetIOType(char IOValue, char IOLocation){
  switch (IOValue){
  case UNUSED:
    LOG("Unused IO\n");
    /* code */
    break;
  case IOINPUT:
    LOG("Input IO\n");
    //LOG("%s",IOArray[IOLocation]);
    pinMode(IOArray[IOLocation], INPUT);
    /* code */
    break;
  case IOOUTPUT:
    LOG("Output IO\n");
    //LOG("%s",IOArray[IOLocation]);
    pinMode(IOArray[IOLocation], OUTPUT);
    /* code */
    break;
  case UART1TX:
    LOG("UART TX IO\n");
    //LOG("%s",IOArray[IOLocation]);
    SetUARTWires(TX, IOArray[IOLocation]);
    /* code */
    break;
  case UART1RX:
    LOG("UART TX IO\n");
    //LOG("%s",IOArray[IOLocation]);
    SetUARTWires(RX, IOArray[IOLocation]);
    break;
  case IOSCL:
    LOG("I2C SCL IO\n");
    //LOG("%s",IOArray[IOLocation]);
    SetI2CWires(SCLPIN, IOArray[IOLocation]);
    break;
  case IOSDA:
    LOG("I2C SDA IO\n");
    //LOG("%s",IOArray[IOLocation]);
    SetI2CWires(SDAPIN, IOArray[IOLocation]);
    break;
  case NEOPIXEL:
    LOG("Neopixel IO\n");
    //LOG("%s",IOArray[IOLocation]);
    break;
  case ONEWIRE:
    LOG("OneWore IO\n");
    //LOG("%s",IOArray[IOLocation]);
    break;
  default:
    LOG("Bad IO Config\n");
    break;
  }

}

String GetBoardName(){
  return TypeName;
}