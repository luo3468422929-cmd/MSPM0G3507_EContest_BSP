#include "test.h"
#include "encoder.h"
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
static uint8_t g_phase;
static bool g_firstRun;

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
    if (g_test == TEST_MOTOR) {
        Motor_EmergencyStop();
    }
    g_test = id;
    g_lastTickMs = 0U;
    g_phase = 0U;
    g_firstRun = true;
}

Test_Id_t Test_GetSelected(void)
{
    return g_test;
}

static void Test_RunKey(uint32_t nowMs)
{
    UART_SendString(UART_ID_DEBUG, "UART TEST\r\n");
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
    if (g_firstRun) {
        g_firstRun = false;
        (void)UART_SendString(UART_ID_DEBUG,
            "MOTOR TEST: lift wheels before running\r\n");
        (void)Motor_Enable(true);
        (void)Motor_SetDutyPair(250, 250);
        g_lastTickMs = nowMs;
        g_phase = 0U;
        return;
    }
    Motor_RampUpdate();
    if ((uint32_t)(nowMs - g_lastTickMs) < 800U) { return; }
    g_lastTickMs = nowMs;
    g_phase++;
    if (g_phase == 1U) {
        Motor_StopAll();
    } else if (g_phase == 2U) {
        (void)Motor_SetDutyPair(-250, -250);
    } else {
        Motor_EmergencyStop();
        (void)UART_SendString(UART_ID_DEBUG, "MOTOR TEST DONE\r\n");
        Test_Select(TEST_NONE);
    }
}

static void Test_RunEncoder(uint32_t nowMs)
{
    Encoder_Data_t left;
    Encoder_Data_t right;
    if ((uint32_t)(nowMs - g_lastTickMs) < 100U) { return; }
    g_lastTickMs = nowMs;
    Encoder_UpdateSpeed(0.1f);
    (void)Encoder_GetData(ENCODER_LEFT, &left);
    (void)Encoder_GetData(ENCODER_RIGHT, &right);
    (void)UART_Printf("ENC L=%ld %.1f R=%ld %.1f\r\n",
        (long)left.totalCount, (double)left.rpm,
        (long)right.totalCount, (double)right.rpm);
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
        case TEST_ENCODER: Test_RunEncoder(nowMs); break;
        case TEST_IMU: Test_RunImu(nowMs); break;
        case TEST_LCD: Test_RunLcd(); break;
        case TEST_NONE:
        default:
            break;
    }
}
