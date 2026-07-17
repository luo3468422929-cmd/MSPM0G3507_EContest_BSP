/**
 * @file led.h
 * @brief 提供板载状态 LED 的初始化、设置和翻转接口。
 *
 * 所属层：Hardware 简单器件层。LED 只用于显示运行状态，任何安全逻辑
 * 都不能依赖 LED 是否点亮；有效电平由 LED_ACTIVE_HIGH 配置。
 */
#ifndef LED_H
#define LED_H

#include "common.h"

/** @brief 将 LED 初始化为关闭状态。 */
Status_t LED_Init(void);

/** @brief 按逻辑 on/off 设置 LED，内部自动处理有效电平。 */
void LED_Set(bool on);

/** @brief 翻转当前物理输出电平，适合故障闪烁或心跳测试。 */
void LED_Toggle(void);

#endif

