/**
 * @file    bsp_delay.c
 * @brief   系统延时模块
 */

#include "bsp_delay.h"

#include "FreeRTOS.h"
#include "task.h"

#include "stm32h7xx.h"

/**
 * @brief  系统延时初始化
 * @param  None
 * @retval None
 */
void BSP_Delay_Init(void)
{
    BSP_DWT_Init();
}

/**
 * @brief  系统级自适应延时函数
 * @note   1. 调度器未启动 -> 阻塞延时
 *         2. 中断上下文中 -> 阻塞延时 (ISR内禁止调用vTaskDelay)
 *         3. 临界区内     -> 阻塞延时
 *         4. 延时太短     -> 阻塞延时 (避免任务切换开销)
 *         5. 正常任务中   -> 非阻塞延时 (释放CPU)
 * @param  ms: 延时毫秒数
 */
void BSP_DelayMs(uint32_t ms)
{
    if (ms == 0)
    {
        return;
    }

    BaseType_t scheduler_state = xTaskGetSchedulerState(); /* 获取调度器状态 */

    /*
    检查是否在中断中：xPortIsInsideInterrupt() 是 FreeRTOS 提供的接口
    如果没有该宏，也可以继续使用 ( __get_IPSR() != 0 )
    */
    uint32_t isr_status = __get_IPSR();

    /* 综合逻辑判定 */
    if (scheduler_state == taskSCHEDULER_NOT_STARTED || /* 调度器未启动 */
        isr_status != 0 ||                              /* 在中断处理中 */
        ms < 2)                                         /* 延时太短(根据实际Tick频率调整) */
    {
        /* 使用 DWT 硬件阻塞延时 */
        BSP_DWT_DelayMs(ms);
    }
    else
    {
        /* 正常任务环境，且延时足够长，调用 RTOS 阻塞接口 */
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}
