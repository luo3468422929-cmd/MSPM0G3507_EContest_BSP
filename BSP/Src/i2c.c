#include "i2c.h"

#include "bsp_config.h"

Status_t I2C_Init(void)
{
    return STATUS_OK;
}

Status_t I2C_Write(uint8_t address, const uint8_t *data, uint16_t length)
{
    uint16_t sent = 0U;
    uint32_t timeout = OLED_I2C_TIMEOUT_LOOPS;
    if ((data == NULL) || (length == 0U) || (length > 255U)) {
        return STATUS_INVALID_PARAM;
    }
    while (((DL_I2C_getControllerStatus(OLED_I2C_INST) &
             DL_I2C_CONTROLLER_STATUS_IDLE) == 0U) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) { return STATUS_TIMEOUT; }

    DL_I2C_flushControllerTXFIFO(OLED_I2C_INST);
    /* START 前至少预填一个字节，避免控制器启动后 TX FIFO 立即欠载。 */
    DL_I2C_fillControllerTXFIFO(OLED_I2C_INST, &data[0], 1U);
    sent = 1U;
    DL_I2C_startControllerTransfer(OLED_I2C_INST, address,
        DL_I2C_CONTROLLER_DIRECTION_TX, length);
    timeout = OLED_I2C_TIMEOUT_LOOPS;
    while (sent < length) {
        if (DL_I2C_getControllerTXFIFOCounter(OLED_I2C_INST) > 0U) {
            DL_I2C_fillControllerTXFIFO(OLED_I2C_INST, &data[sent], 1U);
            sent++;
            timeout = OLED_I2C_TIMEOUT_LOOPS;
        } else if (timeout-- == 0U) {
            return STATUS_TIMEOUT;
        }
    }
    timeout = OLED_I2C_TIMEOUT_LOOPS;
    while ((DL_I2C_getControllerStatus(OLED_I2C_INST) &
            DL_I2C_CONTROLLER_STATUS_BUSY_BUS) != 0U) {
        if (timeout-- == 0U) { return STATUS_TIMEOUT; }
    }
    if ((DL_I2C_getControllerStatus(OLED_I2C_INST) &
         DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
        return STATUS_ERROR;
    }
    return STATUS_OK;
}

Status_t I2C_WriteRegister(uint8_t address, uint8_t reg,
                           const uint8_t *data, uint16_t length)
{
    uint8_t packet[17];
    uint16_t offset = 0U;
    if ((data == NULL) && (length != 0U)) { return STATUS_INVALID_PARAM; }
    while (offset < length || (length == 0U && offset == 0U)) {
        uint16_t chunk = (uint16_t)(length - offset);
        Status_t status;
        if (chunk > 16U) { chunk = 16U; }
        packet[0] = reg;
        for (uint16_t index = 0U; index < chunk; ++index) {
            packet[index + 1U] = data[offset + index];
        }
        status = I2C_Write(address, packet, (uint16_t)(chunk + 1U));
        if (status != STATUS_OK) { return status; }
        if (length == 0U) { break; }
        offset = (uint16_t)(offset + chunk);
    }
    return STATUS_OK;
}
