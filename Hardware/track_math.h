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

/**
 * @brief 根据已触发通道的位图计算离散加权位置。
 * @param activeMask bit0 对应最左侧第 1 路。
 * @param weights 各通道位置权重数组，bit0 对应最左侧第 1 路。
 * @param count 通道数，合法范围 1~8。
 * @return 已触发通道权重的平均值；无通道触发或参数非法时返回 0。
 */
float TrackMath_WeightedPosition(uint8_t activeMask,
                                 const float *weights,
                                 uint8_t count);

#endif

