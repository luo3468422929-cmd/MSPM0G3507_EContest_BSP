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
#include "task_safety.h"
#include "test.h"
#include "timer.h"
#include "track.h"
#include "user_config.h"

typedef enum {
    TASK_WAIT_START = 0,
    TASK_ARMING,
    TASK_RUNNING,
    TASK_STOPPED
} Task_State_t;

static Task_State_t g_state;
static uint32_t g_armingStartMs;
static uint8_t g_validTrackFrames;
#if CONFIG_LCD_ENABLE
static uint8_t g_displayLine;
#endif

static void Task_SetLed(bool on)
{
#if CONFIG_LED_ENABLE
    LED_Set(on);
#else
    (void)on;
#endif
}

#if CONFIG_CAR_CONTROL_ENABLE
static void Task_BeginArming(uint32_t nowMs)
{
    g_state = TASK_ARMING;
    g_armingStartMs = nowMs;
    g_validTrackFrames = 0U;
    CarControl_Enable(false);
    Task_SetLed(false);
}
#endif

static void Task_EmergencyStop(void)
{
    /*
     * 这里是正常跑车和所有电机测试共用的唯一软件急停出口。
     * 停止后保持锁定，必须复位才能再次启动，避免误触二次起步。
     */
    g_state = TASK_STOPPED;
#if CONFIG_CAR_CONTROL_ENABLE
    CarControl_Enable(false);
#endif
    Board_EmergencyStop();
    Test_Select(TEST_NONE);
    Task_SetLed(false);
}

#if CONFIG_CAR_CONTROL_ENABLE
static void Task_UpdateArming(uint32_t nowMs, Status_t trackStatus)
{
    const Track_Data_t *track = Track_GetData();
    bool frameReady = !Key_IsPressed() &&
                      (trackStatus == STATUS_OK) &&
                      track->communicationOk && track->lineFound;

    if (frameReady) {
        if (g_validTrackFrames < AUTO_START_VALID_FRAMES) {
            g_validTrackFrames++;
        }
    } else {
        g_validTrackFrames = 0U;
    }

    if (((uint32_t)(nowMs - g_armingStartMs) >= AUTO_START_DELAY_MS) &&
        (g_validTrackFrames >= AUTO_START_VALID_FRAMES)) {
        CarControl_Enable(true);
        if (CarControl_GetData()->enabled) {
            g_state = TASK_RUNNING;
            Task_SetLed(true);
        }
    }
}
#endif

