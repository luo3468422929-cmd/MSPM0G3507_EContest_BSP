/**
 * @file ring_buffer.c
 * @brief 实现支持 uint16 自然回绕的 SPSC 字节环形缓冲。
 *
 * 所属层：Bsp 通用工具。生产者先写数据再发布 head，消费者先读数据再
 * 发布 tail；当前 Cortex-M0+ 上 16 位索引读写为单条访问。
 */
#include "ring_buffer.h"

Status_t RingBuffer_Init(RingBuffer_t *buffer, uint8_t *storage, uint16_t capacity)
{
    /* 容量限制在半个计数空间内，保证 head-tail 的无符号差值含义唯一。 */
    if ((buffer == NULL) || (storage == NULL) || (capacity == 0U) ||
        (capacity > (UINT16_MAX / 2U))) {
        return STATUS_INVALID_PARAM;
    }
    buffer->storage = storage;
    buffer->capacity = capacity;
    buffer->initialized = true;
    buffer->overflowCount = 0U;
    RingBuffer_Clear(buffer);
    return STATUS_OK;
}

Status_t RingBuffer_Push(RingBuffer_t *buffer, uint8_t value)
{
    uint16_t head;
    uint16_t tailSnapshot;

    if ((buffer == NULL) || !buffer->initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    head = buffer->head;
    tailSnapshot = buffer->tail;
    /* head/tail 为单调计数，取模只用于映射到底层数组槽位。 */
    if ((uint16_t)(head - tailSnapshot) >= buffer->capacity) {
        buffer->overflowCount++;
        return STATUS_OVERFLOW;
    }
    buffer->storage[head % buffer->capacity] = value;
    /* 数据写完后再发布新 head；消费者不会读取尚未写完的槽位。 */
    buffer->head = (uint16_t)(head + 1U);
    return STATUS_OK;
}

Status_t RingBuffer_Pop(RingBuffer_t *buffer, uint8_t *value)
{
    uint16_t headSnapshot;
    uint16_t tail;

    if ((buffer == NULL) || !buffer->initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    if (value == NULL) {
        return STATUS_INVALID_PARAM;
    }
    tail = buffer->tail;
    headSnapshot = buffer->head;
    if (tail == headSnapshot) {
        return STATUS_EMPTY;
    }
    /* 只有生产者发布 head 后，消费者才会到达对应槽位。 */
    *value = buffer->storage[tail % buffer->capacity];
    buffer->tail = (uint16_t)(tail + 1U);
    return STATUS_OK;
}

uint16_t RingBuffer_Count(const RingBuffer_t *buffer)
{
    uint16_t headSnapshot;
    uint16_t tailSnapshot;
    uint16_t count;

    if ((buffer == NULL) || !buffer->initialized) {
        return 0U;
    }
    headSnapshot = buffer->head;
    tailSnapshot = buffer->tail;
    count = (uint16_t)(headSnapshot - tailSnapshot);
    return (count > buffer->capacity) ? buffer->capacity : count;
}

void RingBuffer_Clear(RingBuffer_t *buffer)
{
    if (buffer == NULL) {
        return;
    }
    buffer->head = 0U;
    buffer->tail = 0U;
}

