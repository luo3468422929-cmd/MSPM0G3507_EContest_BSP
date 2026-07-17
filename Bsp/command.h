/**
 * @file command.h
 * @brief 定义调试 UART 上的命令注册、解析分发和应答接口。
 *
 * 所属层：Bsp 通信服务。它使用 frame_protocol 的 AA55 帧格式，但不知道
 * 具体比赛业务；业务命令通过回调函数注册。
 */
#ifndef COMMAND_H
#define COMMAND_H

#include "common.h"
#include "frame_protocol.h"

/** 命令处理回调；request 仅在本次回调期间有效。 */
typedef Status_t (*Command_Handler_t)(const FrameProtocol_Frame_t *request);

/** @brief 清空命令表并复位帧解析器。 */
Status_t Command_Init(void);

/**
 * @brief 注册或替换一个 8 位命令码的处理函数。
 * @return STATUS_OK 表示成功；表满返回 STATUS_OVERFLOW。
 */
Status_t Command_Register(uint8_t command, Command_Handler_t handler);

/**
 * @brief 从调试 UART 取出全部可用字节，解析完整帧并调用匹配回调。
 * @note 在主循环调用，禁止在 UART 中断中执行回调。
 */
void Command_Process(void);

/**
 * @brief 将命令和载荷编码为 AA55 帧并从调试 UART 发送。
 * @param payload length 为 0 时允许为 NULL。
 * @param length 载荷长度，最大 FRAME_PROTOCOL_MAX_PAYLOAD。
 */
Status_t Command_Send(uint8_t command, const uint8_t *payload, uint8_t length);

#endif

