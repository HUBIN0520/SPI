/* ============================================================
 * 功能：读取 MT6701 SSI 模式下的 14bit 绝对角度
 * ============================================================
 * 接线：
 *   编码器 DO  (A)  ---- PA6  (SPI1_MISO，上拉输入)
 *   编码器 CLK (B)  ---- PA5  (SPI1_SCK)
 *   编码器 CSN (Z)  ---- PA4  (GPIO 软件片选，低有效)
 *   编码器 VCC ------- 5V
 *   编码器 GND ------- GND
 *
 * SPI 参数：
 *   主机、8bit、MSB 先行、Mode 1（CPOL=0，CPHA=1）、1.125MHz
 *
 * SSI 帧：
 *   D[13:0] 角度 + Mg[3:0] 磁场状态 + CRC[5:0] 校验
 *   CRC 多项式 x^6+x+1，覆盖前 18bit
 *
 * CSN 拉低后严格发送 24 个时钟。编码器在 CLK 上升沿更新数据，
 * STM32 在下降沿采样。
 * ============================================================ */
#include "main.h"
#include "spi.h"
#include "Encoder.h"

uint16_t raw14;
uint8_t mag;
uint8_t crc_rx;
uint8_t crc_calc;
uint32_t valid24;
int32_t cdeg;// 角度，单位为 0.01 度 

#define SPI_HIGH()  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET)
#define SPI_LOW()   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET)

/**
 * @brief 读取编码器的一帧 24bit 数据
 * @param rx 接收数据的数组
 * @return HAL_OK 表示读取成功
 */
static HAL_StatusTypeDef Encoder_Read3(uint8_t rx[3])
{
    const uint8_t tx[3] = {0};
    HAL_StatusTypeDef status;

    SPI_LOW();
    status = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 3, 10);
    SPI_HIGH();

    return status;
}



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
 * @param crc_rx 指向存储 CRC 校验码的变量的指针
 * @param frame24 指向存储完整 24bit 数据的变量的指针
 * @return 1 表示 SPI 读取成功，0 表示失败
 */
static uint8_t Encoder_ReadAngle(uint16_t *angle, uint8_t *mag, uint8_t *crc_rx,
                                 uint32_t *frame24)
{
    uint8_t rx[3];

    if (Encoder_Read3(rx) != HAL_OK)
    {
        return 0;
    }

    *frame24 = ((uint32_t)rx[0] << 16) |
               ((uint32_t)rx[1] << 8) |
                (uint32_t)rx[2];
    *angle = (*frame24 >> 10) & 0x3FFFU;
    *mag = (*frame24 >> 6) & 0x0FU;
    *crc_rx = *frame24 & 0x3FU;
    return 1;
}
    
/**
 * @brief 编码器所有功能
 * @return 返回操作结果
 */
uint8_t Encoder_All_Function(void)
{
    uint16_t raw14_rx;
    uint8_t mag_rx;
    uint8_t crc_rx_frame;
    uint8_t crc_calc_frame;
    uint32_t frame24;
    int32_t cdeg_frame;

    if (Encoder_ReadAngle(&raw14_rx, &mag_rx, &crc_rx_frame, &frame24) == 0)
    {
        return 0;
    }

    crc_calc_frame = crc6_calc(((uint32_t)raw14_rx << 4) | mag_rx);

    // 根据 CRC 校验结果处理数据
    if (crc_calc_frame == crc_rx_frame) {
        // 仅在 CRC 校验成功时更新角度，失败时保留上一次有效值
        // 将 14bit 原始角度转换为 0.01 度单位，36000 = 360 * 100，16384 = 2^14
        cdeg_frame = (int32_t)((uint32_t)raw14_rx * 36000 / 16384);

        raw14 = raw14_rx;
        mag = mag_rx;
        crc_rx = crc_rx_frame;
        crc_calc = crc_calc_frame;
        valid24 = frame24;
        cdeg = cdeg_frame;
        return 1; // 返回成功标志
        
    } else {
        // CRC 校验失败，处理错误情况
        return 0; // 返回失败标志
    }
}
