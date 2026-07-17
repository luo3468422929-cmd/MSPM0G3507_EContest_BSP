#include "task.h"

#include <stdio.h>

#include "board.h"
#include "car_control.h"
#include "command.h"
#include "imu.h"
#include "key.h"
#include "lcd.h"
#include "led.h"
#include "scheduler.h"
#include "test.h"
#include "timer.h"
#include "track.h"

typedef enum {
    TASK_WAIT_START = 0,
    TASK_RUNNING,
    TASK_STOPPED
} Task_State_t;

static Task_State_t g_state;

static void Task_ShowLine(uint16_t y, const char *text)
{
    char padded[22];
    (void)snprintf(padded, sizeof(padded), "%-21.21s", text);
    (void)LCD_ShowString(0U, y, padded, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
}

static void Task_UpdateDisplay(uint32_t nowMs)
{
    char text[22];
    ImuSample_t imu;
    const Track_Data_t *track = Track_GetData();
    const CarControl_Data_t *car = CarControl_GetData();

    Task_ShowLine(0U, (g_state == TASK_RUNNING) ? "RUN" :
                      ((g_state == TASK_STOPPED) ? "STOPPED" : "WAIT START"));
    (void)snprintf(text, sizeof(text), "TRK %02X E %.1f",
                   track->activeMask, (double)track->positionError);
    Task_ShowLine(16U, text);
    (void)snprintf(text, sizeof(text), "L %.0f/%.0f",
                   (double)car->actualLeftRpm, (double)car->targetLeftRpm);
    Task_ShowLine(32U, text);
    (void)snprintf(text, sizeof(text), "R %.0f/%.0f",
                   (double)car->actualRightRpm, (double)car->targetRightRpm);
    Task_ShowLine(48U, text);
    if (IMU_GetSample(&imu) == STATUS_OK) {
        (void)snprintf(text, sizeof(text), "YAW %.1f %s",
            (double)imu.yawDeg, IMU_IsOnline(nowMs) ? "OK" : "OFF");
    } else {
        (void)snprintf(text, sizeof(text), "IMU WAIT");
    }
    Task_ShowLine(64U, text);
}

Status_t System_Init(void)
{
    return Board_Init();
}

Status_t Task_Init(void)
{
    Status_t status = CarControl_Init();
    if (status != STATUS_OK) { return status; }
    g_state = (AUTO_START_ON_BOOT != 0) ? TASK_RUNNING : TASK_WAIT_START;
    Scheduler_Init(Timer_GetTickMs());
    Test_Select(TEST_NONE); /* 单模块调试时改为 TEST_TRACK/TEST_PID 等。 */
    if (g_state == TASK_RUNNING) {
        CarControl_Enable(true);
        LED_Set(true);
    }
    return STATUS_OK;
}


void Task_Run(void)
{
    uint32_t nowMs = Timer_GetTickMs();
    Key_Event_t event;

    if (Test_GetSelected() != TEST_NONE) {
        Test_Run(nowMs);
        return;
    }
    if (Scheduler_IsDue(SCHEDULER_TASK_5_MS, nowMs)) {
        Key_Scan(nowMs);
        event = Key_GetEvent();
        if (event == KEY_EVENT_CLICKED) {
            if (g_state == TASK_RUNNING) {
                g_state = TASK_WAIT_START;
                CarControl_Enable(false);
            } else if (g_state == TASK_WAIT_START) {
                g_state = TASK_RUNNING;
                CarControl_Enable(true);
            }
            LED_Set(g_state == TASK_RUNNING);
        } else if (event == KEY_EVENT_LONG_PRESSED) {
            g_state = TASK_STOPPED;
            CarControl_Enable(false);
            Board_EmergencyStop();
            LED_Set(false);
        }
    }
    if (Scheduler_IsDue(SCHEDULER_TASK_10_MS, nowMs)) {
        if (g_state == TASK_RUNNING) {
            CarControl_Update(nowMs);
        } else {
            (void)Track_Update();
        }
    }
    if (Scheduler_IsDue(SCHEDULER_TASK_20_MS, nowMs)) {
        IMU_Process(nowMs);
        Command_Process();
    }
    if (Scheduler_IsDue(SCHEDULER_TASK_200_MS, nowMs)) {
        Task_UpdateDisplay(nowMs);
    }
}
