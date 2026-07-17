#include "timer.h"

#include "board_pins.h"
#include "user_config.h"

static volatile uint32_t g_tickMs;

Status_t Timer_Init(void)
{
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

