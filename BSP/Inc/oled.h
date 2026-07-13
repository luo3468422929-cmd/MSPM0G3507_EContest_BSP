#ifndef OLED_H
#define OLED_H

#include "common.h"

Status_t OLED_Init(void);
void OLED_Clear(void);
Status_t OLED_ShowString(uint8_t x, uint8_t y, const char *text);
Status_t OLED_ShowInt(uint8_t x, uint8_t y, int32_t value);
Status_t OLED_ShowFloat(uint8_t x, uint8_t y, float value, uint8_t decimals);
Status_t OLED_Refresh(void);

#endif

