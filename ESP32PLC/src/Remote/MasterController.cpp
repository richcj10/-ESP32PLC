#include "MasterController.h"
#include "RemoteDeviceType.h"
#include <Arduino.h>

#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>

char MasterStart(){
  Serial1.begin(38400);
  if (!ModbusRTUClient.begin(38400)) {
      Serial.println("Failed to start Modbus RTU Client!");
       return 0;
  }
  return 1;
}

int ReadDeviceType(int Address) {
  int DeviceType = 0;
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
  }
  return DeviceType;
}

float readDeviceVIN(int Address){
  if (!ModbusRTUClient.requestFrom(Address, INPUT_REGISTERS, 0x02, 1)) {
    return -1;
  } else {
    while (ModbusRTUClient.available()) {
      return (float)ModbusRTUClient.read()/100.00;
    }
  }
  return -1;
}

char SetOcupyLED(int Address,unsigned char R,unsigned char G, unsigned char B) {
  ModbusRTUClient.beginTransmission(Address, HOLDING_REGISTERS, 0x02, 2);
  int HighByte = R;
  int LowByte = G<<8 | B;
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
  }

  // Alternatively, to read a single Discrete Input value use:
  // ModbusRTUClient.discreteInputRead(...)
}