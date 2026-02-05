#include "bsp_dwt.h"

/**
 * @brief  DWT初始化，用DWT实现阻塞延时
 * @retval 无
 */
void BSP_DWT_Init(void)
{
    /* 使能DWT外设 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    /* 如果DWT_CTRL的CYCCNTENA位未被设置，则清零计数器 */
    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0)
    {
        DWT->CYCCNT = 0; /* 清零计数值 */
    }

    /* 使能CYCCNT计数器 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

/**
 * @brief  微秒延时函数
 * @note   在移植到其它芯片上后，要测试后才能使用
 * @param  _us: 要延时的微秒数，单位 1us
 * @retval 无
 */
void BSP_DWT_DelayUs(uint32_t _us)
{
    uint32_t start_tick = DWT->CYCCNT; /* 进入时的计数值 */
    uint32_t delay_ticks = 0;          /* 等待的时钟周期数 */

    /* 计算需要等待的时钟周期数 */
    /* HAL_RCC_GetHCLKFreq() 用于获取HCLK频率（通常等于内核频率） */
    delay_ticks = _us * (SystemCoreClock / 1000000);

    /* 等待达到目标计数值 */
    while ((DWT->CYCCNT - start_tick) < delay_ticks)
    {
        /* 空循环等待 */
    }
}

/**
 * @brief  毫秒延时函数
 * @note   形参不可超过 429496
 * @param  _ms: 要延时的毫秒数，单位 1ms
 * @retval 无
 */
void BSP_DWT_DelayMs(uint32_t _ms)
{
    BSP_DWT_DelayUs(1000 * _ms);
}
