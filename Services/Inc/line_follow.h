#ifndef LINE_FOLLOW_H
#define LINE_FOLLOW_H

#include "common.h"

typedef struct {
    float baseSpeedRpm;
    float steering;
    float positionError;
    bool lineFound;
} LineFollow_Data_t;

Status_t LineFollow_Init(void);
void LineFollow_SetBaseSpeed(float rpm);
void LineFollow_Update(void);
const LineFollow_Data_t *LineFollow_GetData(void);

#endif

