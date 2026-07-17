/**
 * @file imu.h
 * @brief 定义 NCU 串口惯导的接收处理、样本读取、在线判断和航向清零接口。
 *
 * 所属层：Hardware 传感器层。当前协议只提供 yaw 与 gyroZ，不生成模块
 * 未发送的 pitch/roll 数据。底层 UART2 接收由 Bsp/uart 提供。
 */
#ifndef IMU_H
#define IMU_H

#include "common.h"
#include "imu_protocol.h"

/** @brief 复位协议解析器、最新样本、新数据标志和命令状态机。 */
Status_t IMU_Init(void);

/** @brief 清空 UART2 已收字节、解析样本并推进非阻塞清零命令状态机。 */
void IMU_Process(uint32_t nowMs);
/** @brief 读取最新样本但不清除“新数据”标志，适合状态页显示。 */
Status_t IMU_PeekSample(ImuSample_t *sample);
/** @brief 读取最新样本并清除“新数据”标志，适合算法消费。 */
Status_t IMU_GetSample(ImuSample_t *sample);

/** @brief 返回上次 GetSample 后是否又解析到合法惯导帧。 */
bool IMU_HasNewData(void);

/** @brief 最近合法帧未超过 IMU_ONLINE_TIMEOUT_MS 时返回 true。 */
bool IMU_IsOnline(uint32_t nowMs);

/** @brief 启动“解锁→等待→清零→等待→保存”的非阻塞航向清零序列。 */
Status_t IMU_StartYawZero(uint32_t nowMs);

#endif

