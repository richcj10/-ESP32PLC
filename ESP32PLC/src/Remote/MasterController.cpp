#include "MasterController.h"
#include "RemoteDeviceType.h"
#include <Arduino.h>
#include <SimpleModbusMaster.h>
#include "Devices/Log.h"


//////////////////// Port information ///////////////////
#define baud 38400
#define timeout 2000
#define polling 1500 // the scan rate
#define retry_count 0

// used to toggle the receive/transmit pin on the driver
#define TxEnablePin 20 

#define LED 9

// The total amount of available memory on the master to store data
#define TOTAL_NO_OF_REGISTERS 3

// This is the easiest way to create new packets
// Add as many as you want. TOTAL_NO_OF_PACKETS
// is automatically updated.
unsigned int regs[20];
unsigned int write_regs[3];

enum
{
  PACKET1,
  PACKET2,
  PACKET3,
  TOTAL_NO_OF_PACKETS // leave this last entry
};

// Create an array of Packets to be configured
Packet packets[TOTAL_NO_OF_PACKETS];

char RemoteStart(){
  modbus_uart_mod(16,17);
  modbus_construct(&packets[PACKET1], 32, READ_INPUT_REGISTERS, 3, 3, 1);
  modbus_construct(&packets[PACKET2], 48, READ_INPUT_REGISTERS, 0, 7, 4);
  modbus_construct(&packets[PACKET3], 5, READ_INPUT_REGISTERS, 1, 5, 12);
  modbus_configure(&Serial1, baud, SERIAL_8N1, timeout, polling, retry_count, TxEnablePin, packets, TOTAL_NO_OF_PACKETS, regs);
  return 1;
}

void RemoteRun(){
  modbus_update();
  //waiting_for_reply();
}

char ReadRemoteTemp(){
  Log(LOG,"Temp = %0.2f\r\n",regs[1]/100.0);
  Log(LOG,"Temp = %0.2f\r\n",regs[2]/100.0);
  Log(LOG,"Temp = %0.2f\r\n",regs[3]/100.0);
  return 1;
}

char ReadRemoteWeather(){
  Log(LOG,"Type = %f\r\n",regs[4]);
  Log(LOG,"Outside Temp = %0.2f\r\n",regs[5]/100.0);
  Log(LOG,"Outside Humid = %0.2f\r\n",regs[6]/100.0);
  Log(LOG,"Outside mC = %0.2f\r\n",regs[7]/100.0);
  Log(LOG,"Wind Speed = %0.2f\r\n",regs[8]/100.0);
  Log(LOG,"Wind Dir = %0.2f\r\n",regs[9]);
  //Log(LOG,"Temp = %0.2f\r\n",regs[10]/100.0);
  return 1;
}

char ReadRemoteCurrent(){
  Log(LOG,"Type = %f\r\n",regs[12]);
  Log(LOG,"Current A = %0.2f\r\n",regs[REMOTE_CS_A]/100.0);
  Log(LOG,"Current B = %0.2f\r\n",regs[REMOTE_CS_B]/100.0);
  Log(LOG,"Current C = %0.2f\r\n",regs[REMOTE_CS_C]/100.0);
  //Log(LOG,"Temp = %0.2f\r\n",regs[10]/100.0);
  return 1;
}

float GetRemoteDataFromQue(unsigned char x, bool Divide){
  if(Divide){
    return (float)regs[x]/100.00;
  }
  else{
    return regs[x];
  }
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
  //modbus_construct(&packets[PACKET1], 0x10, PRESET_SINGLE_REGISTER, 0x02, 2, 1);
  //Serial.println("MB Send");
  //modbus_construct(&packets[PACKET2], 0x10, PRESET_SINGLE_REGISTER, 0x01, 1, 0);
  return 0;
}


char GetRemoteTemp(char TempCH){
  char Channel = 0;
  if(TempCH == 1){
    Channel = TempSensorDefine::TSSensor1;
  }
  if(TempCH == 2){
    Channel = TempSensorDefine::TSSensor1;
  }
  if(TempCH == 3){
    Channel = TempSensorDefine::TSSensor1;
  }
  else{
    Channel = 5;
  }
  //regs[0] = 0;
  //modbus_construct(&packets[PACKET1], 0x20, READ_INPUT_REGISTERS, 4, 1, 0);
  return 0;
}