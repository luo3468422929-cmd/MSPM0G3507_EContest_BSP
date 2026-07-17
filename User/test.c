#include "test.h"

#include <stdio.h>
#include "car_control.h"
#include "encoder.h"
#include "encoder_verification.h"
#include "imu.h"
#include "key.h"
#include "lcd.h"
#include "lcd_bitmap.h"
#include "lcd_font.h"
#include "led.h"
#include "motor.h"
#include "track.h"
#include "uart.h"
#include "user_config.h"

static Test_Id_t g_test = TEST_NONE;
static uint32_t g_lastTickMs;
static uint32_t g_lastEncoderUpdateMs;
static uint32_t g_lastPidReportMs;
static uint8_t g_phase;
static bool g_firstRun;
static EncoderVerification_t g_encoderVerification;

/* 16×16 RGB565 棋盘图，仅用于验证图片接口和高字节在前的数据格式。 */
#define LCD_CHECKER_ROW_A \
    0xFF,0xE0, 0xFF,0xE0, 0xFF,0xE0, 0xFF,0xE0, \
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, \
    0xFF,0xE0, 0xFF,0xE0, 0xFF,0xE0, 0xFF,0xE0, \
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00
#define LCD_CHECKER_ROW_B \
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, \
    0xFF,0xE0, 0xFF,0xE0, 0xFF,0xE0, 0xFF,0xE0, \
    0x00,0x00, 0x00,0x00, 0x00,0x00, 0x00,0x00, \
    0xFF,0xE0, 0xFF,0xE0, 0xFF,0xE0, 0xFF,0xE0
static const uint8_t g_lcdCheckerboard[16U * 16U * 2U] = {
    LCD_CHECKER_ROW_A, LCD_CHECKER_ROW_B,
    LCD_CHECKER_ROW_A, LCD_CHECKER_ROW_B,
    LCD_CHECKER_ROW_A, LCD_CHECKER_ROW_B,
    LCD_CHECKER_ROW_A, LCD_CHECKER_ROW_B,
    LCD_CHECKER_ROW_A, LCD_CHECKER_ROW_B,
    LCD_CHECKER_ROW_A, LCD_CHECKER_ROW_B,
    LCD_CHECKER_ROW_A, LCD_CHECKER_ROW_B,
    LCD_CHECKER_ROW_A, LCD_CHECKER_ROW_B
};
#undef LCD_CHECKER_ROW_A
#undef LCD_CHECKER_ROW_B

void Test_Select(Test_Id_t id)
{
    if ((g_test == TEST_MOTOR) || (g_test == TEST_PID)) {
        Motor_EmergencyStop();
        CarControl_Stop();
    }
    g_test = id;
    g_lastTickMs = 0U;
    g_lastEncoderUpdateMs = 0U;
    g_lastPidReportMs = 0U;
    g_phase = 0U;
    g_firstRun = true;
    if (id == TEST_ENCODER) {
        (void)EncoderVerification_Init(&g_encoderVerification,
                                       ENCODER_VERIFY_TURNS);
    }
}

Test_Id_t Test_GetSelected(void)
{
    return g_test;
}

static void Test_RunKey(uint32_t nowMs)
{
    
    Key_Event_t event;
    Key_Scan(nowMs);
    event = Key_GetEvent();
    if (event != KEY_EVENT_NONE) {
        static const char *const names[] = {
            "NONE", "PRESSED", "RELEASED", "CLICKED", "LONG"
        };
        (void)UART_Printf("KEY %s\r\n", names[event]);
    }
}

static void Test_RunTrack(uint32_t nowMs)
{
    const Track_Data_t *data;
    if ((uint32_t)(nowMs - g_lastTickMs) < 100U) { return; }
    g_lastTickMs = nowMs;
    (void)Track_Update();
    data = Track_GetData();
    (void)UART_Printf("TRACK mask=%02X error=%.2f i2c=%s\r\n",
        data->activeMask, (double)data->positionError,
        data->communicationOk ? "OK" : "ERR");
}

