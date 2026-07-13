#ifndef TRACK_MATH_H
#define TRACK_MATH_H

#include "common.h"

typedef enum {
    TRACK_STATE_LOST = 0,
    TRACK_STATE_LINE,
    TRACK_STATE_ALL_ACTIVE
} Track_State_t;

typedef struct {
    float position;
    uint32_t sum;
    uint8_t activeCount;
    Track_State_t state;
} TrackMath_Result_t;

Status_t TrackMath_Calculate(const uint16_t *values,
                             const int8_t *weights,
                             uint8_t count,
                             uint16_t activeThreshold,
                             TrackMath_Result_t *result);

#endif

