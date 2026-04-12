/**
 * @file    app_radar_service.c
 * @brief   应用层雷达监测逻辑模块
 */

#include "app_radar_service.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "usart.h"

#include "app_alarm_notify.h"
#include "app_voice_service.h"

#include "bsp_uart_comm.h"

#include "pal_log.h"

/* 定义二值信号量，用于同步中断与任务 */
SemaphoreHandle_t xRadarDataReadySem = NULL;

/* 任务句柄：用于外部管理该任务（如删除、挂起、获取状态） */
TaskHandle_t xRadarServiceTaskHandle = NULL;

uint8_t Verify_Radar_Checksum(uint8_t *frame, uint16_t len);
static void Process_Radar_Protocol(uint8_t *pFrame);
void Radar_Protocol_Parse_Stream(uint8_t *p_buf, uint16_t len);

/**
 * @brief  雷达监测处理任务
 * @param  pvParameters: 任务创建时传入的参数
 * @retval None
 */
static void Radar_Service_Task(void *pvParameters)
{
    for (;;)
    {
        /* 阻塞等待信号量 */
        if (xSemaphoreTake(xRadarDataReadySem, portMAX_DELAY) == pdPASS)
        {
            Radar_Protocol_Parse_Stream(radar_conn.rx_buf, radar_conn.rx_cnt); /* 调用解析函数 */
            BSP_UART_ClearBuffer(&radar_conn);                                 /* 重置串口接收缓冲区状态 */
        }
    }
}

/**
 * @brief  雷达监测模块初始化
 * @details 由 APP_Init 函数调用，负责分配任务资源并加入 FreeRTOS 调度器
 * @retval None
 */
void App_Radar_Service_Init(void)
{
    /* 创建信号量，用于外部中断回调函数中通知采集任务 */
    if (xRadarDataReadySem == NULL)
    {
        xRadarDataReadySem = xSemaphoreCreateBinary();
    }

    xTaskCreate(Radar_Service_Task,
                "Radar",
                512,
                NULL,
                10,
                &xRadarServiceTaskHandle);
}

/**
 * @brief  计算并验证雷达数据帧的和校验
 * @param  frame: 指向待校验雷达数据帧的首地址
 * @param  len:   待校验帧的完整长度 (包含帧头、数据、校验位及帧尾)
 * @retval 校验结果: 0-成功 , 1-失败
 * @note   校验算法为：对除[校验位]和[帧尾]之外的所有字节进行 8 位累加后取低八位
 */
uint8_t Verify_Radar_Checksum(uint8_t *frame, uint16_t len)
{
    uint8_t calculated_sum = 0;
    uint16_t check_range = len - 3; /* 排除掉 [校验1B] [帧尾2B] */

    for (uint16_t i = 0; i < check_range; i++)
    {
        calculated_sum += frame[i]; /* 累加会自动溢出，相当于取低八位 */
    }

    if (calculated_sum == frame[len - 3])
    {
        PAL_LOG(PAL_LOG_LEVEL_INFO, "校验成功");
        return 0; /* 校验成功 */
    }

    PAL_LOG(PAL_LOG_LEVEL_ERROR, "校验失败");
    return 1; /* 校验失败 */
}

/**
 * @brief  毫米波雷达协议解析与任务分发逻辑
 * @param  pFrame: 指向已接收到的完整协议帧首地址 (通常以 0x53 起始)
 * @note   协议结构: [帧头 2B] + [控制 1B] + [命令 1B] + [长度 2B] + [数据 nB] + [校验 1B] + [帧尾 2B]
 */
static void Process_Radar_Protocol(uint8_t *pFrame)
{
    uint8_t control = pFrame[2];
    uint8_t command = pFrame[3];
    uint16_t d_len = (pFrame[4] << 8) | pFrame[5];
    uint8_t *pData = &pFrame[6]; /* 数据区起始地址 */

    switch (control)
    {
    case 0x01: /* 心跳包标识 */
        break;
    case 0x02: /* 产品信息 */
        break;
    case 0x03: /* OTA 升级 */
        break;
    case 0x05: /* 工作状态 */
        break;
    case 0x06: /* 安装方式 */
        break;
    case 0x80: /* 人体存在信息 */
        break;
    case 0x82: /* 轨迹信息 */
        break;

    case 0x83: /* 跌倒监测状态 */
        if (command == 0x01 && pData[0] == 0x01)
        {
            PAL_LOG(PAL_LOG_LEVEL_INFO, "!!! 警报：检测到跌倒 !!!");

            /* 释放信号量，唤醒报警任务 */
            if (xAlarmSemaphore != NULL)
            {
                xSemaphoreGive(xAlarmSemaphore);
            }

            /* 用队列向语音任务发送跌倒报警的通知 */
            if (xVoiceQueue != NULL)
            {
                Voice_Msg_Type_t msg = MSG_VOICE_FALL_ALARM;
                xQueueSend(xVoiceQueue, &msg, 0); /* 跨任务投递消息 */
            }
        }
        break;

    default:
        PAL_LOG(PAL_LOG_LEVEL_ERROR, "未定义控制字: 0x%02X", control);
        break;
    }
}

/**
 * @brief  解析雷达原始数据缓冲区
 * @param  p_buf: 缓冲区首地址
 * @param  len: 缓冲区内有效数据长度
 * @retval None
 */
void Radar_Protocol_Parse_Stream(uint8_t *p_buf, uint16_t len)
{
    uint16_t cur = 0;

    /* 只要剩余长度大于最小包长(10字节)就继续解析 */
    while (cur + 10 <= len)
    {
        /* 定位帧头 */
        if (p_buf[cur] == 0x53 && p_buf[cur + 1] == 0x59)
        {
            /* 提取数据长度并计算整帧长度 */
            uint16_t data_len = (p_buf[cur + 4] << 8) | p_buf[cur + 5];
            uint16_t full_len = data_len + 9;

            /* 边界检查 */
            if (cur + full_len <= len)
            {
                /* 验证帧尾 */
                if (p_buf[cur + full_len - 2] == 0x54 && p_buf[cur + full_len - 1] == 0x43)
                {
                    /* 校验和验证 */
                    if (Verify_Radar_Checksum(&p_buf[cur], full_len) == 0)
                    {
                        Process_Radar_Protocol(&p_buf[cur]); /* 协议解析与任务分发 */
                    }
                    /* 成功解析或校验失败，都跳过这整一帧 */
                    cur += full_len;
                    continue;
                }
            }
        }
        /* 若未匹配到完整帧，滑动 1 字节继续搜索 */
        cur++;
    }
}
