#include "I2C.h"
#include <Arduino.h>
#include <Wire.h>

static const uint8_t SCLPin = 15;
static const uint8_t SDAPin = 7;

void I2CStart(){
    Wire.begin(SDAPin, SCLPin);
}







uint8_t I2CScanAddrs(uint8_t* addrs, uint8_t maxCount) {
    uint8_t found = 0;
    for (uint8_t addr = 1; addr < 127 && found < maxCount; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
            addrs[found++] = addr;
    }
    return found;
}

const char* I2CDeviceName(uint8_t addr) {
    struct { uint8_t addr; const char* name; } known[] = {
        { 0x3C, "OLED SSD1306"   },
        { 0x3D, "OLED SSD1306"   },
        { 0x38, "AHT20"          },
        { 0x40, "HDC2080/SI7021" },
        { 0x44, "SHT31"          },
        { 0x45, "SHT31"          },
        { 0x48, "ADS1115"        },
        { 0x49, "ADS1115"        },
        { 0x4A, "ADS1115"        },
        { 0x4B, "ADS1115"        },
        { 0x57, "EEPROM AT24"    },
        { 0x60, "MCP4725 DAC"    },
        { 0x68, "DS3231/MPU6050" },
        { 0x76, "BME/BMP280"     },
        { 0x77, "BME/BMP280"     },
        { 0x20, "PCF8574/MCP23017" },
        { 0x21, "PCF8574/MCP23017" },
        { 0x22, "PCF8574/MCP23017" },
        { 0x23, "PCF8574/MCP23017" },
        { 0x24, "PCF8574/MCP23017" },
        { 0x25, "PCF8574/MCP23017" },
        { 0x26, "PCF8574/MCP23017" },
        { 0x27, "PCF8574/MCP23017" },
    };
    for (auto& e : known)
        if (e.addr == addr) return e.name;

    // Unknown — format into a static buffer
    static char _unk[8];
    snprintf(_unk, sizeof(_unk), "0x%02X", addr);
    return _unk;
}

