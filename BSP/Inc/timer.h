#ifndef BSP_TIMER_H
#define BSP_TIMER_H

#include "common.h"

Status_t Timer_Init(void);
uint32_t Timer_GetTickMs(void);
uint32_t Timer_ElapsedMs(uint32_t sinceMs);
void Timer_DelayMs(uint32_t delayMs);
void Timer_TickIRQHandler(void);

#endif

