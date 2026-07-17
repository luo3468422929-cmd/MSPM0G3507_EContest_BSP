/**
 * @file encoder_decode.h
 * @brief 提供与 GPIO 无关的 A/B 两相上升沿 x2 正交判向公式。
 *
 * 所属层：Hardware 纯算法。A、B 每相只在上升沿计数，因此每个编码器
 * 信号周期产生 2 个计数；函数可直接在 Host 测试验证正反转。
 */
#ifndef ENCODER_DECODE_H
#define ENCODER_DECODE_H

#include <stdbool.h>
#include <stdint.h>

/** 本次中断来自哪一相的上升沿。 */
typedef enum {
    ENCODER_EDGE_A_RISING = 0, /**< A 相上升沿。 */
    ENCODER_EDGE_B_RISING,     /**< B 相上升沿。 */
    ENCODER_EDGE_COUNT         /**< 非法值边界。 */
} Encoder_Edge_t;

/**
 * @brief 将一次 A/B 相上升沿和当前两相电平转换为有符号计数增量。
 * @param edge 触发本次中断的相。
 * @param phaseAHigh A 相当前是否为高电平。
 * @param phaseBHigh B 相当前是否为高电平。
 * @return +1 或 -1 表示方向；非法边沿源返回 0。
 */
static inline int8_t EncoderDecode_GetIncrement(Encoder_Edge_t edge,
                                                bool phaseAHigh,
                                                bool phaseBHigh)
{
    /* A/B 哪一相领先会同时改变“触发边沿”和“两相是否相同”的组合。 */
    bool samePhase = (phaseAHigh == phaseBHigh);

    if (edge == ENCODER_EDGE_A_RISING) {
        return samePhase ? 1 : -1;
    }
    if (edge == ENCODER_EDGE_B_RISING) {
        return samePhase ? -1 : 1;
    }
    return 0;
}

#endif
