#include "line_follow.h"

#include "motor_control.h"
#include "pid.h"
#include "track.h"

static PID_t g_steeringPid;
static LineFollow_Data_t g_data;

Status_t LineFollow_Init(void)
{
    const PID_Config_t config = {
        .kp = 20.0f, .ki = 0.0f, .kd = 2.0f, .sampleTimeS = 0.010f,
        .outputMin = -120.0f, .outputMax = 120.0f,
        .integralMin = -20.0f, .integralMax = 20.0f,
        .integralSeparation = 3.0f, .deadband = 0.02f
    };
    g_data = (LineFollow_Data_t){.baseSpeedRpm = 80.0f};
    return PID_Init(&g_steeringPid, &config);
}

void LineFollow_SetBaseSpeed(float rpm) { g_data.baseSpeedRpm = rpm; }

void LineFollow_Update(void)
{
    const Track_Data_t *track = Track_GetData();
    g_data.positionError = track->positionError;
    g_data.lineFound = (track->state != TRACK_STATE_LOST);
    if (!g_data.lineFound) {
        /* 丢线时保持低速向最后偏差方向搜索，防止立即高速冲出赛道。 */
        float search = (g_data.positionError >= 0.0f) ? 25.0f : -25.0f;
        MotorControl_SetTarget(-search, search);
        return;
    }
    g_data.steering = PID_UpdatePosition(&g_steeringPid, 0.0f,
                                         g_data.positionError);
    MotorControl_SetTarget(g_data.baseSpeedRpm - g_data.steering,
                           g_data.baseSpeedRpm + g_data.steering);
}

const LineFollow_Data_t *LineFollow_GetData(void) { return &g_data; }

