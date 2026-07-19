/**
 * @file i2c.c
 * @brief 实现 I2C0 原始/寄存器读写、有限轮询、控制器复位和有限重试。
 *
 * 所属层：Bsp 总线层。设备协议由 Hardware 层决定；所有等待都有上限，
 * 通信失败会返回上层，由控制层执行停车策略。
 */
#include "i2c.h"

#include <string.h>

#include "board_pins.h"
#include "ti/driverlib/dl_i2c.h"
#include "user_config.h"

/** 等待控制器进入空闲，同时优先报告 DriverLib 的总线错误状态。 */
static Status_t I2C_WaitIdle(I2C_Regs *instance)
{
    for (uint32_t loops = 0U; loops < TRACK_I2C_TIMEOUT_LOOPS; ++loops) {
        uint32_t status = DL_I2C_getControllerStatus(instance);
        if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
            return STATUS_ERROR;
        }
        if ((status & DL_I2C_CONTROLLER_STATUS_IDLE) != 0U) {
            return STATUS_OK;
        }
    }
    return STATUS_TIMEOUT;
}

/** 从 RX FIFO 等待并取走一个字节；不会无限卡住主循环。 */
static Status_t I2C_ReadOneByte(I2C_Regs *instance, uint8_t *value)
{
    for (uint32_t loops = 0U; loops < TRACK_I2C_TIMEOUT_LOOPS; ++loops) {
        uint32_t status = DL_I2C_getControllerStatus(instance);
        if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
            return STATUS_ERROR;
        }
        if (!DL_I2C_isControllerRXFIFOEmpty(instance)) {
            *value = DL_I2C_receiveControllerData(instance);
            return STATUS_OK;
        }
    }
    return STATUS_TIMEOUT;
}

/** MSPM0 DriverLib 接口使用未左移的 7 位地址，合法范围为 0x00~0x7F。 */
static bool I2C_AddressIsValid(uint8_t address)
{
    return address <= 0x7FU;
}

static void I2C_RecoverController(I2C_Regs *instance)
{
    /*
     * 清除上一次 NACK/超时留下的传输状态和 FIFO，再允许下一次重试。
     * 如果外设把 SDA 物理拉低，本操作无法释放线路，需要复位或断电排查。
     */
    DL_I2C_resetControllerTransfer(instance);
    DL_I2C_flushControllerTXFIFO(instance);
    DL_I2C_flushControllerRXFIFO(instance);
}

static Status_t I2C_ReadOnce(I2C_Regs *instance, uint8_t address,
                             uint8_t *data, uint8_t length)
{
    Status_t status = I2C_WaitIdle(instance);

    if (status != STATUS_OK) {
        return status;
    }

    /* 该设备没有寄存器地址阶段：清 RX FIFO 后直接发起读方向传输。 */
    DL_I2C_flushControllerRXFIFO(instance);
    DL_I2C_startControllerTransfer(instance, address,
                                   DL_I2C_CONTROLLER_DIRECTION_RX, length);
    for (uint8_t index = 0U; index < length; ++index) {
        status = I2C_ReadOneByte(instance, &data[index]);
        if (status != STATUS_OK) {
            DL_I2C_flushControllerRXFIFO(instance);
            return status;
        }
    }
    return I2C_WaitIdle(instance);
}

Status_t I2C_Read(uint8_t address, uint8_t *data, uint8_t length)
{
    I2C_Regs *instance = PIN_TRACK_I2C_INST;
    Status_t status = STATUS_ERROR;

    if (!I2C_AddressIsValid(address) || (data == NULL) || (length == 0U)) {
        return STATUS_INVALID_PARAM;
    }

    for (uint8_t attempt = 0U; attempt <= I2C_RECOVERY_RETRY_COUNT;
         ++attempt) {
        status = I2C_ReadOnce(instance, address, data, length);
        if (status == STATUS_OK) {
            return STATUS_OK;
        }
        I2C_RecoverController(instance);
    }
    return status;
}

