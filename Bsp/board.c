#include "board.h"

#include "board_pins.h"
#include "command.h"
#include "encoder.h"
#include "imu.h"
#include "key.h"
#include "lcd.h"
#include "led.h"
#include "motor.h"
#include "timer.h"
#include "track.h"
#include "uart.h"
#include "user_config.h"

static Status_t Board_Check(Status_t status)
{
    if (status != STATUS_OK) {
        Board_EmergencyStop();
    }
    return status;
}

Status_t Board_Init(void)
{
    Status_t status;
    SYSCFG_DL_init();

    status = Timer_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
#if CONFIG_LED_ENABLE
    status = LED_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
#endif
#if CONFIG_KEY_ENABLE
    status = Key_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
#endif
#if CONFIG_UART_ENABLE
    status = UART_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
    status = Command_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
#endif
#if CONFIG_IMU_ENABLE
    status = IMU_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
#endif
#if CONFIG_MOTOR_ENABLE
    status = Motor_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
#endif
#if CONFIG_ENCODER_ENABLE
    status = Encoder_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
#endif
#if CONFIG_TRACK_ENABLE
    status = Track_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
#endif
#if CONFIG_LCD_ENABLE
    status = LCD_Init();
    if (Board_Check(status) != STATUS_OK) { return status; }
#endif
    return STATUS_OK;
}

void Board_EmergencyStop(void)
{
#if CONFIG_MOTOR_ENABLE
    Motor_EmergencyStop();
#endif
}
