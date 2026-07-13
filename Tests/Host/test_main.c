#ifdef HOST_TEST

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "filter_average.h"
#include "frame_protocol.h"
#include "imu_protocol.h"
#include "pid.h"
#include "ring_buffer.h"
#include "track_math.h"

static int g_failures;

#define CHECK_TRUE(expr) do { if (!(expr)) { \
    printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); g_failures++; \
} } while (0)

#define CHECK_NEAR(actual, expected, tolerance) \
    CHECK_TRUE(fabsf((actual) - (expected)) <= (tolerance))

static void Test_PID_PositionAndLimits(void)
{
    PID_t pid;
    PID_Config_t config = {
        .kp = 2.0f, .ki = 1.0f, .kd = 0.0f, .sampleTimeS = 0.1f,
        .outputMin = -5.0f, .outputMax = 5.0f,
        .integralMin = -2.0f, .integralMax = 2.0f,
        .integralSeparation = 100.0f, .deadband = 0.0f
    };

    CHECK_TRUE(PID_Init(&pid, &config) == STATUS_OK);
    CHECK_NEAR(PID_UpdatePosition(&pid, 10.0f, 0.0f), 5.0f, 0.0001f);
    CHECK_TRUE(pid.integral <= 2.0f);
    PID_Reset(&pid);
    CHECK_NEAR(pid.integral, 0.0f, 0.0001f);
}

static void Test_PID_Incremental(void)
{
    PID_t pid;
    PID_Config_t config = {
        .kp = 1.0f, .ki = 0.0f, .kd = 0.0f, .sampleTimeS = 0.01f,
        .outputMin = -100.0f, .outputMax = 100.0f,
        .integralMin = -10.0f, .integralMax = 10.0f,
        .integralSeparation = 100.0f, .deadband = 0.0f
    };
    CHECK_TRUE(PID_Init(&pid, &config) == STATUS_OK);
    CHECK_NEAR(PID_UpdateIncremental(&pid, 5.0f, 0.0f), 5.0f, 0.0001f);
    CHECK_NEAR(PID_UpdateIncremental(&pid, 5.0f, 2.0f), 3.0f, 0.0001f);
}

static void Test_Filter_MovingAverage(void)
{
    float storage[4];
    AverageFilter_t filter;
    CHECK_TRUE(AverageFilter_Init(&filter, storage, 4U) == STATUS_OK);
    CHECK_NEAR(AverageFilter_Update(&filter, 2.0f), 2.0f, 0.0001f);
    CHECK_NEAR(AverageFilter_Update(&filter, 4.0f), 3.0f, 0.0001f);
    CHECK_NEAR(AverageFilter_Update(&filter, 6.0f), 4.0f, 0.0001f);
    CHECK_NEAR(AverageFilter_Update(&filter, 8.0f), 5.0f, 0.0001f);
    CHECK_NEAR(AverageFilter_Update(&filter, 10.0f), 7.0f, 0.0001f);
}

static void Test_RingBuffer_WrapAndOverflow(void)
{
    uint8_t storage[4];
    RingBuffer_t rb;
    uint8_t value = 0U;
    CHECK_TRUE(RingBuffer_Init(&rb, storage, sizeof(storage)) == STATUS_OK);
    CHECK_TRUE(RingBuffer_Push(&rb, 1U) == STATUS_OK);
    CHECK_TRUE(RingBuffer_Push(&rb, 2U) == STATUS_OK);
    CHECK_TRUE(RingBuffer_Push(&rb, 3U) == STATUS_OK);
    CHECK_TRUE(RingBuffer_Push(&rb, 4U) == STATUS_OK);
    CHECK_TRUE(RingBuffer_Push(&rb, 5U) == STATUS_OVERFLOW);
    CHECK_TRUE(RingBuffer_Pop(&rb, &value) == STATUS_OK && value == 1U);
    CHECK_TRUE(RingBuffer_Push(&rb, 5U) == STATUS_OK);
    CHECK_TRUE(RingBuffer_Count(&rb) == 4U);
}

static void Test_ImuProtocol_ValidAndResync(void)
{
    ImuProtocol_t parser;
    ImuSample_t sample;
    const uint8_t gyroFrame[] = {0x00U, 0x5AU, 0xAAU, 0x00U, 0x40U, 0x44U};
    const uint8_t yawFrame[]  = {0x5AU, 0xBBU, 0x00U, 0x40U, 0x55U};
    ImuProtocol_Init(&parser);
    for (size_t i = 0; i < sizeof(gyroFrame); ++i) {
        ImuProtocol_PushByte(&parser, gyroFrame[i], 10U);
    }
    for (size_t i = 0; i < sizeof(yawFrame); ++i) {
        ImuProtocol_PushByte(&parser, yawFrame[i], 20U);
    }
    CHECK_TRUE(ImuProtocol_GetSample(&parser, &sample) == STATUS_OK);
    CHECK_NEAR(sample.gyroZDps, 1000.0f, 0.01f);
    CHECK_NEAR(sample.yawDeg, 90.0f, 0.01f);
    CHECK_TRUE(sample.frameCount == 2U);
    CHECK_TRUE(sample.updateTickMs == 20U);
}

static void Test_FrameProtocol_VariableLengthAndChecksum(void)
{
    FrameProtocol_t parser;
    FrameProtocol_Frame_t frame;
    /* AA 55 + CMD + LEN + PAYLOAD + 8 位累加和 */
    const uint8_t bytes[] = {0x00U, 0xAAU, 0x55U, 0x10U, 0x03U,
                             0x01U, 0x02U, 0x03U, 0x18U};
    FrameProtocol_Init(&parser);
    for (size_t index = 0U; index < sizeof(bytes); ++index) {
        FrameProtocol_PushByte(&parser, bytes[index]);
    }
    CHECK_TRUE(FrameProtocol_GetFrame(&parser, &frame) == STATUS_OK);
    CHECK_TRUE(frame.command == 0x10U);
    CHECK_TRUE(frame.length == 3U);
    CHECK_TRUE(frame.payload[2] == 0x03U);
}

static void Test_TrackMath_PositionAndLost(void)
{
    uint16_t values[8] = {0U, 0U, 0U, 200U, 1000U, 0U, 0U, 0U};
    const int8_t weights[8] = {-7, -5, -3, -1, 1, 3, 5, 7};
    TrackMath_Result_t result;
    CHECK_TRUE(TrackMath_Calculate(values, weights, 8U, 50U, &result) == STATUS_OK);
    CHECK_TRUE(result.state == TRACK_STATE_LINE);
    CHECK_TRUE(result.position > 0.0f);
    memset(values, 0, sizeof(values));
    CHECK_TRUE(TrackMath_Calculate(values, weights, 8U, 50U, &result) == STATUS_OK);
    CHECK_TRUE(result.state == TRACK_STATE_LOST);
}

int main(void)
{
    Test_PID_PositionAndLimits();
    Test_PID_Incremental();
    Test_Filter_MovingAverage();
    Test_RingBuffer_WrapAndOverflow();
    Test_ImuProtocol_ValidAndResync();
    Test_FrameProtocol_VariableLengthAndChecksum();
    Test_TrackMath_PositionAndLost();
    if (g_failures == 0) {
        puts("ALL HOST TESTS PASSED");
        return 0;
    }
    printf("%d HOST TEST(S) FAILED\n", g_failures);
    return 1;
}

#endif /* HOST_TEST */
