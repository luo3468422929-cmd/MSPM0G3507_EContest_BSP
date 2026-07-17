#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "common.h"

typedef struct {
    uint8_t *storage;
    uint16_t capacity;
    /* ISR 生产者只写 head，主循环消费者只写 tail。二者均为单调计数。 */
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint32_t overflowCount;
    bool initialized;
} RingBuffer_t;

Status_t RingBuffer_Init(RingBuffer_t *buffer, uint8_t *storage, uint16_t capacity);
Status_t RingBuffer_Push(RingBuffer_t *buffer, uint8_t value);
Status_t RingBuffer_Pop(RingBuffer_t *buffer, uint8_t *value);
uint16_t RingBuffer_Count(const RingBuffer_t *buffer);
/** 只能在初始化阶段或确认生产者中断已关闭时清空。 */
void RingBuffer_Clear(RingBuffer_t *buffer);

#endif

