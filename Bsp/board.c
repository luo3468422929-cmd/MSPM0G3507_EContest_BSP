/**
 * @file board.c
 * @brief 实现整板安全初始化顺序，并集中处理初始化失败后的电机急停。
 *
 * 所属层：Bsp。SYSCFG_DL_init() 先建立时钟、复用和外设寄存器，随后只
 * 初始化 user_config.h 中启用的模块。比赛增加模块时应在这里安排依赖顺序。
 */
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

/** 初始化步骤的统一错误出口，避免某模块失败后电机仍保持使能。 */
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

    /*
     * SysConfig 必须最先执行；Timer 为后续 LCD 复位延时和任务调度提供时基。
     * 通信通道先于使用它们的惯导初始化，执行器则在控制/传感器之前保持停止。
     */
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
    /* 功能关闭时不引用电机接口，保证 CONFIG_* 开关的编译依赖成立。 */
#if CONFIG_MOTOR_ENABLE
    Motor_EmergencyStop();
#endif
}
