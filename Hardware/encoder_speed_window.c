/**
 * @file encoder_speed_window.c
 * @brief 实现可变 dt 样本组成的最短固定时间窗与有符号 RPM 计算。
 *
 * 所属层：Hardware 纯算法。窗口保留刚好覆盖 minimumWindowS 的最新样本；
 * 若单次调度延迟已经超过窗口，则该单次真实 dt 自身构成有效窗口。
 */
#include "encoder_speed_window.h"

/* 0.01f 累加五次可能略小于 0.05f，比较时保留 1 us 浮点容差。 */
#define ENCODER_SPEED_WINDOW_EPSILON_S 0.000001f

/** 从累计量中移除最旧样本，并推进环形数组起点。 */
static void EncoderSpeedWindow_RemoveOldest(EncoderSpeedWindow_t *window)
{
    uint8_t index;

    if ((window == NULL) || (window->sampleCount == 0U)) {
        return;
    }
    index = window->oldestIndex;
    window->totalDeltaCount -= window->deltaCount[index];
    window->totalTimeS -= window->durationS[index];
    window->oldestIndex = (uint8_t)((index + 1U) %
                                    ENCODER_SPEED_WINDOW_MAX_SAMPLES);
    window->sampleCount--;
}

Status_t EncoderSpeedWindow_Init(EncoderSpeedWindow_t *window)
{
    if (window == NULL) {
        return STATUS_INVALID_PARAM;
    }
    *window = (EncoderSpeedWindow_t){0};
    return STATUS_OK;
}

Status_t EncoderSpeedWindow_Push(EncoderSpeedWindow_t *window,
                                 int32_t deltaCount,
                                 float sampleTimeS,
                                 float minimumWindowS,
                                 float countsPerWheelRev,
                                 float *rpm,
                                 bool *ready)
{
    uint8_t insertIndex;
    float requiredWindowS;

    if ((window == NULL) || (rpm == NULL) || (ready == NULL) ||
        (sampleTimeS <= 0.0f) || (minimumWindowS <= 0.0f) ||
        (countsPerWheelRev <= 0.0f)) {
        return STATUS_INVALID_PARAM;
    }

    *ready = false;
    /* 单次延迟超过默认窗口时，不能假设只经过 minimumWindowS。 */
    requiredWindowS = (sampleTimeS > minimumWindowS) ?
                      sampleTimeS : minimumWindowS;

    if (window->sampleCount >= ENCODER_SPEED_WINDOW_MAX_SAMPLES) {
        EncoderSpeedWindow_RemoveOldest(window);
    }
    insertIndex = (uint8_t)((window->oldestIndex + window->sampleCount) %
                            ENCODER_SPEED_WINDOW_MAX_SAMPLES);
    window->deltaCount[insertIndex] = deltaCount;
    window->durationS[insertIndex] = sampleTimeS;
    window->sampleCount++;
    window->totalDeltaCount += deltaCount;
    window->totalTimeS += sampleTimeS;

    /* 保留刚好覆盖最小时间窗的最新样本，避免窗口无限变长。 */
    while (window->sampleCount > 1U) {
        uint8_t oldest = window->oldestIndex;
        float remainingTimeS = window->totalTimeS - window->durationS[oldest];
        if ((remainingTimeS + ENCODER_SPEED_WINDOW_EPSILON_S) <
            requiredWindowS) {
            break;
        }
        EncoderSpeedWindow_RemoveOldest(window);
    }

    if ((window->totalTimeS + ENCODER_SPEED_WINDOW_EPSILON_S) >=
        requiredWindowS) {
        /* RPM = 窗内转数 / 窗内秒数 × 60。计数正负号保留方向。 */
        *rpm = ((float)window->totalDeltaCount * 60.0f) /
               (countsPerWheelRev * window->totalTimeS);
        *ready = true;
    }
    return STATUS_OK;
}
