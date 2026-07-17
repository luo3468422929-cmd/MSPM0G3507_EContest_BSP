/**
 * @file key.c
 * @brief 实现单按键非阻塞消抖、点击/长按事件和稳定电平查询。
 *
 * 所属层：Hardware 输入器件层。不使用阻塞延时；所有时间比较采用
 * uint32 差值。按键事件由 Task 单点消费，避免测试和正常流程重复扫描。
 */
#include "key.h"

#include "board_pins.h"
#include "user_config.h"

/* 消抖稳定值、最近原始值、变化时间及当前未消费事件。均由主循环访问。 */
static bool g_stablePressed;
static bool g_lastRawPressed;
static bool g_longReported;
static uint32_t g_rawChangedMs;
static uint32_t g_pressedMs;
static volatile Key_Event_t g_event;

static bool Key_ReadRaw(void)
{
    bool high = (DL_GPIO_readPins(PIN_KEY_PORT, PIN_KEY) != 0U);
    /* PB2 当前配置为内部上拉、按下接 GND；极性仍由宏统一适配。 */
    return KEY_ACTIVE_LOW ? !high : high;
}

Status_t Key_Init(void)
{
    /* 上电时已按住会直接成为稳定状态，不伪造新的 PRESSED 事件。 */
    g_stablePressed = Key_ReadRaw();
    g_lastRawPressed = g_stablePressed;
    g_longReported = false;
    g_rawChangedMs = 0U;
    g_pressedMs = 0U;
    g_event = KEY_EVENT_NONE;
    return STATUS_OK;
}

void Key_Scan(uint32_t nowMs)
{
    bool rawPressed = Key_ReadRaw();
    /* 原始电平每次变化都重新开始消抖计时。 */
    if (rawPressed != g_lastRawPressed) {
        g_lastRawPressed = rawPressed;
        g_rawChangedMs = nowMs;
    }
    if ((rawPressed != g_stablePressed) &&
        ((uint32_t)(nowMs - g_rawChangedMs) >= KEY_DEBOUNCE_MS)) {
        g_stablePressed = rawPressed;
        if (rawPressed) {
            g_pressedMs = nowMs;
            g_longReported = false;
            g_event = KEY_EVENT_PRESSED;
        } else {
            /* 短按释放形成 CLICKED；长按已经报告过时只形成 RELEASED。 */
            g_event = g_longReported ? KEY_EVENT_RELEASED : KEY_EVENT_CLICKED;
        }
    }
    if (g_stablePressed && !g_longReported &&
        ((uint32_t)(nowMs - g_pressedMs) >= KEY_LONG_PRESS_MS)) {
        g_longReported = true;
        g_event = KEY_EVENT_LONG_PRESSED;
    }
}

Key_Event_t Key_GetEvent(void)
{
    /* 事件为单槽“读取即消费”，Task 必须把同一 event 再传给测试模块。 */
    Key_Event_t event = g_event;
    g_event = KEY_EVENT_NONE;
    return event;
}

bool Key_IsPressed(void)
{
    return g_stablePressed;
}

