/**
 * @file filter_average.h
 * @brief 定义使用调用者存储空间的通用滑动平均滤波器。
 *
 * 所属层：Hardware 纯算法工具，不使用动态内存或硬件接口。数字循迹当前
 * 不强制使用它，但模拟灰度、距离或电压采样可直接复用。
 */
#ifndef FILTER_AVERAGE_H
#define FILTER_AVERAGE_H

#include "common.h"

/** 滑动平均的环形样本、累计和与初始化状态。 */
typedef struct {
    float *storage;    /**< 调用者提供的样本数组。 */
    float sum;         /**< 当前有效样本和，用于 O(1) 更新平均值。 */
    uint16_t capacity; /**< 窗口最大样本数。 */
    uint16_t count;    /**< 启动阶段已经积累的样本数。 */
    uint16_t index;    /**< 下一次覆盖的环形位置。 */
    bool initialized;  /**< Init 成功后为 true。 */
} AverageFilter_t;

/** @brief 绑定外部数组并清零；capacity 必须大于 0。 */
Status_t AverageFilter_Init(AverageFilter_t *filter, float *storage, uint16_t capacity);

/** @brief 推入一个样本并返回当前有效窗口平均值；未初始化时原样返回。 */
float AverageFilter_Update(AverageFilter_t *filter, float sample);

/** @brief 清零存储、累计和与环形索引，但保留容量和初始化状态。 */
void AverageFilter_Reset(AverageFilter_t *filter);

#endif

