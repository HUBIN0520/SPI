#ifndef __SERIAL_H
#define __SERIAL_H

#include "main.h"
#include <stdint.h>

void Serial_SendByte(uint8_t Byte);
void Serial_SendString(char *String);
void Serial_Printf(char *format, ...);

#endif
