#ifndef ENCODER_SPEED_WINDOW_H
#define ENCODER_SPEED_WINDOW_H

#include "common.h"

/* 10 ms 控制周期下，16 个样本足以覆盖默认 50 ms 速度窗及调度抖动。 */
#define ENCODER_SPEED_WINDOW_MAX_SAMPLES 16U

typedef struct {
    int32_t deltaCount[ENCODER_SPEED_WINDOW_MAX_SAMPLES];
    float durationS[ENCODER_SPEED_WINDOW_MAX_SAMPLES];
    uint8_t oldestIndex;
    uint8_t sampleCount;
    int32_t totalDeltaCount;
    float totalTimeS;
} EncoderSpeedWindow_t;

/**
 * 初始化固定时间窗状态。
 * @param window 固定时间窗对象
 * @return STATUS_OK 或 STATUS_INVALID_PARAM
 */
Status_t EncoderSpeedWindow_Init(EncoderSpeedWindow_t *window);

/**
 * 推入一次采样增量，并在时间窗就绪时计算有符号 RPM。
 * @param window 固定时间窗对象
 * @param deltaCount 本次采样相对上次采样的有符号脉冲增量
 * @param sampleTimeS 本次采样实际经过的秒数
 * @param minimumWindowS 默认最小统计时间窗
 * @param countsPerWheelRev 每个车轮机械转一圈对应的计数数
 * @param rpm 输出的有符号转速
 * @param ready 输出是否已积累足够时间并更新 rpm
 * @return STATUS_OK 或 STATUS_INVALID_PARAM
 */
Status_t EncoderSpeedWindow_Push(EncoderSpeedWindow_t *window,
                                 int32_t deltaCount,
                                 float sampleTimeS,
                                 float minimumWindowS,
                                 float countsPerWheelRev,
                                 float *rpm,
                                 bool *ready);

#endif
