#ifndef MOUSE_H
#define MOUSE_H

typedef int BOOL;

BOOL MouseOpen(void);
void MouseClose(void);
void MouseMove(char Button, char X, char Y, char Wheel);

#endif