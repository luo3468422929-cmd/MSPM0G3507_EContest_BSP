#ifndef BSP_UART_H
#define BSP_UART_H

#include "common.h"

typedef enum {
    UART_ID_DEBUG = 0,
    UART_ID_IMU,
    UART_ID_COUNT
} UART_Id_t;

Status_t UART_Init(void);
Status_t UART_Send(UART_Id_t id, const uint8_t *data, uint16_t length);
Status_t UART_SendString(UART_Id_t id, const char *text);
Status_t UART_Printf(const char *format, ...);
uint16_t UART_Available(UART_Id_t id);
Status_t UART_ReadByte(UART_Id_t id, uint8_t *value);
uint32_t UART_GetOverflowCount(UART_Id_t id);
void UART_RxIRQHandler(UART_Id_t id);

#endif
