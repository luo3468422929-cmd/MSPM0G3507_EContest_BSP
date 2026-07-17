#include "i2c.h"

#include <string.h>

#include "board_pins.h"
#include "ti/driverlib/dl_i2c.h"
#include "user_config.h"

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

static Status_t I2C_ReadRegisterOnce(I2C_Regs *instance, uint8_t address,
                                     uint8_t reg, uint8_t *data,
                                     uint8_t length)
{
    Status_t status;

    status = I2C_WaitIdle(instance);
    if (status != STATUS_OK) {
        return status;
    }

    DL_I2C_flushControllerTXFIFO(instance);
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
