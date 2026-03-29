/**
 * @file    bsp_a7670c.c
 * @brief   A7670C 4G 模块驱动模块
 */

#include "bsp_a7670c.h"
#include "bsp_dwt.h"

#include "usart.h"

#include <string.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

/* TODO: 程序不够完善和健壮，后续需要优化 */

/* 接收短信的号码 */
uint8_t g_current_phone[64] = "xxx";

/* 发送的短信内容：紧急！用户赵XX发出求救，请立即处理！ */
uint8_t g_current_text[128] = "7D276025FF01752862378D750058005853D151FA6C426551FF0C8BF77ACB537359047406FF01";

/* 内部函数 */
static void BSP_A7670C_DelayMs(uint32_t ms);
static void BSP_A7670C_UsartSendString(char *_str);

/**
 * @brief  A7670C 发送紧急报警短信
 * @note   该函数会根据当前全局变量 g_current_phone 和 g_current_text 发送内容。
 * @retval 0-成功, 其他-失败
 */
uint8_t BSP_A7670C_SendEmergencySMS(void)
{
    char buffer[256];

    BSP_A7670C_UsartSendString(A7670C_AT_CMGF_1); /* 设置短信为文本模式 */
    BSP_A7670C_DelayMs(201);

    BSP_A7670C_UsartSendString(A7670C_AT_CSMP_BASE); /* 设置文本模式参数 */
    BSP_A7670C_DelayMs(201);

    sprintf(buffer, A7670C_AT_CMGS, g_current_phone);
    BSP_A7670C_UsartSendString(buffer); /* 发送目标手机号 */
    BSP_A7670C_DelayMs(201);

    sprintf(buffer, "%s\r\n", g_current_text);
    BSP_A7670C_UsartSendString(buffer); /* 发送短信正文内容 */
    BSP_A7670C_DelayMs(201);

    BSP_A7670C_UsartSendString(A7670C_CTRL_Z); /* 发送结束符 (0x1A)，触发模块执行物理发送 */

    return 0;
}

/**
 * @brief  A7670C 模块通用延时函数
 * @note   根据 RTOS 调度器状态自动切换延时方式：
 *         启动前：使用 DWT 硬件计数器进行阻塞延时，确保初始化时序准确。
 *         启动后：使用 vTaskDelay 让出 CPU，提高系统并发效率。
 * @param  ms: 需要延时的毫秒数
 * @retval None
 */
static void BSP_A7670C_DelayMs(uint32_t ms)
{
    /* 检查 FreeRTOS 调度器是否已运行 */
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED)
    {
        /* 调度器未启动，使用 DWT 硬件阻塞延时 (死等) */
        BSP_DWT_DelayMs(ms);
    }
    else
    {
        /* 调度器已启动，调用 RTOS 接口进入阻塞态 (释放 CPU) */
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

/**
 * @brief  通过串口向 A7670C 发送字符串（AT 指令或数据）
 * @param  _str: 待发送的字符串指针
 * @retval None
 */
static void BSP_A7670C_UsartSendString(char *_str)
{
    if (_str == NULL)
    {
        return;
    }

    HAL_UART_Transmit(&A7670C_UART, (uint8_t *)_str, strlen(_str), 100);
}
