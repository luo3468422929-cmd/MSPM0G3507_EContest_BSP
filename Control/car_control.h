#ifndef CAR_CONTROL_H
#define CAR_CONTROL_H

#include "common.h"

typedef struct {
    float baseSpeedRpm;
    float targetLeftRpm;
    float targetRightRpm;
    float actualLeftRpm;
    float actualRightRpm;
    float outputLeft;
    float outputRight;
    float steering;
    float positionError;
    bool lineFound;
    bool enabled;
} CarControl_Data_t;

Status_t CarControl_Init(void);
void CarControl_Enable(bool enable);
void CarControl_SetBaseSpeed(float rpm);
void CarControl_Update(uint32_t nowMs);
/**
 * 仅运行左右轮速度环，跳过循迹输入，供 TEST_PID 使用。
 * @param nowMs 当前系统毫秒计数
 * @param targetLeftRpm 左轮目标转速
 * @param targetRightRpm 右轮目标转速
 */
void CarControl_UpdateSpeedTest(uint32_t nowMs, float targetLeftRpm,
                                float targetRightRpm);
void CarControl_Stop(void);
const CarControl_Data_t *CarControl_GetData(void);

#endif
