#ifndef USER_CONFIG_H
#define USER_CONFIG_H

/*
 * 比赛现场的主要参数入口。
 * 这里只放功能开关和算法/机械参数，禁止填写 PAx、PBx 等物理引脚。
 * 物理引脚请在 empty.syscfg 中修改。
 */

/* 功能开关：1 开启，0 关闭。 */
#define CONFIG_LED_ENABLE                 1
#define CONFIG_KEY_ENABLE                 1
#define CONFIG_UART_ENABLE                1
#define CONFIG_IMU_ENABLE                 1
#define CONFIG_MOTOR_ENABLE               1
#define CONFIG_ENCODER_ENABLE             1
#define CONFIG_TRACK_ENABLE               1
#define CONFIG_LCD_ENABLE                 1

/* 系统节拍。 */
#define SYSTEM_TICK_HZ                    1000U
#define SYSTEM_CLOCK_HZ                   32000000U

/* LED 与按键。临时按键为低电平有效，已在 SysConfig 中配置上拉。 */
#define LED_ACTIVE_HIGH                   1
#define KEY_ACTIVE_LOW                    1
#define KEY_DEBOUNCE_MS                   20U
#define KEY_LONG_PRESS_MS                 800U
#define AUTO_START_ON_BOOT                1     /* 正常模式上电后自动进入 RUN。 */

/* TB6612 和电机。方向不一致时只改 REVERSED，不改 motor.c。 */
#define MOTOR_MAX_DUTY                    1000
#define MOTOR_MIN_START_DUTY              80
#define MOTOR_RAMP_STEP                   40
#define MOTOR_TEST_DUTY                   120  /* 低占空比开环编码器诊断 PWM。 */
#define MOTOR_TEST_RUN_MS                30000U /* 每个方向持续 30 秒，便于观察异常。 */
#define MOTOR_TEST_STOP_MS                1000U /* 正反转之间的停车时间。 */
#define MOTOR_LEFT_REVERSED               0
#define MOTOR_RIGHT_REVERSED              1
#define PID_TEST_TARGET_RPM               40.0f /* 独立速度环测试目标。 */
#define PID_TEST_VOFA_ENABLE              1     /* 输出 FireWater CSV 调参帧。 */

/* 编码器与轮组机械参数。 */
#define ENCODER_LEFT_REVERSED             0
#define ENCODER_RIGHT_REVERSED            1
#define ENCODER_PULSES_PER_MOTOR_REV      11.0f /* 厂家标称：电机轴一圈 11 个信号。 */
#define ENCODER_COUNT_MULTIPLIER          2.0f  /* A/B 两相上升沿 x2 解码。 */
#define ENCODER_GEAR_RATIO                20.4545f /* 450/(11×2)，仅记录实测有效比。 */
#define ENCODER_COUNTS_PER_WHEEL_REV      450.0f /* 实测输出轴一圈 450 个计数。 */
#define ENCODER_WHEEL_DIAMETER_M          0.065f
#define ENCODER_SPEED_WINDOW_S            0.05f
#define ENCODER_SPEED_FILTER_ALPHA        0.35f
#define ENCODER_VERIFY_TURNS              10U   /* 人工校验时输出轴转动圈数。 */

/* UART 与惯导。 */
#define UART_RX_BUFFER_SIZE               128U
#define UART_TX_TIMEOUT_LOOPS             200000U
#define IMU_ONLINE_TIMEOUT_MS             200U

/* 亚博八路 MCU 灰度模块：I2C 地址/寄存器依据模块手册。 */
#define TRACK_CHANNEL_COUNT               8U
#define TRACK_I2C_ADDRESS                 0x12U
#define TRACK_I2C_STATUS_REGISTER         0x30U
#define TRACK_I2C_TIMEOUT_LOOPS           200000U
#define TRACK_BLACK_IS_HIGH               0
#define TRACK_ACTIVE_THRESHOLD            500U

/* ST7735S。若上板出现整体偏移，只微调 OFFSET；颜色方向异常再改 MADCTL。 */
#define LCD_X_OFFSET                      2U
#define LCD_Y_OFFSET                      1U
#define LCD_MADCTL                        0x00U
#define LCD_SPI_TIMEOUT_LOOPS             200000U

/* 10 ms 控制周期及默认速度。 */
#define CONTROL_SAMPLE_TIME_S             0.010f
#define CAR_DEFAULT_BASE_SPEED_RPM        30.0f

#endif
