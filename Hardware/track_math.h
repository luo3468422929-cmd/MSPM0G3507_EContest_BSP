/**
 * @file track_math.h
 * @brief 定义与具体传感器总线无关的循迹加权位置计算。
 *
 * 所属层：Hardware 纯算法。既支持连续灰度值加权，也支持八路数字位图
 * 的离散权重平均，可直接用于 Host 测试。
 */
#ifndef TRACK_MATH_H
#define TRACK_MATH_H

#include "common.h"

/** 当前传感器阵列对赛道黑线的整体观测状态。 */
typedef enum {
    TRACK_STATE_LOST = 0, /**< 无任何有效线信号。 */
    TRACK_STATE_LINE,     /**< 部分通道检测到线。 */
    TRACK_STATE_ALL_ACTIVE /**< 全部通道触发，可能是终点/宽黑带。 */
} Track_State_t;

/** 连续灰度加权算法的完整输出。 */
typedef struct {
    float position;       /**< 灰度加权位置。 */
    uint32_t sum;         /**< 所有输入灰度值之和。 */
    uint8_t activeCount;  /**< 超过 activeThreshold 的通道数。 */
    Track_State_t state;  /**< 根据 activeCount 得到的观测状态。 */
} TrackMath_Result_t;

/**
 * @brief 用连续灰度值与整数权重计算位置和激活通道数。
 * @return STATUS_OK 或 STATUS_INVALID_PARAM。
 */
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

