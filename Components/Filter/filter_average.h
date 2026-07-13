#ifndef FILTER_AVERAGE_H
#define FILTER_AVERAGE_H

#include "common.h"

typedef struct {
    float *storage;
    float sum;
    uint16_t capacity;
    uint16_t count;
    uint16_t index;
    bool initialized;
} AverageFilter_t;

Status_t AverageFilter_Init(AverageFilter_t *filter, float *storage, uint16_t capacity);
float AverageFilter_Update(AverageFilter_t *filter, float sample);
void AverageFilter_Reset(AverageFilter_t *filter);

#endif

