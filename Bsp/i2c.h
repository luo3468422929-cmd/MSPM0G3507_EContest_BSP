/**
 * @file i2c.h
 * @brief 提供带有限超时与可配置恢复重试的 I2C 寄存器读写接口。
 *
 * 所属层：Bsp 总线层。当前实例由 board_pins.h 映射到循迹 I2C0；具体
 * 设备地址和寄存器含义由 Hardware 层决定。重试次数由
 * I2C_RECOVERY_RETRY_COUNT 配置，当前默认一次；SDA/SCL 必须上拉到 3.3 V。
 */
#ifndef I2C_H
#define I2C_H

#include "common.h"

/**
 * @brief 从 I2C 设备的寄存器读取连续字节。
 * @param address 7 位从设备地址，不要传入左移后的地址。
 * @param reg 寄存器地址。
 * @param data 接收缓冲区。
 * @param length 读取字节数，不能为 0。
 * @return STATUS_OK 表示完成；参数、总线或超时错误返回对应状态。
 */
Status_t I2C_ReadRegister(uint8_t address, uint8_t reg,
                          uint8_t *data, uint8_t length);

/**
 * @brief 向 I2C 设备的寄存器写入连续字节。
 * @param address 7 位从设备地址，不要传入左移后的地址。
 * @param reg 寄存器地址。
 * @param data 待发送缓冲区；length 为 0 时允许为 NULL。
 * @param length 写入字节数。
 * @return STATUS_OK 表示完成；参数、总线或超时错误返回对应状态。
 * @note 当前轮询封装一次最多写入 3 个数据字节，足够覆盖本项目的寄存器操作。
 */
Status_t I2C_WriteRegister(uint8_t address, uint8_t reg,
                           const uint8_t *data, uint8_t length);

#endif
