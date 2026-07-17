/**
 * @file timer.c
 * @brief 实现 1 ms SysTick 全局时基和基于 WFI 的阻塞延时。
 *
 * 所属层：Bsp 时基层。g_tickMs 只由 SysTick ISR 写入，主循环只读取；
 * 周期判断使用无符号减法，因此计数回绕时仍正确。
 */
#include "timer.h"

#include "board_pins.h"
#include "user_config.h"

/** 系统毫秒计数；volatile 保证主循环每次都读取 ISR 更新后的值。 */
static volatile uint32_t g_tickMs;

Status_t Timer_Init(void)
{
    /* SYSTEM_TICK_HZ 当前为 1000，reload 对应 1 ms 的 CPU 时钟周期数。 */
    uint32_t reload = SYSTEM_CLOCK_HZ / SYSTEM_TICK_HZ;

    if (reload == 0U) {
        return STATUS_INVALID_PARAM;
    }
    g_tickMs = 0U;
    if (SysTick_Config(reload) != 0U) {
        return STATUS_ERROR;
    }
    return STATUS_OK;
}

uint32_t Timer_GetTickMs(void)
{
    return g_tickMs;
}

uint32_t Timer_ElapsedMs(uint32_t sinceMs)
{
    return g_tickMs - sinceMs;
}

void Timer_DelayMs(uint32_t delayMs)
{
    uint32_t start = Timer_GetTickMs();
    /* WFI 让 CPU 等待中断而不是空转；若 SysTick 未运行，本函数不会返回。 */
    while (Timer_ElapsedMs(start) < delayMs) {
        __WFI();
    }
}

void Timer_TickIRQHandler(void)
{
    g_tickMs++;
}

void SysTick_Handler(void)
{
    /* CMSIS 固定中断名，转交可单独调用/测试的项目处理函数。 */
    Timer_TickIRQHandler();
}

