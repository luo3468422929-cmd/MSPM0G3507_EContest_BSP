#include "key.h"

#include "bsp_config.h"

static bool g_stablePressed;
static bool g_lastRawPressed;
static bool g_longReported;
static uint32_t g_rawChangedMs;
static uint32_t g_pressedMs;
static volatile Key_Event_t g_event;

static bool Key_ReadRaw(void)
{
    bool high = (DL_GPIO_readPins(KEY_PORT, KEY_PIN) != 0U);
    return KEY_ACTIVE_LOW ? !high : high;
}

Status_t Key_Init(void)
{
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
    Key_Event_t event = g_event;
    g_event = KEY_EVENT_NONE;
    return event;
}

bool Key_IsPressed(void)
{
    return g_stablePressed;
}

