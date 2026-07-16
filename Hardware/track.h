#ifndef TRACK_H
#define TRACK_H

#include "common.h"
#include "track_math.h"
#include "user_config.h"

typedef struct {
    uint16_t raw[TRACK_CHANNEL_COUNT];
    uint16_t filtered[TRACK_CHANNEL_COUNT];
    uint16_t minimum[TRACK_CHANNEL_COUNT];
    uint16_t maximum[TRACK_CHANNEL_COUNT];
    uint16_t threshold[TRACK_CHANNEL_COUNT];
    float positionError;
    uint8_t activeMask;
    Track_State_t state;
    bool lineFound;
    bool calibrated;
    bool communicationOk;
} Track_Data_t;

/** 初始化八路 I2C MCU 灰度模块适配器。 */
Status_t Track_Init(void);
/** 读取模块、更新 activeMask/positionError；通信失败返回错误。 */
Status_t Track_Update(void);
/** 注入一帧原始值，供主机测试或离线算法测试复用。 */
Status_t Track_SetRawSamples(const uint16_t *samples, uint8_t count);
Status_t Track_SetThreshold(uint8_t channel, uint16_t threshold);
void Track_CalibrationReset(void);
void Track_CalibrationFeed(const uint16_t *samples, uint8_t count);
Status_t Track_CalibrationFinish(void);
const Track_Data_t *Track_GetData(void);
float Track_GetPositionError(void);

#endif
