#include "imu_protocol.h"

#include <string.h>

static int16_t ImuProtocol_ReadInt16LE(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

void ImuProtocol_Init(ImuProtocol_t *parser)
{
    if (parser != NULL) {
        memset(parser, 0, sizeof(*parser));
    }
}

Status_t ImuProtocol_PushByte(ImuProtocol_t *parser, uint8_t value, uint32_t nowMs)
{
    uint8_t checksum;
    int16_t raw;

    if (parser == NULL) {
        return STATUS_INVALID_PARAM;
    }
    if (parser->index == 0U) {
        if (value != 0x5AU) {
            return STATUS_EMPTY;
        }
        parser->frame[parser->index++] = value;
        return STATUS_BUSY;
    }

    parser->frame[parser->index++] = value;
    if (parser->index < sizeof(parser->frame)) {
        return STATUS_BUSY;
    }

    parser->index = 0U;
    checksum = (uint8_t)(parser->frame[0] + parser->frame[1] +
                         parser->frame[2] + parser->frame[3]);
    if (checksum != parser->frame[4]) {
        parser->sample.errorCount++;
        /* 若最后一个字节恰好是新帧头，立即进入下一帧。 */
        if (parser->frame[4] == 0x5AU) {
            parser->frame[0] = 0x5AU;
            parser->index = 1U;
        }
        return STATUS_CRC_ERROR;
    }

    raw = ImuProtocol_ReadInt16LE(&parser->frame[2]);
    if (parser->frame[1] == 0xAAU) {
        parser->sample.gyroZDps = ((float)raw / 32768.0f) * 2000.0f;
        parser->sample.gyroZValid = true;
    } else if (parser->frame[1] == 0xBBU) {
        parser->sample.yawDeg = ((float)raw / 32768.0f) * 180.0f;
        parser->sample.yawValid = true;
    } else {
        parser->sample.errorCount++;
        return STATUS_NOT_SUPPORTED;
    }

    parser->sample.frameCount++;
    parser->sample.updateTickMs = nowMs;
    parser->newData = true;
    return STATUS_OK;
}

Status_t ImuProtocol_GetSample(ImuProtocol_t *parser, ImuSample_t *sample)
{
    if ((parser == NULL) || (sample == NULL)) {
        return STATUS_INVALID_PARAM;
    }
    *sample = parser->sample;
    parser->newData = false;
    return (sample->yawValid || sample->gyroZValid) ? STATUS_OK : STATUS_EMPTY;
}

