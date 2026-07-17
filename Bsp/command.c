/**
 * @file command.c
 * @brief 实现调试串口 AA55 命令帧的回调表和主循环分发逻辑。
 *
 * 所属层：Bsp 通信服务。UART ISR 只收字节，本文件在 Command_Process()
 * 中完成解析和回调，避免在中断里执行耗时业务。
 */
#include "command.h"

#include <string.h>

#include "uart.h"

/** 最多同时注册的不同命令码数量；重复命令会替换原处理函数。 */
#define COMMAND_HANDLER_COUNT 16U

/** 一个命令码和对应业务回调。 */
typedef struct { uint8_t command; Command_Handler_t handler; } HandlerEntry_t;
/** 命令表和调试口专用流式解析器，均只由主循环访问。 */
static HandlerEntry_t g_handlers[COMMAND_HANDLER_COUNT];
static FrameProtocol_t g_parser;

Status_t Command_Init(void)
{
    memset(g_handlers, 0, sizeof(g_handlers));
    FrameProtocol_Init(&g_parser);
    return STATUS_OK;
}

Status_t Command_Register(uint8_t command, Command_Handler_t handler)
{
    if (handler == NULL) { return STATUS_INVALID_PARAM; }
    for (uint8_t index = 0U; index < COMMAND_HANDLER_COUNT; ++index) {
        /* 优先复用空槽；同一命令码再次注册时直接替换，便于比赛改处理器。 */
        if ((g_handlers[index].handler == NULL) ||
            (g_handlers[index].command == command)) {
            g_handlers[index].command = command;
            g_handlers[index].handler = handler;
            return STATUS_OK;
        }
    }
    return STATUS_OVERFLOW;
}

void Command_Process(void)
{
    uint8_t value;
    FrameProtocol_Frame_t frame;
    /* 一次调用尽量清空软件缓冲，降低连续上位机命令造成的积压。 */
    while (UART_ReadByte(UART_ID_DEBUG, &value) == STATUS_OK) {
        if (FrameProtocol_PushByte(&g_parser, value) == STATUS_OK &&
            FrameProtocol_GetFrame(&g_parser, &frame) == STATUS_OK) {
            for (uint8_t index = 0U; index < COMMAND_HANDLER_COUNT; ++index) {
                if ((g_handlers[index].handler != NULL) &&
                    (g_handlers[index].command == frame.command)) {
                    (void)g_handlers[index].handler(&frame);
                    break;
                }
            }
        }
    }
}

Status_t Command_Send(uint8_t command, const uint8_t *payload, uint8_t length)
{
    /* 最大帧：2 字节帧头 + 命令 + 长度 + 载荷 + 校验和。 */
    uint8_t frame[FRAME_PROTOCOL_MAX_PAYLOAD + 5U];
    uint16_t frameLength = FrameProtocol_Encode(command, payload, length,
                                                frame, sizeof(frame));
    if (frameLength == 0U) { return STATUS_INVALID_PARAM; }
    return UART_Send(UART_ID_DEBUG, frame, frameLength);
}

