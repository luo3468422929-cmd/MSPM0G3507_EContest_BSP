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

