/**
 * @file key.h
 * @brief 定义 PB2 低有效按键的消抖、点击、长按和实时电平接口。
 *
 * 所属层：Hardware 输入器件层。Task 每 5 ms 扫描一次并只消费一次事件；
 * 电机安全逻辑还会读取 Key_IsPressed()，覆盖上电前已经按住的情况。
 */
#ifndef KEY_H
#define KEY_H

#include "common.h"

/** 一次性按键事件；GetEvent() 读取后会清除。 */
typedef enum {
    KEY_EVENT_NONE = 0,     /**< 没有待处理事件。 */
    KEY_EVENT_PRESSED,      /**< 消抖确认后的按下沿，急停使用此事件。 */
    KEY_EVENT_RELEASED,     /**< 长按后释放，不再额外产生 CLICKED。 */
    KEY_EVENT_CLICKED,      /**< 未达到长按时间的一次按下并释放。 */
    KEY_EVENT_LONG_PRESSED  /**< 持续达到 KEY_LONG_PRESS_MS，仅报告一次。 */
} Key_Event_t;

/** @brief 读取上电时按键电平并复位消抖/事件状态。 */
Status_t Key_Init(void);

/** @brief 用当前毫秒时基推进消抖和长按状态机，建议每 5 ms 调用。 */
void Key_Scan(uint32_t nowMs);

/** @brief 取走最近一次事件；读取后内部事件立即变为 NONE。 */
Key_Event_t Key_GetEvent(void);

/** @brief 返回消抖后的稳定按下状态；不会清除任何事件。 */
bool Key_IsPressed(void);

#endif

