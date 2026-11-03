#ifndef __Encoder_H
#define __Encoder_H
#include "main.h"

uint8_t Encoder_All_Function(void);

extern uint16_t raw14;
/*
 * mag 是 MT6701 返回的 4 位状态字段 Mg[3:0]：
 *
 * Mg[3]: 0 = 正常，1 = 超速
 * Mg[2]: 0 = 未按压，1 = 旋钮被按压
 * Mg[1:0] 是磁场状态编码：
 *   00 = 磁场正常
 *   01 = 磁场过强
 *   10 = 磁场过弱
 *   11 = 手册未定义
 *
 * OLED 使用十六进制显示时，含义如下：
 *   0: 0000 磁场正常
 *   1: 0001 磁场过强
 *   2: 0010 磁场过弱
 *   3: 0011 磁场状态编码无效
 *   4: 0100 正常，旋钮被按压
 *   5: 0101 磁场过强，旋钮被按压
 *   6: 0110 磁场过弱，旋钮被按压
 *   7: 0111 磁场状态编码无效，旋钮被按压
 *   8: 1000 正常，超速
 *   9: 1001 磁场过强，超速
 *   A: 1010 磁场过弱，超速
 *   B: 1011 磁场状态编码无效，超速
 *   C: 1100 正常，旋钮被按压，超速
 *   D: 1101 磁场过强，旋钮被按压，超速
 *   E: 1110 磁场过弱，旋钮被按压，超速
 *   F: 1111 磁场状态编码无效，旋钮被按压，超速
 *
 * 注意：Mg[1:0] 是一个两位状态编码，不是两个独立标志。
 * 当前 CRC 校验已关闭，mag 随每次 SPI 读取结果更新。
 */
extern uint8_t mag;
extern uint8_t crc_rx;
extern uint8_t crc_calc;
extern uint32_t valid24;
extern int32_t cdeg;// 角度，单位为 0.01 
extern uint8_t crc_calc_frame;
extern uint8_t crc_rx_frame;
#endif // !__Encoder_H