#if CONFIG_LCD_ENABLE
static void Task_ShowLine(uint16_t y, const char *text)
{
    char padded[22];
    (void)snprintf(padded, sizeof(padded), "%-21.21s", text);
    (void)LCD_ShowString(0U, y, padded, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
}

static void Task_UpdateDisplayLine(uint32_t nowMs)
{
    /* 每 100 ms 只写一行；五行轮换，避免一次 SPI 刷屏阻塞控制任务。 */
    switch (g_displayLine) {
        case 0U:
            Task_ShowLine(0U, (g_state == TASK_RUNNING) ? "RUN" :
                              ((g_state == TASK_ARMING) ? "ARMING" :
                              ((g_state == TASK_STOPPED) ?
                               "STOPPED" : "WAIT START")));
            break;
        case 1U:
#if CONFIG_TRACK_ENABLE
        {
            char text[22];
            const Track_Data_t *track = Track_GetData();
            (void)snprintf(text, sizeof(text), "TRK %02X E %.1f",
                           track->activeMask,
                           (double)track->positionError);
            Task_ShowLine(16U, text);
        }
#else
            Task_ShowLine(16U, "TRACK OFF");
#endif
            break;
        case 2U:
#if CONFIG_CAR_CONTROL_ENABLE
        {
            char text[22];
            const CarControl_Data_t *car = CarControl_GetData();
            (void)snprintf(text, sizeof(text), "L %.0f/%.0f",
                           (double)car->actualLeftRpm,
                           (double)car->targetLeftRpm);
            Task_ShowLine(32U, text);
        }
#else
            Task_ShowLine(32U, "CONTROL OFF");
#endif
            break;
        case 3U:
#if CONFIG_CAR_CONTROL_ENABLE
        {
            char text[22];
            const CarControl_Data_t *car = CarControl_GetData();
            (void)snprintf(text, sizeof(text), "R %.0f/%.0f",
                           (double)car->actualRightRpm,
                           (double)car->targetRightRpm);
            Task_ShowLine(48U, text);
        }
#else
            Task_ShowLine(48U, "CONTROL OFF");
#endif
            break;
        case 4U:
        default:
#if CONFIG_IMU_ENABLE
        {
            char text[22];
            ImuSample_t imu;
            if (IMU_PeekSample(&imu) == STATUS_OK) {
                (void)snprintf(text, sizeof(text), "YAW %.1f %s",
                    (double)imu.yawDeg,
                    IMU_IsOnline(nowMs) ? "OK" : "OFF");
            } else {
                (void)snprintf(text, sizeof(text), "IMU WAIT");
            }
            Task_ShowLine(64U, text);
        }
#else
            (void)nowMs;
            Task_ShowLine(64U, "IMU OFF");
#endif
            break;
    }
    g_displayLine = (uint8_t)((g_displayLine + 1U) % 5U);
}
#endif

Status_t System_Init(void)
{
    return Board_Init();
}

Status_t Task_Init(void)
{
    uint32_t nowMs;
#if CONFIG_CAR_CONTROL_ENABLE
    Status_t status = CarControl_Init();
    if (status != STATUS_OK) { return status; }
#endif
    nowMs = Timer_GetTickMs();
    Scheduler_Init(nowMs);
    Test_Select(STARTUP_TEST);
    if ((STARTUP_TEST != TEST_NONE) &&
        (Test_GetSelected() == TEST_NONE)) {
        /* 测试依赖的模块被功能开关关闭时，禁止悄悄转入自动跑车。 */
        return STATUS_NOT_SUPPORTED;
    }
    g_state = TASK_WAIT_START;
    g_armingStartMs = nowMs;
    g_validTrackFrames = 0U;
#if CONFIG_LCD_ENABLE
    g_displayLine = 0U;
#endif
#if CONFIG_CAR_CONTROL_ENABLE
    CarControl_Enable(false);
#endif
    Task_SetLed(false);

    /* 选择任何模块测试时都禁止正常小车自动启动。 */
#if CONFIG_CAR_CONTROL_ENABLE
    if ((Test_GetSelected() == TEST_NONE) && (AUTO_START_ON_BOOT != 0)) {
        Task_BeginArming(nowMs);
    }
#endif
    return STATUS_OK;
}


void Task_Run(void)
{
    uint32_t nowMs = Timer_GetTickMs();
    Key_Event_t event = KEY_EVENT_NONE;
    Test_Id_t selected;
    bool motorContext;

    /* 按键只在这里扫描一次，再把同一事件交给测试或正常状态机。 */
#if CONFIG_KEY_ENABLE
    if (Scheduler_IsDue(SCHEDULER_TASK_5_MS, nowMs)) {
        Key_Scan(nowMs);
        event = Key_GetEvent();
    }
#endif

    selected = Test_GetSelected();
    motorContext = (g_state == TASK_ARMING) ||
                   (g_state == TASK_RUNNING) ||
                   Test_UsesMotor(selected);
    if (TaskSafety_ShouldStop(motorContext,
                              event == KEY_EVENT_PRESSED,
                              Key_IsPressed())) {
        Task_EmergencyStop();
        return;
    }

    if (selected != TEST_NONE) {
        Test_Run(nowMs, event);
        if (Test_GetSelected() == TEST_NONE) {
#if CONFIG_CAR_CONTROL_ENABLE
            CarControl_Enable(false);
#endif
            g_state = TASK_WAIT_START;
            Task_SetLed(false);
        }
        return;
    }

#if CONFIG_CAR_CONTROL_ENABLE
    if ((g_state == TASK_WAIT_START) && (event == KEY_EVENT_CLICKED)) {
        Task_BeginArming(nowMs);
    }
#endif

    if (Scheduler_IsDue(SCHEDULER_TASK_10_MS, nowMs)) {
#if CONFIG_CAR_CONTROL_ENABLE
        if (g_state == TASK_RUNNING) {
            CarControl_Update(nowMs);
        } else {
            Status_t trackStatus = Track_Update();
            if (g_state == TASK_ARMING) {
                Task_UpdateArming(nowMs, trackStatus);
            }
        }
#elif CONFIG_TRACK_ENABLE
        (void)Track_Update();
#endif
    }
    if (Scheduler_IsDue(SCHEDULER_TASK_20_MS, nowMs)) {
#if CONFIG_IMU_ENABLE
        IMU_Process(nowMs);
#endif
#if CONFIG_UART_ENABLE
        Command_Process();
#endif
    }
#if CONFIG_LCD_ENABLE
    if (Scheduler_IsDue(SCHEDULER_TASK_100_MS, nowMs)) {
        Task_UpdateDisplayLine(nowMs);
    }
#endif
}
