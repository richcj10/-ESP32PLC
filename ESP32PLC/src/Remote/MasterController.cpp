#include "MasterController.h"
#include "RemoteDeviceType.h"
#include <Arduino.h>
#include <SimpleModbusMaster.h>


//////////////////// Port information ///////////////////
#define baud 38400
#define timeout 100
#define polling 100 // the scan rate
#define retry_count 2

// used to toggle the receive/transmit pin on the driver
#define TxEnablePin 20 

#define LED 9

// The total amount of available memory on the master to store data
#define TOTAL_NO_OF_REGISTERS 3

// This is the easiest way to create new packets
// Add as many as you want. TOTAL_NO_OF_PACKETS
// is automatically updated.
unsigned int regs[9];
unsigned int write_regs[3];

enum
{
  PACKET1,
  PACKET2,
  TOTAL_NO_OF_PACKETS // leave this last entry
};

// Create an array of Packets to be configured
Packet packets[TOTAL_NO_OF_PACKETS];

char RemoteStart(){
  modbus_uart_mod(16,17);
  // Initialize each packet
  //modbus_construct(&packets[PACKET1], 1, READ_HOLDING_REGISTERS, 0, 1, 0);
  //modbus_construct(&packets[PACKET2], 1, PRESET_MULTIPLE_REGISTERS, 1, 1, 0);
  
  // Initialize the Modbus Finite State Machine
  modbus_configure(&Serial1, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
  return 1;
}

void RemoteRun(){
  modbus_update();
}

int ReadDeviceType(int Address) {
   int DeviceType = 0;
/*
  Serial.print("Reading Discrete Input values ... ");
  // read 10 Discrete Input values from (slave) id 42, address 0x00
  if (!ModbusRTUClient.requestFrom(Address, INPUT_REGISTERS, 0x01, 1)) {
    Serial.print("failed! ");
    Serial.println(ModbusRTUClient.lastError());
  } 
  else {
    Serial.println("success");
    while (ModbusRTUClient.available()) {
      DeviceType = ModbusRTUClient.read();
      Serial.print(ModbusRTUClient.read());
      Serial.print(' ');
    }
    Serial.println();
  } */
  return DeviceType;
}

float readDeviceVIN(int Address){
/*   if (!ModbusRTUClient.requestFrom(Address, INPUT_REGISTERS, 0x02, 1)) {
    return -1;
  } else {
    while (ModbusRTUClient.available()) {
      return (float)ModbusRTUClient.read()/100.00;
    }
  } */
  return -1;
}

char SetOcupyLED(int Address,unsigned char R,unsigned char G, unsigned char B) { 
 //ModbusRTUClient.beginTransmission(Address, HOLDING_REGISTERS, 0x02, 2);
  regs[0] = 12;
  regs[1] = R;
  regs[2] = G<<8 | B;
  //Serial.println("Occuply LED");
  
  /*  
  ModbusRTUClient.write(HighByte);
  ModbusRTUClient.write(LowByte);
  if (!ModbusRTUClient.endTransmission()) {
    Serial.print("failed! ");
    Serial.println(ModbusRTUClient.lastError());
    return 0;
  }
  ModbusRTUClient.beginTransmission(Address, HOLDING_REGISTERS, 0x01, 1);
  ModbusRTUClient.write(12);
  if (!ModbusRTUClient.endTransmission()) {
    Serial.print("failed! ");
    Serial.println(ModbusRTUClient.lastError());
    return 0;
  } else {
    return 1;
  } */

  // Alternatively, to read a single Discrete Input value use:
  // ModbusRTUClient.discreteInputRead(...)
  modbus_construct(&packets[PACKET1], 0x10, PRESET_SINGLE_REGISTER, 0x02, 2, 1);
  //Serial.println("MB Send");
  modbus_construct(&packets[PACKET2], 0x10, PRESET_SINGLE_REGISTER, 0x01, 1, 0);
  return 0;
}