#include "led.h"

#include "bsp_config.h"

Status_t LED_Init(void)
{
    LED_Set(false);
    return STATUS_OK;
}

void LED_Set(bool on)
{
    bool level = LED_ACTIVE_HIGH ? on : !on;
    if (level) {
        DL_GPIO_setPins(LED_PORT, LED_PIN);
    } else {
        DL_GPIO_clearPins(LED_PORT, LED_PIN);
    }
}

void LED_Toggle(void)
{
    DL_GPIO_togglePins(LED_PORT, LED_PIN);
}

