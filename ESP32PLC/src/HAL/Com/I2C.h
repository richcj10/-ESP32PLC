#ifndef I2C_H
#define  I2C_H

#define SCLPIN 1
#define SDAPIN 2

void SetI2CWires(char Type, char Input);
void I2CStart();
void I2CRun();
void I2CwriteReg(char Address, char Reg);
void I2CwriteValue(char Address, char Reg, char value);
char I2CreadReg(char Address, char Reg);
char I2Cread(char Address, char ByteCount, char returnData[]);
char I2CReadWrite(char Address, char SendCount, char sendData[], char ByteCount, char returnData[]);

void I2CScan();

#endif 