#include "frame_protocol.h"

#include <string.h>

enum {
    FRAME_WAIT_AA = 0,
    FRAME_WAIT_55,
    FRAME_WAIT_COMMAND,
    FRAME_WAIT_LENGTH,
    FRAME_WAIT_PAYLOAD,
    FRAME_WAIT_CHECKSUM
};

void FrameProtocol_Init(FrameProtocol_t *parser)
{
    if (parser != NULL) { memset(parser, 0, sizeof(*parser)); }
}

Status_t FrameProtocol_PushByte(FrameProtocol_t *parser, uint8_t value)
{
    if (parser == NULL) { return STATUS_INVALID_PARAM; }
    switch (parser->state) {
        case FRAME_WAIT_AA:
            if (value == 0xAAU) {
                parser->checksum = value;
                parser->state = FRAME_WAIT_55;
            }
            return STATUS_BUSY;
        case FRAME_WAIT_55:
            if (value == 0x55U) {
                parser->checksum = (uint8_t)(parser->checksum + value);
                parser->state = FRAME_WAIT_COMMAND;
            } else {
                parser->state = (value == 0xAAU) ? FRAME_WAIT_55 : FRAME_WAIT_AA;
                parser->checksum = (value == 0xAAU) ? 0xAAU : 0U;
            }
            return STATUS_BUSY;
        case FRAME_WAIT_COMMAND:
            parser->working.command = value;
            parser->checksum = (uint8_t)(parser->checksum + value);
            parser->state = FRAME_WAIT_LENGTH;
            return STATUS_BUSY;
        case FRAME_WAIT_LENGTH:
            if (value > FRAME_PROTOCOL_MAX_PAYLOAD) {
                parser->state = FRAME_WAIT_AA;
                parser->errorCount++;
                return STATUS_OVERFLOW;
            }
            parser->working.length = value;
            parser->index = 0U;
            parser->checksum = (uint8_t)(parser->checksum + value);
            parser->state = (value == 0U) ? FRAME_WAIT_CHECKSUM : FRAME_WAIT_PAYLOAD;
            return STATUS_BUSY;
        case FRAME_WAIT_PAYLOAD:
            parser->working.payload[parser->index++] = value;
            parser->checksum = (uint8_t)(parser->checksum + value);
            if (parser->index >= parser->working.length) {
                parser->state = FRAME_WAIT_CHECKSUM;
            }
            return STATUS_BUSY;
        case FRAME_WAIT_CHECKSUM:
            parser->state = FRAME_WAIT_AA;
            if (value != parser->checksum) {
                parser->errorCount++;
                return STATUS_CRC_ERROR;
            }
            parser->ready = parser->working;
            parser->frameReady = true;
            parser->validCount++;
            return STATUS_OK;
        default:
            parser->state = FRAME_WAIT_AA;
            return STATUS_ERROR;
    }
}

Status_t FrameProtocol_GetFrame(FrameProtocol_t *parser, FrameProtocol_Frame_t *frame)
{
    if ((parser == NULL) || (frame == NULL)) { return STATUS_INVALID_PARAM; }
    if (!parser->frameReady) { return STATUS_EMPTY; }
    *frame = parser->ready;
    parser->frameReady = false;
    return STATUS_OK;
}

uint16_t FrameProtocol_Encode(uint8_t command, const uint8_t *payload,
                              uint8_t length, uint8_t *output, uint16_t capacity)
{
    uint8_t checksum;
    uint16_t frameLength = (uint16_t)length + 5U;
    if ((output == NULL) || (length > FRAME_PROTOCOL_MAX_PAYLOAD) ||
        ((payload == NULL) && (length != 0U)) || (capacity < frameLength)) {
        return 0U;
    }
    output[0] = 0xAAU;
    output[1] = 0x55U;
    output[2] = command;
    output[3] = length;
    checksum = (uint8_t)(0xAAU + 0x55U + command + length);
    for (uint8_t index = 0U; index < length; ++index) {
        output[index + 4U] = payload[index];
        checksum = (uint8_t)(checksum + payload[index]);
    }
    output[length + 4U] = checksum;
    return frameLength;
}

