#include "main.h"
#include "spi.h"
#include "Encoder.h"
/* ============================================================
 * 功能：读取 SSI 模式下的 14bit 绝对角度
 * ============================================================ */
/* ============================================================ 
 * 接线：
 *   编码器 DO  (A)  ---- PA6  (SPI1_MISO，上拉输入)
 *   编码器 CLK (B)  ---- PA5  (SPI1_SCK)
 *   编码器 CSN (Z)  ---- PA4  (GPIO 软件片选，低有效)
 *   编码器 VCC ------- 5V / 3.3V
 *   编码器 GND ------- GND
 * SPI 参数：
 *   主机、8bit、MSB 先行、Mode 2（CPOL=1，CPHA=0）、1MHz
 *
 * SSI 帧：
 *   D[13:0] 角度 + Mg[3:0] 磁场状态 + CRC[5:0] 校验
 *   CRC 多项式 x^6+x+1，覆盖前 18bit
 *
 * 编码器首位尚未就绪，因此每次读 32bit，左移一位丢弃首位，
 * 再取高 24bit 作为有效帧。此处理与原寄存器版本一致。
 * ============================================================ */

#define SPI_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
#define SPI_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)

/**
 * @brief 读取编码器数据
 * @param rx 接收数据的数组
 */
static void Encoder_Read4(uint8_t rx[4]) //1 位无效首位 + 24 位有效帧 + 7 位额外时钟
{
    SPI_LOW();
    HAL_Delay(1);  //延时 1ms，确保 CSN 拉低稳定
    uint8_t txData = 0x00;
    HAL_SPI_Transmit(&hspi1, &txData, 1, 10);  //发送无效占位字符，开启SPI传输
   for (int i = 0; i < 4; i++)
    {
        while(__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_TXE) == RESET);  //等待发送缓冲区为空
        HAL_SPI_Transmit(&hspi1, &txData, 1, 10);  //发送无效占位字符
        while(__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_RXNE) == RESET);  //等待接收缓冲区非空
        HAL_SPI_Receive(&hspi1, &rx[i], 1, 10);  //接收数据
    }
    HAL_Delay(1);  //延时 1ms，确保 SPI 传输完成
    while(__HAL_SPI_GET_FLAG(&hspi1, SPI_FLAG_BSY) == SET);  //等待SPI总线空闲
    SPI_HIGH();
}

/**
 * @brief 计算 CRC6 校验码
 * @param data18 18bit 数据
 * @return CRC6 校验码
 */
static uint8_t crc6_calc(uint32_t data18)
{
    uint8_t crc = 0U;
    for (int i = 17; i >= 0; i--) {

        // 取 CRC6 的最高位,bit5移到bit0,并将这个位（得到的0或1）存入 top 变量
        uint8_t top = (uint8_t)((crc >> 5) & 0x01);
        /*
        crc << 1： 左移一位，丢弃最高位，空出最低位
        (data18 >> i) & 0x01U：取 data18 的第 i 位，0 或 1
         |：将 data18 的第 i 位 放入 crc 的最低位
         & 0x3FU：保留 crc 的低 6 位，丢弃高位
        */
        crc = (uint8_t)(((crc << 1) | ((data18 >> i) & 0x01)) & 0x3F);

        //x^6+x+1 多项式的二进制表示为 1  000011，低 6 位为 000011，即 0x03
        //如果最高位为 1，则异或多项式 0x03（x^6+x+1），翻转后两位
        if (top != 0) {
            crc ^= 0x03;
        }
    }
    return crc;
}

/**
 * @brief 读取编码器角度、磁场状态和 CRC 校验码
 * @param angle 指向存储角度的变量的指针
 * @param mag 指向存储磁场状态的变量的指针
 * @param crc 指向存储 CRC 校验码的变量的指针
 * @return 返回有效的 24bit 数据
 */
static uint32_t Encoder_ReadAngle(uint16_t *angle, uint8_t *mag, uint8_t *crc_rx)
{
    uint8_t rx[4];// 接收缓冲区，4 字节
    uint32_t frame32;// 32bit 帧数据
    uint32_t valid24;// 有效的 24bit 数据

    Encoder_Read4(rx);// 读取 4 字节数据

    // 将接收到的 4 字节数据组合成一个 32 位整数
    frame32 = ((uint32_t)rx[0] << 24) | 
    ((uint32_t)rx[1] << 16) | 
    ((uint32_t)rx[2] << 8) |
    (uint32_t)rx[3];

    frame32 <<= 1;  // 左移一位，丢弃首位无效数据
    valid24 = (frame32 >> 8) & 0x00FFFFFFUL;  // 取高 24 位作为有效帧,丢弃低 8 位的额外时钟
    *angle = (valid24 >> 18) & 0x3FFF;  // 取高 14bit，作为角度值
    *mag = (valid24 >> 14) & 0x0F;      // 取中间 4bit，作为磁场状态
    *crc_rx = valid24 & 0x3F;           // 取低 6bit，作为 CRC 校验码
    return valid24;
}
    
/**
 * @brief 编码器所有功能
 * @return 返回操作结果
 */
uint8_t Encoder_All_Function(void)
{
    uint16_t raw14;
    uint8_t mag;
    uint8_t crc_rx;
    uint8_t crc_calc;
    uint32_t valid24;
    int32_t cdeg;// 角度，单位为 0.01 度 

    valid24 = Encoder_ReadAngle(&raw14, &mag, &crc_rx);
    crc_calc = crc6_calc((uint32_t)raw14 << 4 | mag);
    // 将 14bit 原始角度转换为 0.01 度单位，36000 = 360 * 100，16384 = 2^14
    cdeg = (int32_t)((uint32_t)raw14 * 36000 / 16384);

    // 根据 CRC 校验结果处理数据
    if (crc_calc == crc_rx) {
        // CRC 校验成功，处理有效数据
        return 1; // 返回成功标志
        
    } else {
        // CRC 校验失败，处理错误情况
        return 0; // 返回失败标志
    }
}
