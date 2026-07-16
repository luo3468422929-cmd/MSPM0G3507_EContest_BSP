#ifndef ENCODER_DECODE_H
#define ENCODER_DECODE_H

#include <stdbool.h>
#include <stdint.h>

/* x2 正交解码：A、B 两相各使用上升沿。 */
typedef enum {
    ENCODER_EDGE_A_RISING = 0,
    ENCODER_EDGE_B_RISING,
    ENCODER_EDGE_COUNT
} Encoder_Edge_t;

/**
 * 将一次 A/B 相上升沿转换为有符号计数增量。
 * @return +1 或 -1 表示方向；非法边沿源返回 0。
 */
static inline int8_t EncoderDecode_GetIncrement(Encoder_Edge_t edge,
                                                bool phaseAHigh,
                                                bool phaseBHigh)
{
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