static void Test_RunMotor(uint32_t nowMs)
{
    Encoder_Data_t left;
    Encoder_Data_t right;
    uint32_t elapsedMs;

    if (g_firstRun) {
        g_firstRun = false;
        Encoder_Reset(ENCODER_LEFT);
        Encoder_Reset(ENCODER_RIGHT);
        (void)UART_Printf(
            "MOTOR TEST: duty=%d, run=%lu ms; lift wheels; encoder telemetry enabled\r\n",
            MOTOR_TEST_DUTY, (unsigned long)MOTOR_TEST_RUN_MS);
        (void)Motor_Enable(true);
        (void)Motor_SetDutyPair(MOTOR_TEST_DUTY, MOTOR_TEST_DUTY);
        g_lastTickMs = nowMs;
        g_lastEncoderUpdateMs = nowMs;
        g_lastPidReportMs = nowMs;
        g_phase = 0U;
        return;
    }

    /* 与正常控制环保持 10 ms 更新周期，使 50 ms 时间窗能积累到足够样本。 */
    if ((uint32_t)(nowMs - g_lastEncoderUpdateMs) >= 10U) {
        elapsedMs = (uint32_t)(nowMs - g_lastEncoderUpdateMs);
        Encoder_UpdateSpeed((float)elapsedMs / 1000.0f);
        g_lastEncoderUpdateMs = nowMs;
    }
    if ((uint32_t)(nowMs - g_lastPidReportMs) >= 100U) {
        g_lastPidReportMs = nowMs;
        (void)Encoder_GetData(ENCODER_LEFT, &left);
        (void)Encoder_GetData(ENCODER_RIGHT, &right);
        (void)UART_Printf("MOTOR ENC L=%ld RPM=%.1f R=%ld RPM=%.1f\r\n",
                          (long)left.totalCount, (double)left.rpm,
                          (long)right.totalCount, (double)right.rpm);
    }

    Motor_RampUpdate();
    if ((uint32_t)(nowMs - g_lastTickMs) <
        ((g_phase == 1U) ? MOTOR_TEST_STOP_MS : MOTOR_TEST_RUN_MS)) {
        return;
    }
    g_lastTickMs = nowMs;
    g_phase++;
    if (g_phase == 1U) {
        Motor_StopAll();
    } else if (g_phase == 2U) {
        (void)Motor_SetDutyPair(-MOTOR_TEST_DUTY, -MOTOR_TEST_DUTY);
    } else {
        Motor_EmergencyStop();
        (void)UART_SendString(UART_ID_DEBUG, "MOTOR TEST DONE\r\n");
        Test_Select(TEST_NONE);
    }
}

