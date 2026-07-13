#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** 所有模块共用的返回状态。 */
typedef enum {
    STATUS_OK = 0,
    STATUS_ERROR,
    STATUS_INVALID_PARAM,
    STATUS_NOT_INITIALIZED,
    STATUS_TIMEOUT,
    STATUS_BUSY,
    STATUS_OVERFLOW,
    STATUS_EMPTY,
    STATUS_CRC_ERROR,
    STATUS_NOT_SUPPORTED
} Status_t;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

static inline float Common_ClampFloat(float value, float minimum, float maximum)
{
    return (value > maximum) ? maximum : ((value < minimum) ? minimum : value);
}

static inline int32_t Common_ClampInt32(int32_t value, int32_t minimum, int32_t maximum)
{
    return (value > maximum) ? maximum : ((value < minimum) ? minimum : value);
}

#endif

