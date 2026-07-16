#include "encoder_speed_window.h"

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
        if (remainingTimeS < requiredWindowS) {
            break;
        }
        EncoderSpeedWindow_RemoveOldest(window);
    }

    if (window->totalTimeS >= requiredWindowS) {
        *rpm = ((float)window->totalDeltaCount * 60.0f) /
               (countsPerWheelRev * window->totalTimeS);
        *ready = true;
    }
    return STATUS_OK;
}
