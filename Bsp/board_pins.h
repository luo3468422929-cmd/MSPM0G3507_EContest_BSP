/**
 * @file board_pins.h
 * @brief 将 SysConfig 生成的外设/引脚名称转换为项目内部统一名称。
 *
 * 所属层：Bsp 板级映射层。本文件不初始化硬件，只为其他驱动屏蔽
 * SysConfig 自动生成宏的具体拼写。比赛换引脚时应修改 empty.syscfg，
 * 只有逻辑名称也改变时才需要同步这里；不要在业务代码中直接写 PA/PB。
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#include "ti_msp_dl_config.h"

/* 人机输入输出：板载 LED 与 PB2 临时按键。 */
#define PIN_LED_PORT                      GPIO_A_PORT
#define PIN_LED                           GPIO_A_BOARD_LED_PIN
#define PIN_KEY_PORT                      GPIO_B_PORT
#define PIN_KEY                           GPIO_B_USER_KEY_PIN

/* TB6612：四个方向脚、STBY 以及 TIMG0 的左右 PWM 比较通道。 */
#define PIN_MOTOR_AIN1_PORT               GPIO_B_PORT
#define PIN_MOTOR_AIN1                    GPIO_B_MOTOR_AIN1_PIN
#define PIN_MOTOR_AIN2_PORT               GPIO_A_PORT
#define PIN_MOTOR_AIN2                    GPIO_A_MOTOR_AIN2_PIN
#define PIN_MOTOR_BIN1_PORT               GPIO_A_PORT
#define PIN_MOTOR_BIN1                    GPIO_A_MOTOR_BIN1_PIN
#define PIN_MOTOR_BIN2_PORT               GPIO_A_PORT
#define PIN_MOTOR_BIN2                    GPIO_A_MOTOR_BIN2_PIN
#define PIN_MOTOR_STBY_PORT               GPIO_B_PORT
#define PIN_MOTOR_STBY                    GPIO_B_MOTOR_STBY_PIN
#define PIN_MOTOR_PWM_INST                MOTOR_PWM_INST
#define PIN_MOTOR_LEFT_CC_INDEX           GPIO_MOTOR_PWM_C0_IDX
#define PIN_MOTOR_RIGHT_CC_INDEX          GPIO_MOTOR_PWM_C1_IDX

/* 左右编码器 A/B 四路 GPIO 及其所在端口的组合中断。 */
#define PIN_ENCODER_LEFT_A_PORT           GPIO_A_PORT
#define PIN_ENCODER_LEFT_A                GPIO_A_ENCODER_LEFT_A_PIN
#define PIN_ENCODER_LEFT_B_PORT           GPIO_A_PORT
#define PIN_ENCODER_LEFT_B                GPIO_A_ENCODER_LEFT_B_PIN
#define PIN_ENCODER_RIGHT_A_PORT          GPIO_B_PORT
#define PIN_ENCODER_RIGHT_A               GPIO_B_ENCODER_RIGHT_A_PIN
#define PIN_ENCODER_RIGHT_B_PORT          GPIO_B_PORT
#define PIN_ENCODER_RIGHT_B               GPIO_B_ENCODER_RIGHT_B_PIN
#define PIN_ENCODER_GPIOA_IRQn            GPIOA_INT_IRQn
#define PIN_ENCODER_GPIOB_IRQn            GPIOB_INT_IRQn

/* 调试 UART、惯导 UART 和八路循迹 I2C 的 SysConfig 实例。 */
#define PIN_UART_DEBUG_INST               DEBUG_UART_INST
#define PIN_UART_DEBUG_IRQn               DEBUG_UART_INST_INT_IRQN
#define PIN_UART_IMU_INST                 IMU_UART_INST
#define PIN_UART_IMU_IRQn                 IMU_UART_INST_INT_IRQN
#define PIN_TRACK_I2C_INST                TRACK_I2C_INST

/* ST7735S LCD：SPI 发送实例和三个控制脚；BL 已直连 3.3 V。 */
#define PIN_LCD_SPI_INST                  LCD_SPI_INST
#define PIN_LCD_CS_PORT                   GPIO_A_PORT
#define PIN_LCD_CS                        GPIO_A_LCD_CS_PIN
#define PIN_LCD_DC_PORT                   GPIO_A_PORT
#define PIN_LCD_DC                        GPIO_A_LCD_DC_PIN
#define PIN_LCD_RES_PORT                  GPIO_A_PORT
#define PIN_LCD_RES                       GPIO_A_LCD_RES_PIN

#endif
