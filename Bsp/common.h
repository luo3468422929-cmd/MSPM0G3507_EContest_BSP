/**
 * @file common.h
 * @brief 定义全工程共用的状态码、基础类型工具和限幅函数。
 *
 * 所属层：Bsp 公共基础。该文件不依赖 DriverLib，可被 Hardware、Control、
 * User 和 Host 单元测试共同使用。
 */
#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** 所有模块共用的返回状态；调用者应优先处理非 STATUS_OK。 */
typedef enum {
    STATUS_OK = 0,          /**< 操作完成。 */
    STATUS_ERROR,           /**< 未分类的硬件或内部错误。 */
    STATUS_INVALID_PARAM,   /**< 指针、长度、编号或范围非法。 */
    STATUS_NOT_INITIALIZED, /**< 模块尚未完成初始化。 */
    STATUS_TIMEOUT,         /**< 有限等待次数耗尽。 */
    STATUS_BUSY,            /**< 操作/协议帧尚未完成。 */
    STATUS_OVERFLOW,        /**< 缓冲区、表项或协议长度超限。 */
    STATUS_EMPTY,           /**< 当前没有可读取的数据。 */
    STATUS_CRC_ERROR,       /**< 校验和或 CRC 错误。 */
    /** 当前配置不支持该功能。 */
    STATUS_NOT_SUPPORTED,
    STATUS_DISABLED         /**< 模块存在但被功能开关禁用。 */
} Status_t;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

/** @brief 将浮点值限制在 [minimum, maximum] 闭区间。 */
static inline float Common_ClampFloat(float value, float minimum, float maximum)
{
    return (value > maximum) ? maximum : ((value < minimum) ? minimum : value);
}

/** @brief 将 32 位有符号整数限制在 [minimum, maximum] 闭区间。 */
static inline int32_t Common_ClampInt32(int32_t value, int32_t minimum, int32_t maximum)
{
    return (value > maximum) ? maximum : ((value < minimum) ? minimum : value);
}

#endif
