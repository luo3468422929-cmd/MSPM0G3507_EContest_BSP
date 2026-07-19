/**
 * @file track_protocol.h
 * @brief 解析 Path Fish 12 路循迹模块的 I2C 双半帧协议。
 *
 * 所属层：Hardware 纯协议层。该文件不依赖 DriverLib，可在电脑端直接测试。
 * 模块每次返回 7 字节：'#'+X1~X6 或 '!'+X7~X12，数据为 ASCII 0/1。
 */
#ifndef TRACK_PROTOCOL_H
#define TRACK_PROTOCOL_H

#include "common.h"

#define TRACK_PROTOCOL_HALF_FRAME_SIZE 7U
#define TRACK_PROTOCOL_FULL_MASK       0x0FFFU

/** 缓存尚未配对的左右半帧；bit0 表示已收到 #，bit1 表示已收到 !。 */
typedef struct {
    uint16_t pendingMask;
    uint8_t receivedHalves;
} TrackProtocol_t;

/** @brief 清空半帧缓存；parser 为 NULL 时不执行操作。 */
void TrackProtocol_Reset(TrackProtocol_t *parser);

/**
 * @brief 推入一个 7 字节半帧。
 * @param parser 协议上下文。
 * @param frame 以 # 或 ! 开头、后跟六个 ASCII 0/1 的缓冲区。
 * @param length 缓冲区长度，必须为 TRACK_PROTOCOL_HALF_FRAME_SIZE。
 * @param completeMask 两个半帧齐全时写入 bit0=X1、bit11=X12 的完整位图。
 * @return STATUS_BUSY 表示仍缺半帧；STATUS_OK 表示得到完整位图；格式或参数
 *         错误返回对应状态，格式错误同时清空缓存，防止新旧半帧混用。
 */
Status_t TrackProtocol_PushHalf(TrackProtocol_t *parser,
                                const uint8_t *frame,
                                size_t length,
                                uint16_t *completeMask);

#endif
