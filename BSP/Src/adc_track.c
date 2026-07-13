#include "adc_track.h"

#include "bsp_config.h"

Status_t ADCTrack_Init(void)
{
#if TRACK_ANALOG_MODE
    DL_ADC12_enableConversions(TRACK_ADC_INST);
    return STATUS_OK;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

Status_t ADCTrack_Read(uint16_t *samples, uint8_t count)
{
#if TRACK_ANALOG_MODE
    static const DL_ADC12_MEM_IDX memory[8] = {
        DL_ADC12_MEM_IDX_0, DL_ADC12_MEM_IDX_1, DL_ADC12_MEM_IDX_2, DL_ADC12_MEM_IDX_3,
        DL_ADC12_MEM_IDX_4, DL_ADC12_MEM_IDX_5, DL_ADC12_MEM_IDX_6, DL_ADC12_MEM_IDX_7
    };
    uint32_t timeout = TRACK_ADC_TIMEOUT_LOOPS;
    if ((samples == NULL) || (count != 8U)) { return STATUS_INVALID_PARAM; }
    DL_ADC12_startConversion(TRACK_ADC_INST);
    while (((DL_ADC12_getRawInterruptStatus(TRACK_ADC_INST,
             DL_ADC12_INTERRUPT_MEM7_RESULT_LOADED)) == 0U) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) { return STATUS_TIMEOUT; }
    for (uint8_t index = 0U; index < count; ++index) {
        samples[index] = DL_ADC12_getMemResult(TRACK_ADC_INST, memory[index]);
    }
    DL_ADC12_clearInterruptStatus(TRACK_ADC_INST,
                                  DL_ADC12_INTERRUPT_MEM7_RESULT_LOADED);
    DL_ADC12_enableConversions(TRACK_ADC_INST);
    return STATUS_OK;
#else
    (void)samples;
    (void)count;
    return STATUS_NOT_SUPPORTED;
#endif
}

