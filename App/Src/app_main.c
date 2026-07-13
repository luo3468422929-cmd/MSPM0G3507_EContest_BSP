#include "app_main.h"

#include <stdio.h>

#include "board.h"
#include "command.h"
#include "imu_uart.h"
#include "key.h"
#include "led.h"
#include "line_follow.h"
#include "motor.h"
#include "motor_control.h"
#include "oled.h"
#include "scheduler.h"
#include "timer.h"
#include "track.h"

static bool g_running;

static void App_UpdateDisplay(uint32_t nowMs)
{
    char line[22];
    ImuSample_t imu;
    const Track_Data_t *track = Track_GetData();
    const MotorControl_Data_t *motor = MotorControl_GetData();
    OLED_Clear();
    (void)OLED_ShowString(0U, 0U, g_running ? "RUN" : "STOP");
    (void)snprintf(line, sizeof(line), "TRK %02X E%.2F", track->activeMask,
                   (double)track->positionError);
    (void)OLED_ShowString(0U, 8U, line);
    (void)snprintf(line, sizeof(line), "L %.0F/%.0F", (double)motor->actualLeftRpm,
                   (double)motor->targetLeftRpm);
    (void)OLED_ShowString(0U, 16U, line);
    (void)snprintf(line, sizeof(line), "R %.0F/%.0F", (double)motor->actualRightRpm,
                   (double)motor->targetRightRpm);
    (void)OLED_ShowString(0U, 24U, line);
    if (IMU_GetSample(&imu) == STATUS_OK) {
        (void)snprintf(line, sizeof(line), "YAW %.1F %s", (double)imu.yawDeg,
                       IMU_IsOnline(nowMs) ? "OK" : "OFF");
    } else {
        (void)snprintf(line, sizeof(line), "IMU WAIT");
    }
    (void)OLED_ShowString(0U, 32U, line);
    (void)OLED_Refresh();
}

Status_t App_Init(void)
{
    Status_t status = Board_Init();
    if (status != STATUS_OK) { return status; }
    status = MotorControl_Init();
    if (status != STATUS_OK) { return status; }
    status = LineFollow_Init();
    if (status != STATUS_OK) { return status; }
    g_running = false;
    Scheduler_Init(Timer_GetTickMs());
    return STATUS_OK;
}

void App_Run(void)
{
    uint32_t nowMs = Timer_GetTickMs();
    if (Scheduler_IsDue(SCHEDULER_TASK_5_MS, nowMs)) {
        Key_Scan(nowMs);
        if (Key_GetEvent() == KEY_EVENT_CLICKED) {
            g_running = !g_running;
            MotorControl_Enable(g_running);
            LED_Set(g_running);
        }
    }
    if (Scheduler_IsDue(SCHEDULER_TASK_10_MS, nowMs)) {
        (void)Track_Update();
        if (g_running) { LineFollow_Update(); }
        MotorControl_Update();
        Motor_RampUpdate();
    }
    if (Scheduler_IsDue(SCHEDULER_TASK_20_MS, nowMs)) {
        IMU_Process(nowMs);
        Command_Process();
    }
    if (Scheduler_IsDue(SCHEDULER_TASK_100_MS, nowMs)) {
        App_UpdateDisplay(nowMs);
    }
}
