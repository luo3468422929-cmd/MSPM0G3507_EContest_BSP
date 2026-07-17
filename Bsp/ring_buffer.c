#include "ring_buffer.h"

Status_t RingBuffer_Init(RingBuffer_t *buffer, uint8_t *storage, uint16_t capacity)
{
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

