#ifndef JOYSTICK_H
#define  JOYSTICK_H

#define JOYSTICK_UP 1
#define JOYSTICK_DOWN 2
#define JOYSTICK_LEFT 3
#define JOYSTICK_RIGHT 4
#define JOYSTICK_NONE 0
#define JOYSTICK_ERROR -1
#define EEPROM_CONFIG_DOMFG 0x03


void JoyStickStart();
void JoyStickUpdate();
char GetJoyStickPos();
char GetJoyStickSelect();

#endif 