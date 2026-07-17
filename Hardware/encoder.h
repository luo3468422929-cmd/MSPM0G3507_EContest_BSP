/**
 * @file encoder.h
 * @brief 定义左右轮 A/B 正交编码器的 x2 计数、RPM 和线速度接口。
 *
 * 所属层：Hardware 传感器层。四相输入由 GPIO 上升沿中断采集，速度按
 * 实际 dt 与 50 ms 滑动窗计算；机械参数集中在 User/user_config.h。
 */
#ifndef ENCODER_H
#define ENCODER_H

#include "common.h"
#include "encoder_decode.h"

/** 左右编码器逻辑编号。 */
typedef enum {
    ENCODER_LEFT = 0, /**< 左轮 PA17/PA16。 */
    ENCODER_RIGHT,    /**< 右轮 PB19/PB20。 */
    ENCODER_COUNT     /**< 编码器数量，仅用于边界检查。 */
} Encoder_Id_t;

/** 编码器主循环数据快照。 */
typedef struct {
    int32_t totalCount;    /**< 最近一次 Encoder_Init/Encoder_Reset 以来的有符号累计计数。 */
    int32_t deltaCount;    /**< 最近一次速度更新新增的有符号计数。 */
    float rpm;             /**< 车轮输出轴有符号转速，单位 r/min。 */
    float linearSpeedMps;  /**< 按轮径换算的线速度，单位 m/s。 */
} Encoder_Data_t;

/** @brief 清零两轮数据并使能 GPIOA/GPIOB 组合中断。 */
Status_t Encoder_Init(void);

/**
 * @brief 将一次 A/B 上升沿及两相电平解码为某轮的有符号计数。
 * @note 由 GPIO ISR 调用；函数只更新计数，不能打印或执行浮点测速。
 */
void Encoder_OnEdge(Encoder_Id_t id, Encoder_Edge_t edge,
                    bool phaseAHigh, bool phaseBHigh);

/**
 * @brief 用本次真实经过时间更新两轮 RPM、滤波值和线速度。
 * @param sampleTimeS 距上一次调用的实际秒数，必须大于 0。
 */
void Encoder_UpdateSpeed(float sampleTimeS);

/** @brief 复制指定轮的最新数据快照。 */
Status_t Encoder_GetData(Encoder_Id_t id, Encoder_Data_t *data);

/** @brief 清零指定轮累计计数、速度窗和滤波状态。 */
void Encoder_Reset(Encoder_Id_t id);

#endif

