#ifndef I2C_H
#define  I2C_H

#define SCLPIN 1
#define SDAPIN 2

void SetI2CWires(char Type, char Input);
void I2CStart();
void I2CRun();
void I2CScan();

#endif 