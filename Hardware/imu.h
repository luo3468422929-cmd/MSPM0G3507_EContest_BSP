#ifndef IMU_H
#define IMU_H

#include "common.h"
#include "imu_protocol.h"

Status_t IMU_Init(void);
void IMU_Process(uint32_t nowMs);
/** 读取最新样本但不清除“新数据”标志，适合状态页显示。 */
Status_t IMU_PeekSample(ImuSample_t *sample);
/** 读取最新样本并清除“新数据”标志，适合算法消费。 */
Status_t IMU_GetSample(ImuSample_t *sample);
bool IMU_HasNewData(void);
bool IMU_IsOnline(uint32_t nowMs);
Status_t IMU_StartYawZero(uint32_t nowMs);

#endif

