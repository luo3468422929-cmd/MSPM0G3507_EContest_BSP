#include "ring_buffer.h"

Status_t RingBuffer_Init(RingBuffer_t *buffer, uint8_t *storage, uint16_t capacity)
{
    if ((buffer == NULL) || (storage == NULL) || (capacity == 0U)) {
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
    if ((buffer == NULL) || !buffer->initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    if (buffer->count >= buffer->capacity) {
        buffer->overflowCount++;
        return STATUS_OVERFLOW;
    }
    buffer->storage[buffer->head] = value;
    buffer->head = (uint16_t)((buffer->head + 1U) % buffer->capacity);
    buffer->count++;
    return STATUS_OK;
}

Status_t RingBuffer_Pop(RingBuffer_t *buffer, uint8_t *value)
{
    if ((buffer == NULL) || !buffer->initialized) {
        return STATUS_NOT_INITIALIZED;
    }
    if (value == NULL) {
        return STATUS_INVALID_PARAM;
    }
    if (buffer->count == 0U) {
        return STATUS_EMPTY;
    }
    *value = buffer->storage[buffer->tail];
    buffer->tail = (uint16_t)((buffer->tail + 1U) % buffer->capacity);
    buffer->count--;
    return STATUS_OK;
}

uint16_t RingBuffer_Count(const RingBuffer_t *buffer)
{
    if ((buffer == NULL) || !buffer->initialized) {
        return 0U;
    }
    return buffer->count;
}

void RingBuffer_Clear(RingBuffer_t *buffer)
{
    if (buffer == NULL) {
        return;
    }
    buffer->head = 0U;
    buffer->tail = 0U;
    buffer->count = 0U;
}

