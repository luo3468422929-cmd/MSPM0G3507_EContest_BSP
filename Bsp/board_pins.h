#ifndef BOARD_PINS_H
#define BOARD_PINS_H

/*
 * SysConfig 逻辑名称到驱动代码的唯一映射层。
 * 不要手改这里的物理引脚；在 empty.syscfg 改完后重新生成即可。
 */
#include "ti_msp_dl_config.h"

#define PIN_LED_PORT                      GPIO_A_PORT
#define PIN_LED                           GPIO_A_BOARD_LED_PIN
#define PIN_KEY_PORT                      GPIO_B_PORT
#define PIN_KEY                           GPIO_B_USER_KEY_PIN

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

#define PIN_ENCODER_LEFT_CAPTURE_INST     ENCODER_LEFT_CAPTURE_INST
#define PIN_ENCODER_LEFT_CAPTURE_IRQn     ENCODER_LEFT_CAPTURE_INST_INT_IRQN
#define PIN_ENCODER_LEFT_A_PORT           GPIO_ENCODER_LEFT_CAPTURE_C0_PORT
#define PIN_ENCODER_LEFT_A                GPIO_ENCODER_LEFT_CAPTURE_C0_PIN
#define PIN_ENCODER_LEFT_B_PORT           GPIO_A_PORT
#define PIN_ENCODER_LEFT_B                GPIO_A_ENCODER_LEFT_B_PIN
#define PIN_ENCODER_RIGHT_CAPTURE_INST    ENCODER_RIGHT_CAPTURE_INST
#define PIN_ENCODER_RIGHT_CAPTURE_IRQn    ENCODER_RIGHT_CAPTURE_INST_INT_IRQN
#define PIN_ENCODER_RIGHT_A_PORT          GPIO_ENCODER_RIGHT_CAPTURE_C1_PORT
#define PIN_ENCODER_RIGHT_A               GPIO_ENCODER_RIGHT_CAPTURE_C1_PIN
#define PIN_ENCODER_RIGHT_B_PORT          GPIO_B_PORT
#define PIN_ENCODER_RIGHT_B               GPIO_B_ENCODER_RIGHT_B_PIN

#define PIN_UART_DEBUG_INST               DEBUG_UART_INST
#define PIN_UART_DEBUG_IRQn               DEBUG_UART_INST_INT_IRQN
#define PIN_UART_IMU_INST                 IMU_UART_INST
#define PIN_UART_IMU_IRQn                 IMU_UART_INST_INT_IRQN
#define PIN_TRACK_I2C_INST                TRACK_I2C_INST

#define PIN_LCD_SPI_INST                  LCD_SPI_INST
#define PIN_LCD_CS_PORT                   GPIO_A_PORT
#define PIN_LCD_CS                        GPIO_A_LCD_CS_PIN
#define PIN_LCD_DC_PORT                   GPIO_A_PORT
#define PIN_LCD_DC                        GPIO_A_LCD_DC_PIN
#define PIN_LCD_RES_PORT                  GPIO_A_PORT
#define PIN_LCD_RES                       GPIO_A_LCD_RES_PIN
#define PIN_LCD_BL_PORT                   GPIO_A_PORT
#define PIN_LCD_BL                        GPIO_A_LCD_BL_PIN

#endif
