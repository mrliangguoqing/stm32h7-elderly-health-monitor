#include "aht30.h"

/* 内部函数 */
static uint8_t AHT30_CRC8(uint8_t *message, uint8_t len);

/**
 * @brief  AHT30 传感器初始化
 * @param  dev: 指向 AHT30 设备句柄结构的指针
 * @retval 初始化结果: 0-成功, 其他-失败
 */
uint8_t AHT30_Init(AHT30_t *dev)
{
    dev->interface.delay(5); // 等待上电稳定
    return 0;
}

/**
 * @brief  读取 AHT30 传感器的温度和湿度数据
 * @param  dev: 指向 AHT30 设备句柄结构的指针
 * @retval 执行结果: 0-成功, 1-通讯错误, 2-CRC校验失败
 */
uint8_t AHT30_Get_Temp_Hum(AHT30_t *dev)
{
    uint8_t start_cmd[] = {0xAC, 0x33, 0x00};
    uint8_t buffer[7] = {0};
    uint32_t srh_raw = 0, st_raw = 0;

    if (dev->interface.write(dev->dev_addr, start_cmd, 3, dev->interface.intf_ptr) != 0)    /* 发送测量命令 */
        return 1;

    dev->interface.delay(85); /* 等待测量完成（手册要求 80ms） */

    if (dev->interface.read(dev->dev_addr, buffer, 7, dev->interface.intf_ptr) != 0)        /* 读取 7 字节原始数据 */
        return 1;

    if (AHT30_CRC8(buffer, 6) != buffer[6]) /* CRC 校验 */
        return 2;

    dev->data.status = buffer[0];

    srh_raw = ((uint32_t)buffer[1] << 12) | ((uint32_t)buffer[2] << 4) | ((uint32_t)buffer[3] >> 4);
    st_raw = (((uint32_t)buffer[3] & 0x0F) << 16) | ((uint32_t)buffer[4] << 8) | (uint32_t)buffer[5];

    dev->data.humidity = (float)srh_raw * 100.0f / 1048576.0f;
    dev->data.temperature = (float)st_raw * 200.0f / 1048576.0f - 50.0f;

    return 0;
}

/**
 * @brief  CRC8 校验计算 (多项式: X8+X5+X4+1 -> 0x31, 初始值: 0xFF)
 * @param  message: 指向包含传感器返回数据的数组首地址
 * @param  len: 需要参与校验的数据字节数
 * @retval 计算出来的 8 位 CRC 校验码
 */
static uint8_t AHT30_CRC8(uint8_t *message, uint8_t len)
{
    uint8_t crc = 0xFF;

    for (uint8_t j = 0; j < len; j++)
    {
        crc ^= message[j];

        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc = (crc << 1);
        }
    }

    return crc;
}
