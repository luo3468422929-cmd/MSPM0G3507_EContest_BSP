#include "module_tests.h"

#include "encoder.h"
#include "imu_uart.h"
#include "led.h"
#include "motor.h"
#include "oled.h"
#include "track.h"

Status_t ModuleTest_LED(void)
{
    LED_Toggle();
    return STATUS_OK;
}

Status_t ModuleTest_Motor(uint32_t nowMs)
{
    uint32_t phase = (nowMs / 1000U) % 5U;
    (void)Motor_Enable(true);
    if (phase == 0U) { return Motor_SetDutyPair(250, 250); }
    if (phase == 1U) { return Motor_SetDutyPair(-250, -250); }
    if (phase == 2U) { return Motor_SetDutyPair(250, -250); }
    if (phase == 3U) { return Motor_SetDutyPair(-250, 250); }
    Motor_StopAll();
    return STATUS_OK;
}

Status_t ModuleTest_Encoder(void)
{
    Encoder_Data_t left;
    Encoder_Data_t right;
    Encoder_UpdateSpeed(0.010f);
    if (Encoder_GetData(ENCODER_LEFT, &left) != STATUS_OK) { return STATUS_ERROR; }
    return Encoder_GetData(ENCODER_RIGHT, &right);
}

Status_t ModuleTest_Track(void)
{
    return Track_Update();
}

Status_t ModuleTest_OLED(void)
{
    OLED_Clear();
    (void)OLED_ShowString(0U, 0U, "OLED TEST");
    (void)OLED_ShowString(0U, 8U, "0123456789");
    return OLED_Refresh();
}

Status_t ModuleTest_IMU(uint32_t nowMs)
{
    ImuSample_t sample;
    IMU_Process(nowMs);
    return IMU_GetSample(&sample);
}

