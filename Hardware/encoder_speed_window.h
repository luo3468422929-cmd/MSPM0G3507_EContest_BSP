/**
 * @file encoder_speed_window.h
 * @brief 定义按真实采样时间累计的编码器固定时间窗测速器。
 *
 * 所属层：Hardware 纯算法。相比直接用单个 10 ms 增量计算，50 ms 窗口
 * 能降低低速量化抖动；调度延迟时仍使用真实 dt，不会虚高转速。
 */
#ifndef ENCODER_SPEED_WINDOW_H
#define ENCODER_SPEED_WINDOW_H

#include "common.h"

/* 10 ms 控制周期下，16 个样本足以覆盖默认 50 ms 速度窗及调度抖动。 */
#define ENCODER_SPEED_WINDOW_MAX_SAMPLES 16U

/** 时间窗内部的脉冲增量、持续时间及其累计值。 */
typedef struct {
    int32_t deltaCount[ENCODER_SPEED_WINDOW_MAX_SAMPLES]; /**< 每段有符号计数。 */
    float durationS[ENCODER_SPEED_WINDOW_MAX_SAMPLES];   /**< 每段真实秒数。 */
    uint8_t oldestIndex;  /**< 当前最旧样本在环形数组中的位置。 */
    uint8_t sampleCount;  /**< 窗内有效样本数。 */
    int32_t totalDeltaCount; /**< 窗内计数和。 */
    float totalTimeS;        /**< 窗内真实时间和，单位 s。 */
} EncoderSpeedWindow_t;

/**
 * @brief 初始化固定时间窗状态。
 * @param window 固定时间窗对象。
 * @return STATUS_OK 或 STATUS_INVALID_PARAM。
 */
Status_t EncoderSpeedWindow_Init(EncoderSpeedWindow_t *window);

/**
 * @brief 推入一次采样增量，并在时间窗就绪时计算有符号 RPM。
 * @param window 固定时间窗对象
 * @param deltaCount 本次采样相对上次采样的有符号脉冲增量
 * @param sampleTimeS 本次采样实际经过的秒数
 * @param minimumWindowS 默认最小统计时间窗
 * @param countsPerWheelRev 每个车轮机械转一圈对应的计数数
 * @param rpm 输出的有符号转速
 * @param ready 输出是否已积累足够时间并更新 rpm；false 时 rpm 不应使用。
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
