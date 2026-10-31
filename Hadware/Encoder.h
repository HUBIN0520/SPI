#ifndef __Encoder_H
#define __Encoder_H
#include "main.h"

uint8_t Encoder_All_Function(void);

extern uint16_t raw14;
extern uint8_t mag;
extern uint8_t crc_rx;
extern uint8_t crc_calc;
extern uint32_t valid24;
extern int32_t cdeg;// 角度，单位为 0.01 

#endif // !__Encoder_H
