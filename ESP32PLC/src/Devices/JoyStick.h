#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>

#define JOYSTICK_NONE  0
#define JOYSTICK_UP    1
#define JOYSTICK_DOWN  2
#define JOYSTICK_LEFT  3
#define JOYSTICK_RIGHT 4
#define JOYSTICK_ERROR -1

struct JoyMapEntry { int lo; int hi; };

struct JoyMap {
    JoyMapEntry none;
    JoyMapEntry up;
    JoyMapEntry down;
    JoyMapEntry left;
    JoyMapEntry right;
};

void   JoyStickStart();
void   JoyStickUpdate();
char   GetJoyStickPos();
char   GetJoyStickSelect();
void   GetJoystickPrint(char x);

int      JoyStickRawAvg();
void     JoyStickSaveMap();
void     JoyStickLoadMap();
JoyMap   JoyStickGetMap();
void     JoyStickSetMap(const JoyMap& m);

#endif
