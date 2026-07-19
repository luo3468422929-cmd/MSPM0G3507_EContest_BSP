/**
 * @file track_protocol.c
 * @brief 实现 Path Fish 12 路循迹模块双半帧的校验与拼接。
 */
#include "track_protocol.h"

void TrackProtocol_Reset(TrackProtocol_t *parser)
{
    if (parser == NULL) {
        return;
    }
    parser->pendingMask = 0U;
    parser->receivedHalves = 0U;
}

Status_t TrackProtocol_PushHalf(TrackProtocol_t *parser,
                                const uint8_t *frame,
                                size_t length,
                                uint16_t *completeMask)
{
    uint8_t baseIndex;
    uint8_t halfFlag;
    uint16_t halfBits = 0U;
    uint16_t halfMask;

    if ((parser == NULL) || (frame == NULL) || (completeMask == NULL) ||
        (length != TRACK_PROTOCOL_HALF_FRAME_SIZE)) {
        return STATUS_INVALID_PARAM;
    }

    if (frame[0] == (uint8_t)'#') {
        baseIndex = 0U;
        halfFlag = 0x01U;
        halfMask = 0x003FU;
    } else if (frame[0] == (uint8_t)'!') {
        baseIndex = 6U;
        halfFlag = 0x02U;
        halfMask = 0x0FC0U;
    } else {
        TrackProtocol_Reset(parser);
        return STATUS_ERROR;
    }

    for (uint8_t index = 0U; index < 6U; ++index) {
        if (frame[index + 1U] == (uint8_t)'1') {
            halfBits |= (uint16_t)(1U << (baseIndex + index));
        } else if (frame[index + 1U] != (uint8_t)'0') {
            TrackProtocol_Reset(parser);
            return STATUS_ERROR;
        }
    }

    /* 重复到达同一半帧时用新值覆盖，另一半仍可继续配对。 */
    parser->pendingMask = (uint16_t)((parser->pendingMask &
                                      (uint16_t)(~halfMask)) | halfBits);
    parser->receivedHalves |= halfFlag;
    if (parser->receivedHalves != 0x03U) {
        return STATUS_BUSY;
    }

    *completeMask = (uint16_t)(parser->pendingMask & TRACK_PROTOCOL_FULL_MASK);
    TrackProtocol_Reset(parser);
    return STATUS_OK;
}
