#ifndef EEPROM_H
#define  EEPROM_H

#define EEPROM_ADDRESS 0x50

#define EEPROM_CONFIG_TYPE 0x01
#define EEPROM_CONFIG_PCBVER 0x02
#define EEPROM_CONFIG_DOMFG 0x03


void EEPROMStart();
void EEPROMGetBoardConfig();
void EEPROMGetPCBConfig();
void EEPROMGetPCBDOMFG();

#endif 