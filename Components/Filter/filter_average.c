#include "filter_average.h"

Status_t AverageFilter_Init(AverageFilter_t *filter, float *storage, uint16_t capacity)
{
    if ((filter == NULL) || (storage == NULL) || (capacity == 0U)) {
        return STATUS_INVALID_PARAM;
    }
    filter->storage = storage;
    filter->capacity = capacity;
    filter->initialized = true;
    AverageFilter_Reset(filter);
    return STATUS_OK;
}

float AverageFilter_Update(AverageFilter_t *filter, float sample)
{
    if ((filter == NULL) || !filter->initialized) {
        return sample;
    }
    if (filter->count < filter->capacity) {
        filter->storage[filter->index] = sample;
        filter->sum += sample;
        filter->count++;
    } else {
        filter->sum -= filter->storage[filter->index];
        filter->storage[filter->index] = sample;
        filter->sum += sample;
    }
    filter->index = (uint16_t)((filter->index + 1U) % filter->capacity);
    return filter->sum / (float)filter->count;
}

void AverageFilter_Reset(AverageFilter_t *filter)
{
    uint16_t index;
    if ((filter == NULL) || (filter->storage == NULL)) {
        return;
    }
    for (index = 0U; index < filter->capacity; ++index) {
        filter->storage[index] = 0.0f;
    }
    filter->sum = 0.0f;
    filter->count = 0U;
    filter->index = 0U;
}

