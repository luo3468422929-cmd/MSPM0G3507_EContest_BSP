#include "command.h"

#include <string.h>

#include "uart.h"

#define COMMAND_HANDLER_COUNT 16U

typedef struct { uint8_t command; Command_Handler_t handler; } HandlerEntry_t;
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
    uint8_t frame[FRAME_PROTOCOL_MAX_PAYLOAD + 5U];
    uint16_t frameLength = FrameProtocol_Encode(command, payload, length,
                                                frame, sizeof(frame));
    if (frameLength == 0U) { return STATUS_INVALID_PARAM; }
    return UART_Send(UART_ID_DEBUG, frame, frameLength);
}

