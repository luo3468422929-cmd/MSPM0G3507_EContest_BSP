#include "uart.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "board_pins.h"
#include "ring_buffer.h"
#include "user_config.h"

typedef struct {
    UART_Regs *instance;
    RingBuffer_t rx;
    uint8_t storage[UART_RX_BUFFER_SIZE];
} UART_Context_t;

static UART_Context_t g_uart[UART_ID_COUNT];

static bool UART_IdIsValid(UART_Id_t id)
{
    return (id >= UART_ID_DEBUG) && (id < UART_ID_COUNT);
}

Status_t UART_Init(void)
{
    g_uart[UART_ID_DEBUG].instance = PIN_UART_DEBUG_INST;
    g_uart[UART_ID_IMU].instance = PIN_UART_IMU_INST;
    for (uint8_t index = 0U; index < (uint8_t)UART_ID_COUNT; ++index) {
        Status_t status = RingBuffer_Init(&g_uart[index].rx,
                                          g_uart[index].storage,
                                          UART_RX_BUFFER_SIZE);
        if (status != STATUS_OK) {
            return status;
        }
    }
    NVIC_ClearPendingIRQ(PIN_UART_DEBUG_IRQn);
    NVIC_EnableIRQ(PIN_UART_DEBUG_IRQn);
    NVIC_ClearPendingIRQ(PIN_UART_IMU_IRQn);
    NVIC_EnableIRQ(PIN_UART_IMU_IRQn);
    return STATUS_OK;
}

Status_t UART_Send(UART_Id_t id, const uint8_t *data, uint16_t length)
{
    uint32_t timeout;
    if (!UART_IdIsValid(id) || ((data == NULL) && (length != 0U))) {
        return STATUS_INVALID_PARAM;
    }
    for (uint16_t index = 0U; index < length; ++index) {
        timeout = UART_TX_TIMEOUT_LOOPS;
        /* FIFO 有空位即可写下一个字节，无需等待整帧完全发送完。 */
        while (DL_UART_Main_isTXFIFOFull(g_uart[id].instance) &&
               (timeout > 0U)) {
            timeout--;
        }
        if (timeout == 0U) {
            return STATUS_TIMEOUT;
        }
        DL_UART_Main_transmitData(g_uart[id].instance, data[index]);
    }
    return STATUS_OK;
}

Status_t UART_SendString(UART_Id_t id, const char *text)
{
    if (text == NULL) {
        return STATUS_INVALID_PARAM;
    }
    return UART_Send(id, (const uint8_t *)text, (uint16_t)strlen(text));
}

Status_t UART_Printf(const char *format, ...)
{
    char buffer[128];
    int length;
    va_list arguments;
    if (format == NULL) { return STATUS_INVALID_PARAM; }
    va_start(arguments, format);
    length = vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);
    if (length < 0) { return STATUS_ERROR; }
    if ((size_t)length >= sizeof(buffer)) { length = (int)sizeof(buffer) - 1; }
    return UART_Send(UART_ID_DEBUG, (const uint8_t *)buffer, (uint16_t)length);
}

uint16_t UART_Available(UART_Id_t id)
{
    return UART_IdIsValid(id) ? RingBuffer_Count(&g_uart[id].rx) : 0U;
}

Status_t UART_ReadByte(UART_Id_t id, uint8_t *value)
{
    if (!UART_IdIsValid(id)) {
        return STATUS_INVALID_PARAM;
    }
    return RingBuffer_Pop(&g_uart[id].rx, value);
}

uint32_t UART_GetOverflowCount(UART_Id_t id)
{
    return UART_IdIsValid(id) ? g_uart[id].rx.overflowCount : 0U;
}

void UART_RxIRQHandler(UART_Id_t id)
{
    if (UART_IdIsValid(id)) {
        /* 一次中断排空当前 FIFO，避免高波特率连续数据残留并溢出。 */
        while (!DL_UART_Main_isRXFIFOEmpty(g_uart[id].instance)) {
            (void)RingBuffer_Push(
                &g_uart[id].rx,
                DL_UART_Main_receiveData(g_uart[id].instance));
        }
    }
}

void UART0_IRQHandler(void)
{
    if (DL_UART_getPendingInterrupt(PIN_UART_DEBUG_INST) == DL_UART_IIDX_RX) {
        UART_RxIRQHandler(UART_ID_DEBUG);
    }
}

void UART2_IRQHandler(void)
{
    if (DL_UART_getPendingInterrupt(PIN_UART_IMU_INST) == DL_UART_IIDX_RX) {
        UART_RxIRQHandler(UART_ID_IMU);
    }
}
