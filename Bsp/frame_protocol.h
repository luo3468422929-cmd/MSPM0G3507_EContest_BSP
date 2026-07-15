#ifndef FRAME_PROTOCOL_H
#define FRAME_PROTOCOL_H

#include "common.h"

#define FRAME_PROTOCOL_MAX_PAYLOAD 32U

typedef struct {
    uint8_t command;
    uint8_t length;
    uint8_t payload[FRAME_PROTOCOL_MAX_PAYLOAD];
} FrameProtocol_Frame_t;

typedef struct {
    uint8_t state;
    uint8_t index;
    uint8_t checksum;
    FrameProtocol_Frame_t working;
    FrameProtocol_Frame_t ready;
    bool frameReady;
    uint32_t validCount;
    uint32_t errorCount;
} FrameProtocol_t;

void FrameProtocol_Init(FrameProtocol_t *parser);
Status_t FrameProtocol_PushByte(FrameProtocol_t *parser, uint8_t value);
Status_t FrameProtocol_GetFrame(FrameProtocol_t *parser, FrameProtocol_Frame_t *frame);
uint16_t FrameProtocol_Encode(uint8_t command, const uint8_t *payload,
                              uint8_t length, uint8_t *output, uint16_t capacity);

#endif

