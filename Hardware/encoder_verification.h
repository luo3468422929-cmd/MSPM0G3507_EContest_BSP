/**
 * @file encoder_verification.h
 * @brief 提供“手转输出轴多圈”校验每圈计数和平均 RPM 的纯计算工具。
 *
 * 所属层：Hardware 校验辅助。该头文件不访问 GPIO、时基或串口，由
 * TEST_ENCODER 传入起止计数和时间，也可直接在 Host 单元测试执行。
 */
#ifndef ENCODER_VERIFICATION_H
#define ENCODER_VERIFICATION_H

#include "common.h"

/** 一次人工多圈校验的状态和最终结果。 */
typedef struct {
    bool active;           /**< 已按第一次键，正在等待转完目标圈数。 */
    uint8_t targetTurns;   /**< 人工承诺转动的输出轴圈数。 */
    uint32_t startTickMs;  /**< 开始时系统毫秒。 */
    uint32_t elapsedMs;    /**< 完成后实际耗时。 */
    int32_t startCount[2]; /**< 开始时左右累计计数。 */
    int32_t deltaCount[2]; /**< 完成时左右有符号计数差。 */
    float countsPerRev[2]; /**< 左右轮绝对计数/输出轴圈。 */
    float averageRpm;      /**< 按人工圈数和耗时换算的平均 RPM。 */
    bool hasResult;        /**< Finish 成功后结果有效。 */
} EncoderVerification_t;

/** @brief 安全取得 int32 的绝对值并用 uint32 表示，覆盖 INT32_MIN。 */
static inline uint32_t EncoderVerification_AbsInt32(int32_t value)
{
    /* 避免直接对 INT32_MIN 做取负运算造成有符号溢出。 */
    return (value < 0) ? ((uint32_t)(-(value + 1)) + 1U) :
                         (uint32_t)value;
}

/**
 * @brief 初始化人工校验状态。
 * @param verification 校验状态对象
 * @param targetTurns 本次人工转动的目标圈数，必须大于 0
 */
static inline Status_t EncoderVerification_Init(
    EncoderVerification_t *verification, uint8_t targetTurns)
{
    if ((verification == NULL) || (targetTurns == 0U)) {
        return STATUS_INVALID_PARAM;
    }
    *verification = (EncoderVerification_t){0};
    verification->targetTurns = targetTurns;
    return STATUS_OK;
}

/**
 * @brief 开始一次人工校验。
 * @param verification 已初始化的校验状态
 * @param leftCount 当前左轮累计计数
 * @param rightCount 当前右轮累计计数
 * @param nowMs 当前系统毫秒计数
 */
static inline Status_t EncoderVerification_Start(
    EncoderVerification_t *verification, int32_t leftCount,
    int32_t rightCount, uint32_t nowMs)
{
    if ((verification == NULL) || (verification->targetTurns == 0U)) {
        return STATUS_INVALID_PARAM;
    }
    verification->active = true;
    verification->startTickMs = nowMs;
    verification->elapsedMs = 0U;
    verification->startCount[0] = leftCount;
    verification->startCount[1] = rightCount;
    verification->deltaCount[0] = 0;
    verification->deltaCount[1] = 0;
    verification->countsPerRev[0] = 0.0f;
    verification->countsPerRev[1] = 0.0f;
    verification->averageRpm = 0.0f;
    verification->hasResult = false;
    return STATUS_OK;
}

/**
 * @brief 结束一次人工校验并计算结果。
 * @param verification 校验状态
 * @param leftCount 当前左轮累计计数
 * @param rightCount 当前右轮累计计数
 * @param nowMs 当前系统毫秒计数
 * @param leftCountsPerRev 输出左轮每圈计数（取绝对值）
 * @param rightCountsPerRev 输出右轮每圈计数（取绝对值）
 * @param averageRpm 输出本次人工转动的平均 RPM
 * @return STATUS_OK 或 STATUS_INVALID_PARAM
 *
 * 注意：averageRpm 的前提是确实转了 targetTurns 圈；方向不影响结果。
 */
static inline Status_t EncoderVerification_Finish(
    EncoderVerification_t *verification, int32_t leftCount,
    int32_t rightCount, uint32_t nowMs, float *leftCountsPerRev,
    float *rightCountsPerRev, float *averageRpm)
{
    uint32_t elapsedMs;

    if ((verification == NULL) || !verification->active ||
        (leftCountsPerRev == NULL) || (rightCountsPerRev == NULL) ||
        (averageRpm == NULL) || (verification->targetTurns == 0U)) {
        return STATUS_INVALID_PARAM;
    }
    elapsedMs = (uint32_t)(nowMs - verification->startTickMs);
    if (elapsedMs == 0U) {
        return STATUS_INVALID_PARAM;
    }

    verification->deltaCount[0] = leftCount - verification->startCount[0];
    verification->deltaCount[1] = rightCount - verification->startCount[1];
    verification->elapsedMs = elapsedMs;
    verification->active = false;

    verification->countsPerRev[0] = (float)EncoderVerification_AbsInt32(
        verification->deltaCount[0]) /
        (float)verification->targetTurns;
    verification->countsPerRev[1] = (float)EncoderVerification_AbsInt32(
        verification->deltaCount[1]) /
        (float)verification->targetTurns;
    verification->averageRpm = ((float)verification->targetTurns *
                                60000.0f) / (float)elapsedMs;
    verification->hasResult = true;
    *leftCountsPerRev = verification->countsPerRev[0];
    *rightCountsPerRev = verification->countsPerRev[1];
    *averageRpm = verification->averageRpm;
    return STATUS_OK;
}

#endif
