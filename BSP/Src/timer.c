#include "timer.h"

#include "bsp_config.h"

static volatile uint32_t g_tickMs;

Status_t Timer_Init(void)
{
    g_tickMs = 0U;
    SysTick_Config(SYSTEM_CLOCK_HZ / SYSTEM_TICK_HZ);
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
    Timer_TickIRQHandler();
}

