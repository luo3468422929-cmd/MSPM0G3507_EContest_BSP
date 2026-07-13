#ifndef MODULE_TESTS_H
#define MODULE_TESTS_H

#include "common.h"

Status_t ModuleTest_LED(void);
Status_t ModuleTest_Motor(uint32_t nowMs);
Status_t ModuleTest_Encoder(void);
Status_t ModuleTest_Track(void);
Status_t ModuleTest_OLED(void);
Status_t ModuleTest_IMU(uint32_t nowMs);

#endif

