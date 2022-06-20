#ifndef SI7021_H
#define  SI7021_H

float ESgetRH();
float ESreadTemp();
float ESgetTemp();
float ESreadTempF();
float ESgetTempF();
float Si7021getTemp();
void Si7021heaterOn();
void Si7021heaterOff();
void Si7021changeResolution(char i);
void Si7021reset();
char Si7021checkID();
int Si7021makeMeasurment(char command);

#define ADDRESS      0x40

#define TEMP_MEASURE_HOLD  0xE3
#define HUMD_MEASURE_HOLD  0xE5
#define TEMP_MEASURE_NOHOLD  0xF3
#define HUMD_MEASURE_NOHOLD  0xF5
#define TEMP_PREV   0xE0
#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE
#define HTRE        0x02

#define CRC_POLY 0x988000 // Shifted Polynomial for CRC check

// Error codes
#define I2C_TIMEOUT   998
#define BAD_CRC   999

#endif 