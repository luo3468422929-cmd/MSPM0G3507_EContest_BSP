#ifndef IMU_UART_H
#define IMU_UART_H

#include "common.h"
#include "imu_protocol.h"

Status_t IMU_Init(void);
void IMU_Process(uint32_t nowMs);
Status_t IMU_GetSample(ImuSample_t *sample);
bool IMU_HasNewData(void);
bool IMU_IsOnline(uint32_t nowMs);
Status_t IMU_StartYawZero(uint32_t nowMs);

#endif

