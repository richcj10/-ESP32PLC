#include "ModbusMasterController.h"
#include <Arduino.h>

ModbusMasterController::ModbusMasterController(HardwareSerial& serial,
                                               long baud,
                                               uint8_t rxPin, uint8_t txPin,
                                               uint8_t txEnablePin,
                                               long timeout, long minPoll)
    : _serial(&serial), _baud(baud),
      _rxPin(rxPin), _txPin(txPin), _txEnablePin(txEnablePin),
      _timeout(timeout), _minPoll(minPoll) {}

bool ModbusMasterController::addDevice(ModbusDevice& device) {
    if (_deviceCount >= MB_MAX_DEVICES) return false;

    uint8_t  slotsNeeded = 1 + (device._writeConfigured ? 1 : 0);
    uint16_t regsNeeded  = device._count + (device._writeConfigured ? device._writeCount : 0);

    if (_packetCount + slotsNeeded >= MB_MAX_PACKETS)   return false;
    if (_nextRegBase  + regsNeeded  > MB_MAX_REGS)       return false;

    device._master     = this;
    device._readPacket = &_packets[_allocPacket()];
    device._regs       = &_regs[_allocRegs(device._count)];

    if (device._writeConfigured) {
        device._writePacket  = &_packets[_allocPacket()];
        device._writeRegs    = &_regs[_allocRegs(device._writeCount)];
        device._hasWritePacket = true;
    }

    _devices[_deviceCount++] = &device;
    return true;
}

void ModbusMasterController::begin() {
    // Reserve queue slot after all devices so it doesn't eat device budget
    _queuePacketIdx = _allocPacket();
    _queueRegBase   = _allocRegs(MB_QUEUE_REG_MAX);

    for (uint8_t i = 0; i < _deviceCount; i++) {
        ModbusDevice* d = _devices[i];

        uint16_t readBase = (uint16_t)(d->_regs - _regs);
        modbus_construct(d->_readPacket, d->_address, d->_readFunction,
                         d->_remoteReg, d->_count, readBase);
        d->_readPacket->polling_interval = d->_pollRate;

        if (d->_hasWritePacket) {
            uint16_t writeBase = (uint16_t)(d->_writeRegs - _regs);
            modbus_construct(d->_writePacket, d->_address, d->_writeFunction,
                             d->_writeRemoteReg, d->_writeCount, writeBase);
            d->_writePacket->connection = 0;  // disabled until sendWrite() arms it
        }
    }

    _packets[_queuePacketIdx].connection = 0;

    modbus_uart_mod(_rxPin, _txPin);
    modbus_configure(_serial, _baud, SERIAL_8N1, _timeout, _minPoll,
                     0, _txEnablePin, _packets, _packetCount, _regs);
}

void ModbusMasterController::update() {
    modbus_update();
    _fireCallbacks();
    _processQueue();
}

ModbusDevice* ModbusMasterController::findByAddress(uint8_t address) {
    for (uint8_t i = 0; i < _deviceCount; i++) {
        if (_devices[i]->_address == address) return _devices[i];
    }
    return nullptr;
}

bool ModbusMasterController::queueWrite(uint8_t address, uint16_t remoteReg,
                                         uint16_t value, ModbusDevice* owner) {
    WriteCmd cmd;
    cmd.address   = address;
    cmd.function  = PRESET_SINGLE_REGISTER;
    cmd.remoteReg = remoteReg;
    cmd.count     = 1;
    cmd.values[0] = value;
    cmd.owner     = owner;
    cmd.valid     = true;
    return _enqueue(cmd);
}

bool ModbusMasterController::queueWriteCoil(uint8_t address, uint16_t remoteReg,
                                             bool value, ModbusDevice* owner) {
    WriteCmd cmd;
    cmd.address   = address;
    cmd.function  = FORCE_SINGLE_COIL;
    cmd.remoteReg = remoteReg;
    cmd.count     = 1;
    cmd.values[0] = value ? COIL_ON : COIL_OFF;
    cmd.owner     = owner;
    cmd.valid     = true;
    return _enqueue(cmd);
}

