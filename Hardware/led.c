/**
 * @file led.c
 * @brief 实现板载 LED 的逻辑电平到 GPIO 物理电平转换。
 *
 * 所属层：Hardware 简单器件层。GPIO 方向和复用已由 SysConfig 配置，
 * 本文件只调用 DriverLib 改变输出状态。
 */
#include "led.h"

#include "board_pins.h"
#include "user_config.h"

Status_t LED_Init(void)
{
    LED_Set(false);
    return STATUS_OK;
}

void LED_Set(bool on)
{
    /* 上层始终使用 true=亮；板卡更换后只需改 LED_ACTIVE_HIGH。 */
    bool level = LED_ACTIVE_HIGH ? on : !on;
    if (level) {
        DL_GPIO_setPins(PIN_LED_PORT, PIN_LED);
    } else {
        DL_GPIO_clearPins(PIN_LED_PORT, PIN_LED);
    }
}

void LED_Toggle(void)
{
    DL_GPIO_togglePins(PIN_LED_PORT, PIN_LED);
}

