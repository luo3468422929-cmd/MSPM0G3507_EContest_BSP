/**
 * @file filter_average.c
 * @brief 实现启动阶段自动缩短窗口、满窗后 O(1) 更新的滑动平均。
 *
 * 所属层：Hardware 纯算法。filter 不拥有 storage，调用者必须保证数组在
 * 使用期内有效且不被其他模块同时修改。
 */
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
        /* 启动阶段只对已经收到的样本求平均，不用 0 填充拉低结果。 */
        filter->storage[filter->index] = sample;
        filter->sum += sample;
        filter->count++;
    } else {
        /* 满窗后减去被覆盖的旧样本，再加入新样本。 */
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

