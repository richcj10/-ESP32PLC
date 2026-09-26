#ifndef I2C_H
#define  I2C_H
#include <stdint.h>


void I2CStart();


// Scans bus, fills addrs[] with responding addresses. Returns count found.
uint8_t      I2CScanAddrs(uint8_t* addrs, uint8_t maxCount);
// Returns a human-readable name for a known address, or "0xXX" for unknowns.
const char*  I2CDeviceName(uint8_t addr);

#endif 