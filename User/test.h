#ifndef USER_TEST_H
#define USER_TEST_H

#include "common.h"

typedef enum {
    TEST_NONE = 0,
    TEST_LED,
    TEST_KEY,
    TEST_TRACK,
    TEST_MOTOR,
    TEST_PID,
    TEST_ENCODER,
    TEST_IMU,
    TEST_LCD
} Test_Id_t;

void Test_Select(Test_Id_t id);
Test_Id_t Test_GetSelected(void);
void Test_Run(uint32_t nowMs);

#endif