static Status_t I2C_ReadRegisterOnce(I2C_Regs *instance, uint8_t address,
                                     uint8_t reg, uint8_t *data,
                                     uint8_t length)
{
    Status_t status;

    status = I2C_WaitIdle(instance);
    if (status != STATUS_OK) {
        return status;
    }

    /* 第一次传输只写寄存器地址；等待完成后再单独发起读传输。 */
    DL_I2C_flushControllerTXFIFO(instance);
    /* 读取阶段由硬件产生读方向和停止条件，软件逐字节清空 RX FIFO。 */
    DL_I2C_flushControllerRXFIFO(instance);
    if (DL_I2C_fillControllerTXFIFO(instance, &reg, 1U) != 1U) {
        return STATUS_OVERFLOW;
    }
    DL_I2C_startControllerTransfer(instance, address,
                                   DL_I2C_CONTROLLER_DIRECTION_TX, 1U);
    status = I2C_WaitIdle(instance);
    if (status != STATUS_OK) {
        DL_I2C_flushControllerTXFIFO(instance);
        return status;
    }

    DL_I2C_flushControllerRXFIFO(instance);
    DL_I2C_startControllerTransfer(instance, address,
                                   DL_I2C_CONTROLLER_DIRECTION_RX, length);
    for (uint8_t index = 0U; index < length; ++index) {
        status = I2C_ReadOneByte(instance, &data[index]);
        if (status != STATUS_OK) {
            DL_I2C_flushControllerRXFIFO(instance);
            return status;
        }
    }
    return I2C_WaitIdle(instance);
}

Status_t I2C_ReadRegister(uint8_t address, uint8_t reg,
                          uint8_t *data, uint8_t length)
{
    I2C_Regs *instance = PIN_TRACK_I2C_INST;
    Status_t status;

    if (!I2C_AddressIsValid(address) || (data == NULL) || (length == 0U)) {
        return STATUS_INVALID_PARAM;
    }

    /* 首次失败后先复位传输状态，再按配置次数重新发起完整寄存器读取。 */
    for (uint8_t attempt = 0U; attempt <= I2C_RECOVERY_RETRY_COUNT;
         ++attempt) {
        status = I2C_ReadRegisterOnce(instance, address, reg, data, length);
        if (status == STATUS_OK) {
            return STATUS_OK;
        }
        I2C_RecoverController(instance);
    }
    return status;
}

static Status_t I2C_WriteRegisterOnce(I2C_Regs *instance, uint8_t address,
                                      uint8_t reg, const uint8_t *data,
                                      uint8_t length)
{
    uint8_t packet[4];
    Status_t status;

    /* 寄存器地址和数据放入同一 TX 传输；packet 容量限制数据最多 3 字节。 */
    packet[0] = reg;
    if (length != 0U) {
        (void)memcpy(&packet[1], data, length);
    }
    status = I2C_WaitIdle(instance);
    if (status != STATUS_OK) {
        return status;
    }

    DL_I2C_flushControllerTXFIFO(instance);
    if (DL_I2C_fillControllerTXFIFO(instance, packet,
                                    (uint16_t)length + 1U) !=
        ((uint16_t)length + 1U)) {
        return STATUS_OVERFLOW;
    }
    DL_I2C_startControllerTransfer(instance, address,
                                   DL_I2C_CONTROLLER_DIRECTION_TX,
                                   (uint16_t)length + 1U);
    status = I2C_WaitIdle(instance);
    DL_I2C_flushControllerTXFIFO(instance);
    return status;
}

Status_t I2C_WriteRegister(uint8_t address, uint8_t reg,
                           const uint8_t *data, uint8_t length)
{
    I2C_Regs *instance = PIN_TRACK_I2C_INST;
    Status_t status;

    if (!I2C_AddressIsValid(address) ||
        ((data == NULL) && (length != 0U))) {
        return STATUS_INVALID_PARAM;
    }
    /* 当前用途为短寄存器命令；需要写长块数据时应扩展接口而非越界复制。 */
    if (length > 3U) {
        return STATUS_OVERFLOW;
    }

    for (uint8_t attempt = 0U; attempt <= I2C_RECOVERY_RETRY_COUNT;
         ++attempt) {
        status = I2C_WriteRegisterOnce(instance, address, reg, data, length);
        if (status == STATUS_OK) {
            return STATUS_OK;
        }
        I2C_RecoverController(instance);
    }
    return status;
}
