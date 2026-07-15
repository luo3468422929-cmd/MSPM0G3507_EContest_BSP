#ifndef IMU_PROTOCOL_H
#define IMU_PROTOCOL_H

#include "common.h"

typedef struct {
    float yawDeg;
    float gyroZDps;
    uint32_t updateTickMs;
    uint32_t frameCount;
    uint32_t errorCount;
    bool yawValid;
    bool gyroZValid;
} ImuSample_t;

typedef struct {
    uint8_t frame[5];
    uint8_t index;
    ImuSample_t sample;
    bool newData;
} ImuProtocol_t;

void ImuProtocol_Init(ImuProtocol_t *parser);
Status_t ImuProtocol_PushByte(ImuProtocol_t *parser, uint8_t value, uint32_t nowMs);
Status_t ImuProtocol_GetSample(ImuProtocol_t *parser, ImuSample_t *sample);

#endif

