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
void CarControl_Update(void);
void CarControl_Stop(void);
const CarControl_Data_t *CarControl_GetData(void);

#endif
