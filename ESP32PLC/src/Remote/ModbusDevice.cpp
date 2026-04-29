#include "ModbusDevice.h"
#include "ModbusMasterController.h"

ModbusDevice::ModbusDevice(uint8_t address, uint8_t function,
                            uint16_t remoteReg, uint8_t count)
    : _address(address), _readFunction(function),
      _remoteReg(remoteReg), _count(count) {}

uint8_t ModbusDevice::getAddress() const { return _address; }

void ModbusDevice::setPollRate(unsigned long ms) {
    _pollRate = ms;
    if (_readPacket) _readPacket->polling_interval = ms;
}

unsigned long ModbusDevice::getPollRate() const { return _pollRate; }
bool          ModbusDevice::isResponding()  const { return _responding; }

uint32_t ModbusDevice::totalRequests()   const { return _readPacket ? _readPacket->requests            : 0; }
uint32_t ModbusDevice::failedRequests()  const { return _readPacket ? _readPacket->failed_requests     : 0; }
uint32_t ModbusDevice::successRequests() const { return _readPacket ? _readPacket->successful_requests : 0; }

uint16_t ModbusDevice::getRaw(uint8_t index) const {
    if (!_regs || index >= _count) return 0;
    return (uint16_t)_regs[index];
}

int16_t ModbusDevice::getSigned(uint8_t index) const {
    return (int16_t)getRaw(index);
}

float ModbusDevice::getFloat(uint8_t index) const {
    return (float)getSigned(index) / 100.0f;
}

bool ModbusDevice::getCoil(uint8_t index) const {
    // FC1/FC2: library packs 16 coils per register word, LSB = coil 0
    return (getRaw(index / 16) >> (index % 16)) & 1;
}

void ModbusDevice::configureWrite(uint8_t function, uint16_t remoteReg, uint8_t count) {
    _writeConfigured = true;
    _writeFunction   = function;
    _writeRemoteReg  = remoteReg;
    _writeCount      = count;
}

void ModbusDevice::setWriteReg(uint8_t index, uint16_t value) {
    if (!_writeRegs || index >= _writeCount) return;
    _writeRegs[index] = value;
}

void ModbusDevice::sendWrite() {
    if (_writePacket) _writePacket->connection = 1;
}

void ModbusDevice::writeSingleReg(uint16_t remoteReg, uint16_t value) {
    if (_master) _master->queueWrite(_address, remoteReg, value, this);
}

void ModbusDevice::writeCoil(uint16_t remoteReg, bool value) {
    if (_master) _master->queueWriteCoil(_address, remoteReg, value, this);
}

void ModbusDevice::onDataReceived (DataCb   cb) { _onDataReceived  = cb; }
void ModbusDevice::onStatusChange (StatusCb cb) { _onStatusChange  = cb; }
void ModbusDevice::onWriteComplete(WriteCb  cb) { _onWriteComplete = cb; }
