#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

#include "ti_msp_dl_config.h"

/* ============================ 模块开关 ============================ */
#define CONFIG_LED_ENABLE              1
#define CONFIG_KEY_ENABLE              1
#define CONFIG_UART_ENABLE             1
#define CONFIG_IMU_ENABLE              1
#define CONFIG_MOTOR_ENABLE            1
#define CONFIG_ENCODER_ENABLE          1
#define CONFIG_TRACK_ENABLE            1
#define CONFIG_OLED_ENABLE             1

/* ============================ 系统时基 ============================ */
#define SYSTEM_TICK_HZ                 1000U
#define SYSTEM_CLOCK_HZ                32000000U

/* ============================ LED / 按键 ========================== */
#define LED_PORT                       GPIOA
#define LED_PIN                        DL_GPIO_PIN_14
#define LED_ACTIVE_HIGH                1

#define KEY_PORT                       GPIOB
#define KEY_PIN                        DL_GPIO_PIN_6
#define KEY_ACTIVE_LOW                 1
#define KEY_DEBOUNCE_MS                20U
#define KEY_LONG_PRESS_MS              800U

/* ============================ TB6612 ============================== */
#define MOTOR_PWM_INST                 TIMG0
#define MOTOR_PWM_LEFT_CC_INDEX        DL_TIMER_CC_0_INDEX
#define MOTOR_PWM_RIGHT_CC_INDEX       DL_TIMER_CC_1_INDEX
#define MOTOR_PWM_PERIOD_COUNTS        1000U
#define MOTOR_MAX_DUTY                 1000
/* TB6612/电机静摩擦补偿；精细低速闭环不需要时可设为 0。 */
#define MOTOR_MIN_START_DUTY           80
#define MOTOR_RAMP_STEP                40

#define MOTOR_AIN1_PORT                GPIOA
#define MOTOR_AIN1_PIN                 DL_GPIO_PIN_8
#define MOTOR_AIN2_PORT                GPIOA
#define MOTOR_AIN2_PIN                 DL_GPIO_PIN_9
#define MOTOR_BIN1_PORT                GPIOB
#define MOTOR_BIN1_PIN                 DL_GPIO_PIN_18
#define MOTOR_BIN2_PORT                GPIOA
#define MOTOR_BIN2_PIN                 DL_GPIO_PIN_7
#define MOTOR_STBY_PORT                GPIOB
#define MOTOR_STBY_PIN                 DL_GPIO_PIN_24
#define MOTOR_LEFT_REVERSED            0
#define MOTOR_RIGHT_REVERSED           0

/* ============================ 编码器 ============================== */
#define ENCODER_LEFT_A_PORT            GPIOA
#define ENCODER_LEFT_A_PIN             DL_GPIO_PIN_21
#define ENCODER_LEFT_B_PORT            GPIOA
#define ENCODER_LEFT_B_PIN             DL_GPIO_PIN_22
#define ENCODER_RIGHT_A_PORT           GPIOB
#define ENCODER_RIGHT_A_PIN            DL_GPIO_PIN_19
#define ENCODER_RIGHT_B_PORT           GPIOB
#define ENCODER_RIGHT_B_PIN            DL_GPIO_PIN_20
#define ENCODER_LEFT_REVERSED           0
#define ENCODER_RIGHT_REVERSED          0
#define ENCODER_PULSES_PER_MOTOR_REV    13.0f
#define ENCODER_COUNT_MULTIPLIER        2.0f
#define ENCODER_GEAR_RATIO              30.0f
#define ENCODER_WHEEL_DIAMETER_M        0.065f
#define ENCODER_SAMPLE_TIME_S           0.010f
#define ENCODER_SPEED_FILTER_ALPHA      0.35f
#define ENCODER_LEFT_IRQn               GPIO_A_INT_IRQN
#define ENCODER_RIGHT_IRQn              GPIO_B_INT_IRQN

/* ============================ UART ================================ */
#define UART_DEBUG_INST                 UART0
#define UART_DEBUG_IRQn                 UART0_INT_IRQn
#define UART_IMU_INST                   UART3
#define UART_IMU_IRQn                   UART3_INT_IRQn
#define UART_RX_BUFFER_SIZE             128U
#define UART_TX_TIMEOUT_LOOPS           200000U
#define IMU_ONLINE_TIMEOUT_MS           200U

/* ============================ OLED ================================ */
#define OLED_I2C_INST                   I2C1
#define OLED_I2C_ADDRESS                0x3CU
#define OLED_WIDTH                      128U
#define OLED_HEIGHT                     64U
#define OLED_I2C_TIMEOUT_LOOPS          200000U
#define OLED_USE_SPI                    0
#define OLED_SPI_INST                   SPI0
#define OLED_SPI_TIMEOUT_LOOPS          200000U
#define OLED_SPI_CS_PORT                GPIOB
#define OLED_SPI_CS_PIN                 DL_GPIO_PIN_14
#define OLED_SPI_DC_PORT                GPIOB
#define OLED_SPI_DC_PIN                 DL_GPIO_PIN_11
#define OLED_SPI_RESET_PORT             GPIOB
#define OLED_SPI_RESET_PIN              DL_GPIO_PIN_10

/* ============================ 八路寻迹 ============================ */
#define TRACK_SENSOR_COUNT              8U
#define TRACK_FILTER_LENGTH             4U
#define TRACK_ACTIVE_THRESHOLD          500U
#define TRACK_BLACK_IS_HIGH             1
#define TRACK_ANALOG_MODE               0
#define TRACK_ADC_INST                  ADC0
#define TRACK_ADC_TIMEOUT_LOOPS         200000U

/* 数字模式默认引脚。模拟模式时由 Track_SetRawSamples() 或 ADC 适配函数写入。 */
#define TRACK_D0_PORT GPIOA
#define TRACK_D0_PIN  DL_GPIO_PIN_24
#define TRACK_D1_PORT GPIOB
#define TRACK_D1_PIN  DL_GPIO_PIN_9
#define TRACK_D2_PORT GPIOB
#define TRACK_D2_PIN  DL_GPIO_PIN_8
#define TRACK_D3_PORT GPIOA
#define TRACK_D3_PIN  DL_GPIO_PIN_18
#define TRACK_D4_PORT GPIOA
#define TRACK_D4_PIN  DL_GPIO_PIN_17
#define TRACK_D5_PORT GPIOA
#define TRACK_D5_PIN  DL_GPIO_PIN_16
#define TRACK_D6_PORT GPIOA
#define TRACK_D6_PIN  DL_GPIO_PIN_15
#define TRACK_D7_PORT GPIOA
#define TRACK_D7_PIN  DL_GPIO_PIN_23

#endif
