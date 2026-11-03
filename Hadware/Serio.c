#include "main.h"
#include "Serio.h"
#include "usart.h"  //Tx:PC10:Rx:PC11
#include <stdarg.h>
#include <stdio.h>

/**
  * 函    数：串口发送单个字节
  * 参    数：Byte 要发送的一字节数据
  * 返 回 值：无
  */
void Serial_SendByte(uint8_t Byte)
{
    HAL_UART_Transmit(&huart4, &Byte, 1, HAL_MAX_DELAY);
}

/**
  * 函    数：串口发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void Serial_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i ++)//遍历字符数组（字符串），遇到字符串结束标志位后停止
    {
        Serial_SendByte(String[i]);        //依次调用Serial_SendByte发送每个字节数据
    }
}

/**
  * 函    数：使用printf需要重定向的底层函数（HAL库）
  * 参    数：保持原始格式即可，无需变动
  * 返 回 值：保持原始格式即可，无需变动
  */
int fputc(int ch, FILE *f)
{
    uint8_t temp = ch;
    HAL_UART_Transmit(&huart4, &temp, 1, HAL_MAX_DELAY);
    return ch;
}

/**
  * 函    数：自己封装的prinf函数
  * 参    数：format 格式化字符串
  * 参    数：... 可变的参数列表
  * 返 回 值：无
  */
void Serial_Printf(char *format, ...)
{
    char String[100];                //定义字符数组
    va_list arg;                    //定义可变参数列表数据类型的变量arg
    va_start(arg, format);            //从format开始，接收参数列表到arg变量
    vsprintf(String, format, arg);    //使用vsprintf打印格式化字符串和参数列表到字符数组中
    va_end(arg);                    //结束变量arg
    Serial_SendString(String);        //串口发送字符数组（字符串）
}
