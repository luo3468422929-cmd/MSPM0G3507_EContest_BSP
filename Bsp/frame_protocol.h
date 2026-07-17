/**
 * @file frame_protocol.h
 * @brief 定义与硬件无关的 AA 55 不定长命令帧编码和流式解析器。
 *
 * 帧格式：AA 55 | command | length | payload | 8 位累加和。
 * 所属层：Bsp 协议工具；可直接在 Host 测试中使用。
 */
#ifndef FRAME_PROTOCOL_H
#define FRAME_PROTOCOL_H

#include "common.h"

/** 单帧最大载荷；增大时同时检查 UART 缓冲和栈空间。 */
#define FRAME_PROTOCOL_MAX_PAYLOAD 32U

/** 已解码、可交给命令处理器的帧。 */
typedef struct {
    uint8_t command; /**< 业务命令码。 */
    uint8_t length;  /**< payload 的有效字节数。 */
    uint8_t payload[FRAME_PROTOCOL_MAX_PAYLOAD]; /**< 载荷副本。 */
} FrameProtocol_Frame_t;

/** 流式解析器上下文；每个独立字节流必须使用各自的实例。 */
typedef struct {
    uint8_t state;    /**< 当前解析状态。 */
    uint8_t index;    /**< 正在接收的载荷下标。 */
    uint8_t checksum; /**< 当前帧累加和。 */
    FrameProtocol_Frame_t working; /**< 尚未校验完成的帧。 */
    FrameProtocol_Frame_t ready;   /**< 最近一帧已校验数据。 */
    bool frameReady;               /**< ready 是否尚未被取走。 */
    uint32_t validCount;           /**< 成功帧累计数，用于调试链路。 */
    uint32_t errorCount;           /**< 长度/校验错误累计数。 */
} FrameProtocol_t;

/** @brief 将解析器清零并回到等待 0xAA 状态；NULL 时不执行操作。 */
void FrameProtocol_Init(FrameProtocol_t *parser);

/**
 * @brief 向状态机推入一个字节。
 * @return 完整合法帧返回 STATUS_OK；未完成返回 STATUS_BUSY；错误返回对应状态。
 */
Status_t FrameProtocol_PushByte(FrameProtocol_t *parser, uint8_t value);

/** @brief 取走最近一帧；无完整帧时返回 STATUS_EMPTY。 */
Status_t FrameProtocol_GetFrame(FrameProtocol_t *parser, FrameProtocol_Frame_t *frame);

/**
 * @brief 编码一帧到调用者缓冲区。
 * @return 实际帧长；参数、容量或载荷长度非法时返回 0。
 */
uint16_t FrameProtocol_Encode(uint8_t command, const uint8_t *payload,
                              uint8_t length, uint8_t *output, uint16_t capacity);

#endif

