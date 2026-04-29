#ifndef REMOTEDEVICETYPE_H
#define  REMOTEDEVICETYPE_H

#include <Arduino.h>

/* class BitRef
{
public:
    BitRef& operator=( bool x )
    {
        *byteRef = (*byteRef & ~(1 << bitPos));
        if (x) *byteRef = *byteRef | (1 << bitPos);
        return *this;
    }
    //BitRef& operator=( const BitRef& x );

    operator bool() const 
    {
        if (*byteRef & (1 << bitPos)) return true;
        return false;
    }
public:
    BitRef(uint8_t *ref, int pos)
    {
        byteRef = ref;
        bitPos = pos;
    }
private:
    uint8_t *byteRef;
    int bitPos;
};

typedef union {
    uint64_t uint64;
    uint32_t uint32[2]; 
    uint16_t uint16[4];
    uint8_t  uint8[8];
    int64_t int64;
    int32_t int32[2]; 
    int16_t int16[4];
    int8_t  int8[8];

    //deprecated names used by older code
    uint64_t value;
    struct {
        uint32_t low;
        uint32_t high;
    };
    struct {
        uint16_t s0;
        uint16_t s1;
        uint16_t s2;
        uint16_t s3;
    };
    uint8_t bytes[8];
    uint8_t byte[8]; //alternate name so you can omit the s if you feel it makes more sense
    struct {
        uint8_t bitField[8];
        const bool operator[]( int pos ) const
        {
            if (pos < 0 || pos > 63) return 0;
            int bitFieldIdx = pos / 8;
            return (bitField[bitFieldIdx] >> pos) & 1;
        }
        BitRef operator[]( int pos )
        {
            if (pos < 0 || pos > 63) return BitRef((uint8_t *)&bitField[0], 0);
            uint8_t *ptr = (uint8_t *)&bitField[0]; 
            return BitRef(ptr + (pos / 8), pos & 7);
        }
    } bit;
} BytesUnion;

class MODBUS_TX_FRAME
{
    //MODBUS_TX_FRAME();

    BytesUnion data;    // 64 bits - lots of ways to access it.
    unsigned char id;        // 29 bit if ide set, 11 bit otherwise
    unsigned char RegType;
    unsigned char length;     // Number of data bytes
    void *Returnfcn;
}; */

struct MODBUS_TX_TX_FRAME {
  unsigned char id;        // 29 bit if ide set, 11 bit otherwise
  unsigned char RegType;
  unsigned char length;     // Number of data bytes
  unsigned char Data[40];
  void *Returnfcn;
};

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
  LSHoldingRegisters = 3,
  VIN = 1,
  Sensor1 = 4,
  Sensor2 = 5,
  Sensor3 = 6
};

enum TempSensorDefine {
  TSSensorCode = 0x10,
  TSCoil = 4,
  TSInputRegisters = 6,
  TSHoldingRegisters = 3,
  TSVIN = 1,
  TSSensor1 = 4,
  TSSensor2 = 5,
  TSSensor3 = 6
};

enum CurrentSensorDefine {
  CSSensorCode = 0x10,
  CSCoil = 4,
  CSInputRegisters = 6,
  CSHoldingRegisters = 3,
  CSVIN = 1,
  CSSensor1 = 4,
  CSSensor2 = 5,
  CSSensor3 = 6
};


struct LeakSensor {
  unsigned char LeakSensorAddress = 0;
  struct RemoteSensorChannel Coil[LeakSensorDefine::LSCoil];
  struct RemoteSensorChannel InputRegisters[LeakSensorDefine::LSInputRegisters];
  struct RemoteSensorChannel HoldingRegisters[LeakSensorDefine::LSHoldingRegisters];
};

struct TempSensor {
  unsigned char LeakSensorAddress = 0;
  struct RemoteSensorChannel Coil[LeakSensorDefine::LSCoil];
  struct RemoteSensorChannel InputRegisters[LeakSensorDefine::LSInputRegisters];
  struct RemoteSensorChannel HoldingRegisters[LeakSensorDefine::LSHoldingRegisters];
};

struct CurrentSensor {
  unsigned char LeakSensorAddress = 0;
  struct RemoteSensorChannel Coil[LeakSensorDefine::LSCoil];
  struct RemoteSensorChannel InputRegisters[LeakSensorDefine::LSInputRegisters];
  struct RemoteSensorChannel HoldingRegisters[LeakSensorDefine::LSHoldingRegisters];
};

#endif