bool ModbusMasterController::queueWriteMulti(uint8_t address, uint16_t remoteReg,
                                              const uint16_t* values, uint8_t count,
                                              ModbusDevice* owner) {
    if (count > MB_QUEUE_REG_MAX) return false;
    WriteCmd cmd;
    cmd.address   = address;
    cmd.function  = PRESET_MULTIPLE_REGISTERS;
    cmd.remoteReg = remoteReg;
    cmd.count     = count;
    for (uint8_t i = 0; i < count; i++) cmd.values[i] = values[i];
    cmd.owner     = owner;
    cmd.valid     = true;
    return _enqueue(cmd);
}

uint8_t ModbusMasterController::_allocPacket() {
    return _packetCount++;
}

uint16_t ModbusMasterController::_allocRegs(uint8_t n) {
    uint16_t base = _nextRegBase;
    _nextRegBase += n;
    return base;
}

bool ModbusMasterController::_enqueue(const WriteCmd& cmd) {
    if (_queueCount >= MB_QUEUE_DEPTH) return false;
    _queue[_queueTail] = cmd;
    _queueTail = (_queueTail + 1) % MB_QUEUE_DEPTH;
    _queueCount++;
    return true;
}

void ModbusMasterController::_fireCallbacks() {
    for (uint8_t i = 0; i < _deviceCount; i++) {
        ModbusDevice* d  = _devices[i];
        Packet*       rp = d->_readPacket;

        // --- Read success ---
        if (rp->successful_requests != d->_lastReadSuccess) {
            d->_lastReadSuccess  = rp->successful_requests;
            d->_consecutiveFails = 0;
            if (!d->_responding) {
                d->_responding = true;
                if (d->_onStatusChange) d->_onStatusChange(d, true);
            }
            if (d->_onDataReceived) d->_onDataReceived(d);
        }

        // --- Read failure → offline after 3 consecutive misses ---
        if (rp->failed_requests != d->_lastReadFailed) {
            d->_lastReadFailed = rp->failed_requests;
            if (++d->_consecutiveFails >= 3 && d->_responding) {
                d->_responding = false;
                if (d->_onStatusChange) d->_onStatusChange(d, false);
            }
        }

        // --- Device-owned write packet ---
        if (d->_hasWritePacket) {
            Packet* wp = d->_writePacket;

            if (wp->successful_requests != d->_lastWriteSuccess) {
                d->_lastWriteSuccess = wp->successful_requests;
                wp->connection = 0;
                if (d->_onWriteComplete) d->_onWriteComplete(d, true);
            } else if (wp->failed_requests != d->_lastWriteFailed) {
                d->_lastWriteFailed = wp->failed_requests;
                wp->connection = 0;
                if (d->_onWriteComplete) d->_onWriteComplete(d, false);
            }
        }
    }
}

void ModbusMasterController::_processQueue() {
    Packet* qp = &_packets[_queuePacketIdx];

    // Check if in-flight write has completed
    if (_queueWriteInFlight) {
        bool succeeded = (qp->successful_requests != _lastQueueWriteSuccess);
        bool failed    = (qp->failed_requests     != _lastQueueWriteFailed);

        if (succeeded || failed) {
            _lastQueueWriteSuccess = qp->successful_requests;
            _lastQueueWriteFailed  = qp->failed_requests;
            qp->connection         = 0;
            _queueWriteInFlight    = false;

            if (_activeQueueOwner && _activeQueueOwner->_onWriteComplete) {
                _activeQueueOwner->_onWriteComplete(_activeQueueOwner, succeeded);
            }
            _activeQueueOwner = nullptr;
        }
        return;  // wait for current write to finish before popping next
    }

    if (_queueCount == 0) return;

    // Pop next command and fire it
    WriteCmd& cmd = _queue[_queueHead];
    _queueHead  = (_queueHead + 1) % MB_QUEUE_DEPTH;
    _queueCount--;

    for (uint8_t i = 0; i < cmd.count && i < MB_QUEUE_REG_MAX; i++) {
        _regs[_queueRegBase + i] = cmd.values[i];
    }

    modbus_construct(qp, cmd.address, cmd.function,
                     cmd.remoteReg, cmd.count, _queueRegBase);
    qp->connection = 1;

    _activeQueueOwner      = cmd.owner;
    _lastQueueWriteSuccess = qp->successful_requests;
    _lastQueueWriteFailed  = qp->failed_requests;
    _queueWriteInFlight    = true;
}
