#ifndef REMOTEDEVICETYPE_H
#define  REMOTEDEVICETYPE_H

struct RemoteSensorChannel {
    unsigned char Code = 0;
    unsigned char Type = 0; 
    char Name[50];
};

struct RemoteDevice {
  unsigned char RemoteDeviceCount = 0;
  unsigned char RemoteDeviceAddress[128];
  unsigned char RemoteDeviceType[128];
  unsigned char DeviceTypeDefined[7] = {0x10,0x20,0x30,0x40,0x41,0x42,0x50};
};

enum LeakSensorDefine {
  SensorCode = 0x10,
  LSCoil = 4,
  LSInputRegisters = 6,
  LSHoldingRegisters = 3
};

struct LeakSensor {
  unsigned char LeakSensorAddress = 0;
  struct RemoteSensorChannel Coil[LeakSensorDefine::LSCoil];
  struct RemoteSensorChannel InputRegisters[LeakSensorDefine::LSInputRegisters];
  struct RemoteSensorChannel HoldingRegisters[LeakSensorDefine::LSHoldingRegisters];
};

#endif