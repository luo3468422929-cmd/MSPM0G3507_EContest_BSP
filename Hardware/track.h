/**
 * @file track.h
 * @brief 定义 Path Fish 12 路 MCU 灰度模块的统一寻迹数据接口。
 *
 * 所属层：Hardware 传感器层。适配器从 I2C 地址 0x48 直接读取两个 7 字节
 * 半帧；上层始终通过 Track_Update/GetData 使用，不接触模块协议。
 */
#ifndef TRACK_H
#define TRACK_H

#include "common.h"
#include "track_math.h"
#include "user_config.h"

/** 一帧循迹结果、阈值/校准状态和通信健康状态。 */
typedef struct {
    uint16_t raw[TRACK_CHANNEL_COUNT];       /**< 逻辑黑=1000、白=0 的原始值。 */
    uint16_t filtered[TRACK_CHANNEL_COUNT];  /**< 当前数字适配器等于 raw。 */
    uint16_t minimum[TRACK_CHANNEL_COUNT];   /**< 校准期间各路最小值。 */
    uint16_t maximum[TRACK_CHANNEL_COUNT];   /**< 校准期间各路最大值。 */
    uint16_t threshold[TRACK_CHANNEL_COUNT]; /**< 各路黑线判定阈值。 */
    float positionError; /**< 加权位置，负值偏左、正值偏右。 */
    uint16_t activeMask; /**< bit0=逻辑最左 X1，bit11=逻辑最右 X12。 */
    Track_State_t state; /**< 丢线、普通线或全路触发。 */
    bool lineFound;      /**< 当前帧是否允许控制层继续循迹。 */
    bool calibrated;     /**< 软件阈值校准是否成功完成。 */
    bool communicationOk; /**< 最近一次 I2C/注入读取是否成功。 */
} Track_Data_t;

/** @brief 初始化 12 路 I2C MCU 灰度模块适配器和默认阈值。 */
Status_t Track_Init(void);
/** @brief 读取模块并更新 activeMask/positionError；通信失败会标记丢线。 */
Status_t Track_Update(void);
/** @brief 注入恰好 12 路原始值，供 Host/离线测试在下一次 Update 使用。 */
Status_t Track_SetRawSamples(const uint16_t *samples, uint8_t count);

/** @brief 修改单路 0~1000 判黑阈值。 */
Status_t Track_SetThreshold(uint8_t channel, uint16_t threshold);

/** @brief 清除各路最小/最大记录并开始一次软件阈值校准。 */
void Track_CalibrationReset(void);

/** @brief 把一帧 12 路样本纳入最小/最大统计；参数非法时忽略。 */
void Track_CalibrationFeed(const uint16_t *samples, uint8_t count);

/** @brief 用各路 (minimum+maximum)/2 生成阈值；无有效跨度时返回错误。 */
Status_t Track_CalibrationFinish(void);

/** @brief 返回内部最新数据的只读指针；调用者不得修改或长期缓存。 */
const Track_Data_t *Track_GetData(void);

/** @brief 快速返回最近一次位置误差；通信状态仍应通过 GetData 检查。 */
float Track_GetPositionError(void);

#endif
