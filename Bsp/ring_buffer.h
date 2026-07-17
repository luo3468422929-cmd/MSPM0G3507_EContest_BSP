/**
 * @file ring_buffer.h
 * @brief 定义 UART 中断与主循环之间的单生产者/单消费者字节环形缓冲。
 *
 * ISR 只调用 Push，主循环只调用 Pop；这种所有权约束避免共享 count 的
 * 读改写竞争。若改变为多生产者或多消费者，必须重新设计同步方式。
 */
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "common.h"

/** 环形缓冲上下文；storage 由调用者提供并在整个使用期保持有效。 */
typedef struct {
    uint8_t *storage;  /**< 实际字节数组。 */
    uint16_t capacity; /**< 可保存的最大字节数。 */
    /* ISR 生产者只写 head，主循环消费者只写 tail。二者均为单调计数。 */
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint32_t overflowCount; /**< 满缓冲时丢字节的累计次数。 */
    bool initialized;                /**< Init 成功后为 true。 */
} RingBuffer_t;

/** @brief 绑定外部存储并清空缓冲；容量最大为 UINT16_MAX/2。 */
Status_t RingBuffer_Init(RingBuffer_t *buffer, uint8_t *storage, uint16_t capacity);

/** @brief 生产者写入一个字节；缓冲满时返回 STATUS_OVERFLOW。 */
Status_t RingBuffer_Push(RingBuffer_t *buffer, uint8_t value);

/** @brief 消费者读取一个字节；无数据时返回 STATUS_EMPTY。 */
Status_t RingBuffer_Pop(RingBuffer_t *buffer, uint8_t *value);

/** @brief 返回当前可读字节数；未初始化时返回 0。 */
uint16_t RingBuffer_Count(const RingBuffer_t *buffer);

/** @brief 清空读写索引；只能在初始化阶段或生产者中断已关闭时调用。 */
void RingBuffer_Clear(RingBuffer_t *buffer);

#endif

