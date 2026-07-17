/**
 * @file uart.h
 * @brief 统一封装调试串口和 NCU 惯导串口的中断接收、阻塞发送接口。
 *
 * 所属层：Bsp 通信层。ISR 只把 RX FIFO 字节写入环形缓冲，协议解析和
 * printf 均在主循环执行。实例和引脚来自 SysConfig/board_pins.h。
 */
#ifndef BSP_UART_H
#define BSP_UART_H

#include "common.h"

/** 项目内 UART 逻辑编号，不等同于芯片 UART 外设编号。 */
typedef enum {
    UART_ID_DEBUG = 0, /**< 板载 CH340 调试串口 UART0。 */
    UART_ID_IMU,       /**< NCU 惯导 UART2。 */
    UART_ID_COUNT      /**< 逻辑串口数量，仅用于边界检查。 */
} UART_Id_t;

/** @brief 初始化两个 RX 环形缓冲并使能对应 NVIC 中断。 */
Status_t UART_Init(void);

/** @brief 阻塞写入指定 UART 的 TX FIFO；length 为 0 时允许 data 为 NULL。 */
Status_t UART_Send(UART_Id_t id, const uint8_t *data, uint16_t length);

/** @brief 发送以 '\0' 结尾的字符串，不发送结尾 '\0'。 */
Status_t UART_SendString(UART_Id_t id, const char *text);

/** @brief 使用 128 字节临时缓冲格式化并发送到调试 UART，超长内容会截断。 */
Status_t UART_Printf(const char *format, ...);

/** @brief 返回指定 UART 当前可读取的字节数，编号非法时返回 0。 */
uint16_t UART_Available(UART_Id_t id);

/** @brief 从 RX 环形缓冲读取一个字节；无数据时返回 STATUS_EMPTY。 */
Status_t UART_ReadByte(UART_Id_t id, uint8_t *value);

/** @brief 返回 RX 缓冲已丢弃字节的累计次数，用于诊断波特率/处理延迟。 */
uint32_t UART_GetOverflowCount(UART_Id_t id);

/** @brief UART IRQ 共用搬运函数；只允许由具体 UARTx_IRQHandler 调用。 */
void UART_RxIRQHandler(UART_Id_t id);

#endif
