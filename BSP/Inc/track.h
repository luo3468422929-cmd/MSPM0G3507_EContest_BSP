#ifndef TRACK_H
#define TRACK_H

#include "common.h"
#include "track_math.h"

typedef struct {
    uint16_t raw[8];
    uint16_t filtered[8];
    uint16_t minimum[8];
    uint16_t maximum[8];
    uint16_t threshold[8];
    float positionError;
    uint8_t activeMask;
    Track_State_t state;
    bool calibrated;
} Track_Data_t;

Status_t Track_Init(void);
Status_t Track_Update(void);
Status_t Track_SetRawSamples(const uint16_t *samples, uint8_t count);
Status_t Track_SetThreshold(uint8_t channel, uint16_t threshold);
void Track_CalibrationReset(void);
void Track_CalibrationFeed(const uint16_t *samples, uint8_t count);
Status_t Track_CalibrationFinish(void);
const Track_Data_t *Track_GetData(void);
float Track_GetPositionError(void);

#endif

