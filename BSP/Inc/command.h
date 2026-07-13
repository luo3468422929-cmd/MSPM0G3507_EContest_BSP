#ifndef COMMAND_H
#define COMMAND_H

#include "common.h"
#include "frame_protocol.h"

typedef Status_t (*Command_Handler_t)(const FrameProtocol_Frame_t *request);

Status_t Command_Init(void);
Status_t Command_Register(uint8_t command, Command_Handler_t handler);
void Command_Process(void);
Status_t Command_Send(uint8_t command, const uint8_t *payload, uint8_t length);

#endif

