/**
 * @file frame_protocol.c
 * @brief 实现 AA55 命令帧的逐字节状态机、自动重同步和累加和校验。
 *
 * 所属层：Bsp 纯协议工具，不访问 UART 或 DriverLib。每收到一个字节调用
 * PushByte，只有返回 STATUS_OK 后才有完整帧可取。
 */
#include "frame_protocol.h"

#include <string.h>

/** 解析阶段；状态值保存在 FrameProtocol_t.state 中。 */
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
                /* 第二个字节仍为 AA 时保留为新帧头，尽快从错位字节流恢复。 */
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
            /* 无论校验成功与否都回到帧头搜索，下一字节可开始新帧。 */
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
    /* 单槽 ready 缓冲采用“读取即消费”，上层应及时调用以免被后帧覆盖。 */
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
    /* 校验覆盖帧头、命令、长度和全部载荷，不包含校验字节自身。 */
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

