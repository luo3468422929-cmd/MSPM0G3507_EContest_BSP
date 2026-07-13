#ifndef ADC_TRACK_H
#define ADC_TRACK_H

#include "common.h"

Status_t ADCTrack_Init(void);
Status_t ADCTrack_Read(uint16_t *samples, uint8_t count);

#endif

