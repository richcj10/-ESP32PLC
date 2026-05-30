#ifndef RS485_H
#define  RS485_H

#define RX 1
#define TX 2

void SetUARTWires(char Type, char Input);
void RS485Start();
void RS485Run();

#endif 