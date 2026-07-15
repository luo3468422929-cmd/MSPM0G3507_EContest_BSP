#ifndef LED_H
#define LED_H

#include "common.h"

Status_t LED_Init(void);
void LED_Set(bool on);
void LED_Toggle(void);

#endif

