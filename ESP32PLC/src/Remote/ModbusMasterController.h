#pragma once
#include <HardwareSerial.h>
#include <SimpleModbusMaster.h>
#include "ModbusDevice.h"

#define MB_MAX_DEVICES    8
#define MB_MAX_PACKETS   (MB_MAX_DEVICES * 2 + 1)  // read + write slots + 1 queue slot
#define MB_MAX_REGS       128
#define MB_QUEUE_DEPTH    4
#define MB_QUEUE_REG_MAX  8    // max registers per queued write command

class ModbusMasterController {
public:
    ModbusMasterController(HardwareSerial& serial,
                           long    baud,
                           uint8_t rxPin,
                           uint8_t txPin,
                           uint8_t txEnablePin,
                           long    timeout = 2000,
                           long    minPoll = 200);

    // ----------------------------------------------------------------
    // Setup — call addDevice() for each slave, then begin()
    // configureWrite() on a device must happen before addDevice()
    // ----------------------------------------------------------------
    bool addDevice(ModbusDevice& device);
    void begin();

    // ----------------------------------------------------------------
    // Runtime — single call every loop()
    // ----------------------------------------------------------------
    void update();

    // ----------------------------------------------------------------
    // Lookup by slave address
    // ----------------------------------------------------------------
    ModbusDevice* findByAddress(uint8_t address);

    // ----------------------------------------------------------------
    // Ad-hoc writes — no device object required
    // owner = optional device to receive onWriteComplete callback
    // Returns false if queue is full (depth 4)
    // ----------------------------------------------------------------
    bool queueWrite     (uint8_t address, uint16_t remoteReg, uint16_t value,
                         ModbusDevice* owner = nullptr);
    bool queueWriteCoil (uint8_t address, uint16_t remoteReg, bool     value,
                         ModbusDevice* owner = nullptr);
    bool queueWriteMulti(uint8_t address, uint16_t remoteReg,
                         const uint16_t* values, uint8_t count,
                         ModbusDevice* owner = nullptr);

private:
    struct WriteCmd {
        uint8_t       address             = 0;
        uint8_t       function            = 0;
        uint16_t      remoteReg           = 0;
        uint8_t       count               = 0;
        uint16_t      values[MB_QUEUE_REG_MAX] = {};
        ModbusDevice* owner               = nullptr;
        bool          valid               = false;
    };

    HardwareSerial* _serial;
    long     _baud;
    uint8_t  _rxPin, _txPin, _txEnablePin;
    long     _timeout, _minPoll;

    ModbusDevice* _devices[MB_MAX_DEVICES] = {};
    uint8_t       _deviceCount             = 0;

    Packet   _packets[MB_MAX_PACKETS]      = {};
    uint8_t  _packetCount                  = 0;

    unsigned int _regs[MB_MAX_REGS]        = {};
    uint16_t     _nextRegBase              = 0;

    // Circular write queue
    WriteCmd _queue[MB_QUEUE_DEPTH]        = {};
    uint8_t  _queueHead                    = 0;
    uint8_t  _queueTail                    = 0;
    uint8_t  _queueCount                   = 0;

    // Queue packet state
    uint8_t       _queuePacketIdx          = 0;
    uint16_t      _queueRegBase            = 0;
    bool          _queueWriteInFlight      = false;
    ModbusDevice* _activeQueueOwner        = nullptr;
    unsigned int  _lastQueueWriteSuccess   = 0;
    unsigned int  _lastQueueWriteFailed    = 0;

    uint8_t  _allocPacket();
    uint16_t _allocRegs(uint8_t n);
    bool     _enqueue(const WriteCmd& cmd);
    void     _fireCallbacks();
    void     _processQueue();
};
