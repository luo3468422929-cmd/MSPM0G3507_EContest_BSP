/**
 * @file track_math.c
 * @brief 实现连续灰度与数字位图两种循迹加权位置计算。
 *
 * 所属层：Hardware 纯算法，不保存状态。调用者负责决定负/正误差分别
 * 对应小车向哪侧修正，并处理丢线或全路触发策略。
 */
#include "track_math.h"

Status_t TrackMath_Calculate(const uint16_t *values,
                             const int8_t *weights,
                             uint8_t count,
                             uint16_t activeThreshold,
                             TrackMath_Result_t *result)
{
    uint8_t index;
    int32_t weightedSum = 0;
    uint32_t sum = 0U;
    uint8_t activeCount = 0U;

    if ((values == NULL) || (weights == NULL) || (result == NULL) ||
        (count == 0U)) {
        return STATUS_INVALID_PARAM;
    }

    /* 连续灰度越大，对对应位置权重的贡献越大。 */
    for (index = 0U; index < count; ++index) {
        sum += values[index];
        weightedSum += (int32_t)values[index] * (int32_t)weights[index];
        if (values[index] >= activeThreshold) {
            activeCount++;
        }
    }
    result->sum = sum;
    result->activeCount = activeCount;
    if (activeCount == 0U || sum == 0U) {
        result->position = 0.0f;
        result->state = TRACK_STATE_LOST;
    } else {
        result->position = (float)weightedSum / (float)sum;
        result->state = (activeCount == count) ? TRACK_STATE_ALL_ACTIVE : TRACK_STATE_LINE;
    }
    return STATUS_OK;
}

float TrackMath_WeightedPosition(uint8_t activeMask,
                                 const float *weights,
                                 uint8_t count)
{
    float sum = 0.0f;
    uint8_t active = 0U;

    if ((weights == NULL) || (count == 0U) || (count > 8U)) {
        return 0.0f;
    }
    /* 数字位图中每个触发通道权重相同，因此取触发权重的算术平均。 */
    for (uint8_t index = 0U; index < count; ++index) {
        if ((activeMask & (uint8_t)(1U << index)) != 0U) {
            sum += weights[index];
            active++;
        }
    }
    return (active == 0U) ? 0.0f : (sum / (float)active);
}

