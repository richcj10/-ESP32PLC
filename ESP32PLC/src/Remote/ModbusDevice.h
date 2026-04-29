#pragma once
#include <stdint.h>
#include <SimpleModbusMaster.h>

class ModbusMasterController;

class ModbusDevice {
public:
    // address   = Modbus slave ID
    // function  = READ_INPUT_REGISTERS / READ_HOLDING_REGISTERS etc.
    // remoteReg = register start address on the slave
    // count     = number of registers to read
    ModbusDevice(uint8_t address, uint8_t function,
                 uint16_t remoteReg, uint8_t count);

    // ----------------------------------------------------------------
    // Identity & poll config
    // ----------------------------------------------------------------
    uint8_t       getAddress()           const;
    void          setPollRate(unsigned long ms);
    unsigned long getPollRate()          const;

    // ----------------------------------------------------------------
    // Status
    // ----------------------------------------------------------------
    bool     isResponding()    const;
    uint32_t totalRequests()   const;
    uint32_t failedRequests()  const;
    uint32_t successRequests() const;

    // ----------------------------------------------------------------
    // Read register access
    // ----------------------------------------------------------------
    uint16_t getRaw   (uint8_t index) const;   // unsigned, e.g. type field, wind dir
    int16_t  getSigned(uint8_t index) const;   // two's-complement, e.g. temperatures
    float    getFloat (uint8_t index) const;   // getSigned / 100.0
    bool     getCoil  (uint8_t index) const;   // bit n of packed FC1/FC2 coil data

    // ----------------------------------------------------------------
    // Write — multi-register (pre-configured slot)
    // configureWrite() must be called before master.addDevice()
    // Stage values with setWriteReg(), then arm with sendWrite()
    // ----------------------------------------------------------------
    void configureWrite(uint8_t function, uint16_t remoteReg, uint8_t count);
    void setWriteReg(uint8_t index, uint16_t value);
    void sendWrite();

    // ----------------------------------------------------------------
    // Write — single-shot convenience (routed via master queue)
    // ----------------------------------------------------------------
    void writeSingleReg(uint16_t remoteReg, uint16_t value);  // FC6
    void writeCoil     (uint16_t remoteReg, bool     value);  // FC5

    // ----------------------------------------------------------------
    // Callbacks
    // ----------------------------------------------------------------
    using DataCb   = void (*)(ModbusDevice*);
    using StatusCb = void (*)(ModbusDevice*, bool online);
    using WriteCb  = void (*)(ModbusDevice*, bool success);

    void onDataReceived (DataCb   cb);
    void onStatusChange (StatusCb cb);
    void onWriteComplete(WriteCb  cb);

private:
    uint8_t       _address;
    uint8_t       _readFunction;
    uint16_t      _remoteReg;
    uint8_t       _count;
    unsigned long _pollRate = 0;

    // Read packet — pointer assigned by master in addDevice()
    Packet*       _readPacket = nullptr;
    unsigned int* _regs       = nullptr;

    // Write packet — assigned only if configureWrite() was called before addDevice()
    bool          _writeConfigured = false;
    bool          _hasWritePacket  = false;
    Packet*       _writePacket     = nullptr;
    uint8_t       _writeFunction   = 0;
    uint16_t      _writeRemoteReg  = 0;
    uint8_t       _writeCount      = 0;
    unsigned int* _writeRegs       = nullptr;

    // Change-detection state — compared each update()
    unsigned int _lastReadSuccess  = 0;
    unsigned int _lastReadFailed   = 0;
    unsigned int _lastWriteSuccess = 0;
    unsigned int _lastWriteFailed  = 0;
    uint8_t      _consecutiveFails = 0;
    bool         _responding       = false;

    DataCb   _onDataReceived  = nullptr;
    StatusCb _onStatusChange  = nullptr;
    WriteCb  _onWriteComplete = nullptr;

    ModbusMasterController* _master = nullptr;

    friend class ModbusMasterController;
};
