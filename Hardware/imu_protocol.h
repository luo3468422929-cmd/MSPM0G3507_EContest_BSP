/**
 * @file imu_protocol.h
 * @brief 定义 NCU 0x5A 五字节帧的流式解析器和 yaw/gyroZ 样本。
 *
 * 所属层：Hardware 纯协议。0xAA 帧为 Z 轴角速度，0xBB 帧为航向角；
 * 最后一字节为前四字节累加和。
 */
#ifndef IMU_PROTOCOL_H
#define IMU_PROTOCOL_H

#include "common.h"

/** 惯导最新有效数据及链路统计。 */
typedef struct {
    float yawDeg;           /**< 航向角，单位 °，量程约 ±180°。 */
    float gyroZDps;         /**< Z 轴角速度，单位 °/s，量程约 ±2000°/s。 */
    uint32_t updateTickMs;  /**< 最近合法帧的系统毫秒。 */
    uint32_t frameCount;    /**< 合法支持帧累计数。 */
    uint32_t errorCount;    /**< 校验错误或未知帧类型累计数。 */
    bool yawValid;          /**< 至少收到过一帧 yaw。 */
    bool gyroZValid;        /**< 至少收到过一帧 gyroZ。 */
} ImuSample_t;

/** 五字节接收缓存及对外样本。 */
typedef struct {
    uint8_t frame[5];   /**< 正在接收的完整帧。 */
    uint8_t index;      /**< 下一字节写入位置。 */
    ImuSample_t sample; /**< 合法帧更新后的最新样本。 */
    bool newData;       /**< sample 是否尚未由 GetSample 消费。 */
} ImuProtocol_t;

/** @brief 清零解析器并回到等待 0x5A 帧头状态。 */
void ImuProtocol_Init(ImuProtocol_t *parser);

/** @brief 推入一个字节；合法完整帧返回 STATUS_OK，未完成返回 BUSY/EMPTY。 */
Status_t ImuProtocol_PushByte(ImuProtocol_t *parser, uint8_t value, uint32_t nowMs);

/** @brief 复制最新样本并清除协议层 newData；无有效字段时返回 STATUS_EMPTY。 */
Status_t ImuProtocol_GetSample(ImuProtocol_t *parser, ImuSample_t *sample);

#endif

