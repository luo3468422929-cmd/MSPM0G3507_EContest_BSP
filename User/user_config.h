/**
 * @file user_config.h
 * @brief 集中管理比赛现场允许快速修改的功能、机械、协议和控制参数。
 *
 * 所属层：User 配置入口。这里禁止写 PAx/PBx 物理引脚；换引脚只改
 * empty.syscfg。修改参数后先运行对应 TEST_*，再进入正常小车模式。
 */
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
#define CONFIG_CAR_CONTROL_ENABLE         1

/* 功能依赖在编译期直接报错，避免比赛现场出现“能编译但一定不能跑”。 */
#if CONFIG_CAR_CONTROL_ENABLE && \
    (!CONFIG_MOTOR_ENABLE || !CONFIG_ENCODER_ENABLE || \
     !CONFIG_TRACK_ENABLE || !CONFIG_KEY_ENABLE)
#error "Car control requires motor, encoder, track and key"
#endif
#if CONFIG_IMU_ENABLE && !CONFIG_UART_ENABLE
#error "IMU requires UART"
#endif

/*
 * 上电测试入口。正常跑车保持 TEST_NONE；测试模块时只改这里。
 * 可选值见 User/test.h，例如 TEST_LCD、TEST_TRACK、TEST_ENCODER、
 * TEST_MOTOR、TEST_PID。宏在使用处展开，因此这里不需要包含 test.h。
 */
#define STARTUP_TEST                      TEST_NONE

/* 系统节拍：当前 32 MHz / 1000 Hz = 32000 个 CPU 周期一次 SysTick。 */
#define SYSTEM_TICK_HZ                    1000U
#define SYSTEM_CLOCK_HZ                   32000000U
#if SYSTEM_TICK_HZ == 0U
#error "SYSTEM_TICK_HZ must be greater than zero"
#endif

/* LED 与按键。临时按键为低电平有效，已在 SysConfig 中配置上拉。 */
#define LED_ACTIVE_HIGH                   1
#define KEY_ACTIVE_LOW                    1
#define KEY_DEBOUNCE_MS                   20U
#define KEY_LONG_PRESS_MS                 800U
#define AUTO_START_ON_BOOT                1     /* 正常模式满足就绪条件后自动运行。 */
#define AUTO_START_DELAY_MS               1000U /* 上电后至少等待 1 秒再使能电机。 */
#define AUTO_START_VALID_FRAMES           5U    /* 连续有效循迹帧数，防止误启动。 */

/* TB6612 和电机。方向不一致时只改 REVERSED，不改 motor.c。 */
#define MOTOR_MAX_DUTY                    1000
#define MOTOR_MIN_START_DUTY              80
#define MOTOR_RAMP_STEP                   40
#define MOTOR_TEST_DUTY                   120  /* 低占空比开环编码器诊断 PWM。 */
#define MOTOR_TEST_RUN_MS                 5000U /* 每个方向持续 5 秒，便于安全测试。 */
#define MOTOR_TEST_STOP_MS                1000U /* 正反转之间的停车时间。 */
#define MOTOR_LEFT_REVERSED               0
#define MOTOR_RIGHT_REVERSED              1
#define PID_TEST_TARGET_RPM               40.0f /* 独立速度环测试目标。 */
#define PID_TEST_VOFA_ENABLE              1     /* 输出 FireWater CSV 调参帧。 */
#define PID_TEST_VOFA_PERIOD_MS           100U  /* 波形采样周期。 */
#define PID_TEST_TEXT_REPORT_MS           500U  /* 人类可读串口输出降频。 */

/*
 * 编码器与轮组机械参数：方向先用 TEST_ENCODER 手转确认；CPR 用输出轴
 * 多圈实测，不要只按厂家电机轴信号数和标称减速比猜测。
 */
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

/* UART 与惯导：循环次数是保护上限，不代表实际微秒数。 */
#define UART_RX_BUFFER_SIZE               128U
#define UART_TX_TIMEOUT_LOOPS             200000U
#define IMU_ONLINE_TIMEOUT_MS             200U

/*
 * Path Fish 12 路带 MCU 灰度模块：每次直接读取 7 字节半帧，两个半帧
 * 拼成 X1~X12。模块可用 5 V 供电，但 SDA/SCL 必须上拉到 3.3 V。
 */
#define TRACK_CHANNEL_COUNT               12U
#define TRACK_I2C_ADDRESS                 0x48U /* DriverLib 使用未左移的 7 位地址。 */
#define TRACK_I2C_TIMEOUT_LOOPS           200000U
#define I2C_RECOVERY_RETRY_COUNT           1U /* 超时/NACK 后复位控制器并重试一次。 */
#define TRACK_I2C_HALF_READS_PER_UPDATE    2U /* 一次更新最多读取 #、! 两个半帧。 */
#define TRACK_I2C_STALE_UPDATE_LIMIT       4U /* 连续缺半帧超过此值才判通信失效。 */
#define TRACK_BLACK_IS_HIGH                1 /* 若 TEST_TRACK 黑白相反，只改成 0。 */
#define TRACK_ACTIVE_THRESHOLD            500U
#define TRACK_SENSOR_REVERSED              0 /* 1：传感器左右安装方向与权重相反。 */
#define TRACK_ALL_ACTIVE_IS_LINE           0 /* 0：全黑按停止/终点处理；1：继续循迹。 */

#if (TRACK_I2C_HALF_READS_PER_UPDATE == 0U) || \
    (TRACK_I2C_STALE_UPDATE_LIMIT == 0U)
#error "Tracker half-frame read count and stale limit must be greater than zero"
#endif

/* ST7735S。若上板出现整体偏移，只微调 OFFSET；颜色方向异常再改 MADCTL。 */
#define LCD_X_OFFSET                      2U
#define LCD_Y_OFFSET                      1U
#define LCD_MADCTL                        0x00U
#define LCD_SPI_TIMEOUT_LOOPS             200000U

/* 10 ms 控制周期及正常循迹速度；修改周期必须同步 PID sampleTime。 */
#define CONTROL_SAMPLE_TIME_S             0.010f
#define CAR_DEFAULT_BASE_SPEED_RPM        30.0f
#define CAR_MAX_TARGET_RPM                120.0f
#define CAR_ALLOW_REVERSE_WHEEL            0 /* 普通循迹时禁止单轮突然反转。 */

/* 左右速度环独立参数。电机或轮组更换后可分别在这里整定。 */
#define SPEED_PID_LEFT_KP                  6.0f
#define SPEED_PID_LEFT_KI                  1.0f
#define SPEED_PID_LEFT_KD                  0.0f
#define SPEED_PID_RIGHT_KP                 6.0f
#define SPEED_PID_RIGHT_KI                 1.0f
#define SPEED_PID_RIGHT_KD                 0.0f
#define SPEED_PID_INTEGRAL_LIMIT         300.0f
#define SPEED_PID_INTEGRAL_SEPARATION    150.0f
#define SPEED_PID_DEADBAND                 0.5f

/* 循迹位置环输出的是左右轮目标转速差，不直接输出 PWM。 */
#define STEERING_PID_KP                   20.0f
#define STEERING_PID_KI                    0.0f
#define STEERING_PID_KD                    2.0f
#define STEERING_OUTPUT_LIMIT            120.0f
#define STEERING_INTEGRAL_LIMIT           20.0f
#define STEERING_INTEGRAL_SEPARATION       3.0f
#define STEERING_DEADBAND                  0.02f

#endif