static void Test_RunPid(uint32_t nowMs)
{
    const CarControl_Data_t *data;
    Key_Event_t event;
    char line[22];

    if (g_firstRun) {
        g_firstRun = false;
        g_lastTickMs = nowMs;
        g_lastPidReportMs = nowMs;
        (void)CarControl_Enable(true);
        (void)UART_Printf("PID TEST: target=%.1f RPM; click key to stop\r\n",
                          (double)PID_TEST_TARGET_RPM);
        (void)LCD_Clear(LCD_COLOR_BLACK);
    }

    Key_Scan(nowMs);
    event = Key_GetEvent();
    if ((event == KEY_EVENT_CLICKED) ||
        (event == KEY_EVENT_LONG_PRESSED)) {
        (void)UART_SendString(UART_ID_DEBUG, "PID TEST STOP\r\n");
        Test_Select(TEST_NONE);
        return;
    }

    if ((uint32_t)(nowMs - g_lastTickMs) < 10U) { return; }
    g_lastTickMs = nowMs;
    CarControl_UpdateSpeedTest(nowMs, PID_TEST_TARGET_RPM,
                               PID_TEST_TARGET_RPM);
    if ((uint32_t)(nowMs - g_lastPidReportMs) < 100U) { return; }
    g_lastPidReportMs = nowMs;

    data = CarControl_GetData();
    (void)UART_Printf("PID L=%.1f/%.1f D=%.0f R=%.1f/%.1f D=%.0f\r\n",
                      (double)data->actualLeftRpm,
                      (double)data->targetLeftRpm,
                      (double)data->outputLeft,
                      (double)data->actualRightRpm,
                      (double)data->targetRightRpm,
                      (double)data->outputRight);
#if PID_TEST_VOFA_ENABLE
    /* FireWater 通道：目标左、实际左、左 PWM、目标右、实际右、右 PWM。 */
    (void)UART_Printf("pid:%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\r\n",
                      (double)data->targetLeftRpm,
                      (double)data->actualLeftRpm,
                      (double)data->outputLeft,
                      (double)data->targetRightRpm,
                      (double)data->actualRightRpm,
                      (double)data->outputRight);
#endif
    (void)snprintf(line, sizeof(line), "L %.0f/%.0f",
                   (double)data->actualLeftRpm,
                   (double)data->targetLeftRpm);
    (void)LCD_ShowString(0U, 0U, line, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "R %.0f/%.0f",
                   (double)data->actualRightRpm,
                   (double)data->targetRightRpm);
    (void)LCD_ShowString(0U, 24U, line, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
    (void)snprintf(line, sizeof(line), "D %.0f %.0f",
                   (double)data->outputLeft, (double)data->outputRight);
    (void)LCD_ShowString(0U, 48U, line, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
}

static float Test_EncoderCountErrorPercent(float countsPerRev)
{
    return (countsPerRev - ENCODER_COUNTS_PER_WHEEL_REV) * 100.0f /
           ENCODER_COUNTS_PER_WHEEL_REV;
}

static void Test_RunEncoder(uint32_t nowMs)
{
    Encoder_Data_t left;
    Encoder_Data_t right;
    Key_Event_t event;
    uint32_t elapsedMs;
    char line[22];

    /* 速度计算使用更高频率的实际 dt，校验起止计数不会被 100 ms 打印周期截断。 */
    if (g_firstRun) {
        g_firstRun = false;
        g_lastTickMs = nowMs;
        g_lastEncoderUpdateMs = nowMs;
        (void)UART_Printf("ENC TEST: click key to start %u turns\r\n",
                          (unsigned)ENCODER_VERIFY_TURNS);
        (void)LCD_Clear(LCD_COLOR_BLACK);
    }
    if ((uint32_t)(nowMs - g_lastEncoderUpdateMs) >= 10U) {
        elapsedMs = (uint32_t)(nowMs - g_lastEncoderUpdateMs);
        g_lastEncoderUpdateMs = nowMs;
        Encoder_UpdateSpeed((float)elapsedMs / 1000.0f);
    }

    /* 按键第一次点击开始，第二次点击结束人工十圈校验。 */
    Key_Scan(nowMs);
    event = Key_GetEvent();
    if (event == KEY_EVENT_CLICKED) {
        (void)Encoder_GetData(ENCODER_LEFT, &left);
        (void)Encoder_GetData(ENCODER_RIGHT, &right);
        if (!g_encoderVerification.active) {
            if (EncoderVerification_Start(&g_encoderVerification,
                                          left.totalCount, right.totalCount,
                                          nowMs) == STATUS_OK) {
                (void)UART_Printf("ENC CAL START turns=%u; turn output "
                                  "shafts then click\r\n",
                                  (unsigned)ENCODER_VERIFY_TURNS);
            }
        } else {
            float leftCountsPerRev;
            float rightCountsPerRev;
            float averageRpm;
            if (EncoderVerification_Finish(
                    &g_encoderVerification, left.totalCount,
                    right.totalCount, nowMs, &leftCountsPerRev,
                    &rightCountsPerRev, &averageRpm) == STATUS_OK) {
                (void)UART_Printf(
                    "ENC CAL DONE dL=%ld dR=%ld CPR L=%.1f R=%.1f "
                    "ERR L=%.1f%% R=%.1f%% AVG=%.1f\r\n",
                    (long)g_encoderVerification.deltaCount[0],
                    (long)g_encoderVerification.deltaCount[1],
                    (double)leftCountsPerRev, (double)rightCountsPerRev,
                    (double)Test_EncoderCountErrorPercent(leftCountsPerRev),
                    (double)Test_EncoderCountErrorPercent(rightCountsPerRev),
                    (double)averageRpm);
            }
        }
    }

    if ((uint32_t)(nowMs - g_lastTickMs) < 100U) { return; }
    g_lastTickMs = nowMs;
    (void)Encoder_GetData(ENCODER_LEFT, &left);
    (void)Encoder_GetData(ENCODER_RIGHT, &right);
    (void)UART_Printf("ENC L=%ld %.1f R=%ld %.1f\r\n",
        (long)left.totalCount, (double)left.rpm,
        (long)right.totalCount, (double)right.rpm);

    (void)snprintf(line, sizeof(line), "ENC L %.1f R %.1f",
                   (double)left.rpm, (double)right.rpm);
    (void)LCD_ShowString(0U, 0U, line, LCD_COLOR_WHITE, LCD_COLOR_BLACK);
    if (g_encoderVerification.active) {
        (void)snprintf(line, sizeof(line), "CAL: turn %02u",
                       (unsigned)ENCODER_VERIFY_TURNS);
    } else if (g_encoderVerification.hasResult) {
        (void)snprintf(line, sizeof(line), "CPR %.0f %.0f",
                       (double)g_encoderVerification.countsPerRev[0],
                       (double)g_encoderVerification.countsPerRev[1]);
    } else {
        (void)snprintf(line, sizeof(line), "CAL: click key");
    }
    (void)LCD_ShowString(0U, 24U, line, LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
    if (g_encoderVerification.hasResult) {
        (void)snprintf(line, sizeof(line), "AVG %.1f RPM",
                       (double)g_encoderVerification.averageRpm);
        (void)LCD_ShowString(0U, 48U, line, LCD_COLOR_GREEN, LCD_COLOR_BLACK);
    } else {
        (void)LCD_ShowString(0U, 48U, "                    ",
                             LCD_COLOR_BLACK, LCD_COLOR_BLACK);
    }
}

static void Test_RunImu(uint32_t nowMs)
{
    ImuSample_t sample;

    if (g_firstRun) {
        g_firstRun = false;
        (void)LCD_Clear(LCD_COLOR_BLACK);
    }
    IMU_Process(nowMs);
    if ((uint32_t)(nowMs - g_lastTickMs) < 100U) { return; }
    g_lastTickMs = nowMs;
    if (IMU_GetSample(&sample) == STATUS_OK) {
        (void)UART_Printf("IMU yaw=%.1f gyroZ=%.1f %s\r\n",
            (double)sample.yawDeg, (double)sample.gyroZDps,
            IMU_IsOnline(nowMs) ? "ONLINE" : "OFFLINE");
        (void)LCD_ShowString(0U, 0U, "IMU TEST       ",
                             LCD_COLOR_WHITE, LCD_COLOR_BLACK);
        (void)LCD_ShowString(0U, 24U, "YAW:",
                             LCD_COLOR_WHITE, LCD_COLOR_BLACK);
        (void)LCD_ShowString(36U, 24U, "       ",
                             LCD_COLOR_BLACK, LCD_COLOR_BLACK);
        (void)LCD_ShowFloat(36U, 24U, sample.yawDeg, 1U,
                            LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
        (void)LCD_ShowString(0U, 48U, "GYRO Z:",
                             LCD_COLOR_WHITE, LCD_COLOR_BLACK);
        (void)LCD_ShowString(48U, 48U, "       ",
                             LCD_COLOR_BLACK, LCD_COLOR_BLACK);
        (void)LCD_ShowFloat(48U, 48U, sample.gyroZDps, 1U,
                            LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
        (void)LCD_ShowString(0U, 72U,
                             IMU_IsOnline(nowMs) ? "ONLINE " : "OFFLINE",
                             LCD_COLOR_GREEN, LCD_COLOR_BLACK);
    } else {
        (void)UART_SendString(UART_ID_DEBUG, "IMU WAIT\r\n");
        (void)LCD_ShowString(0U, 0U, "IMU TEST       ",
                             LCD_COLOR_WHITE, LCD_COLOR_BLACK);
        (void)LCD_ShowString(0U, 24U, "IMU WAIT       ",
                             LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
        (void)LCD_ShowString(0U, 48U, "             ",
                             LCD_COLOR_BLACK, LCD_COLOR_BLACK);
        (void)LCD_ShowString(0U, 72U, "OFFLINE ",
                             LCD_COLOR_RED, LCD_COLOR_BLACK);
    }
}

static void Test_RunLcd(void)
{
    if (!g_firstRun) { return; }
    g_firstRun = false;
    (void)LCD_Clear(LCD_COLOR_BLACK);
    (void)LCD_Fill(0U, 0U, 31U, 31U, LCD_COLOR_RED);
    (void)LCD_Fill(32U, 0U, 63U, 31U, LCD_COLOR_GREEN);
    (void)LCD_Fill(64U, 0U, 95U, 31U, LCD_COLOR_BLUE);
    (void)LCD_Fill(96U, 0U, 127U, 31U, LCD_COLOR_WHITE);
    (void)LCD_ShowString(34U, 48U, "codex nb", LCD_COLOR_YELLOW,
                         LCD_COLOR_BLACK);
    (void)LCD_ShowChinese16(8U, 96U, "\xE7\x94\xB5\xE8\xB5\x9B",
                            LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
    (void)LCD_ShowBitmap(96U, 96U, 16U, 16U, g_lcdCheckerboard);
}

void Test_Run(uint32_t nowMs)
{
    switch (g_test) {
        case TEST_LED:
            if ((uint32_t)(nowMs - g_lastTickMs) >= 500U) {
                g_lastTickMs = nowMs;
                LED_Toggle();
            }
            break;
        case TEST_KEY: Test_RunKey(nowMs); break;
        case TEST_TRACK: Test_RunTrack(nowMs); break;
        case TEST_MOTOR: Test_RunMotor(nowMs); break;
        case TEST_PID: Test_RunPid(nowMs); break;
        case TEST_ENCODER: Test_RunEncoder(nowMs); break;
        case TEST_IMU: Test_RunImu(nowMs); break;
        case TEST_LCD: Test_RunLcd(); break;
        case TEST_NONE:
        default:
            break;
    }
}
