#ifndef ENCODER_H
#define ENCODER_H

#include "common.h"
#include "encoder_decode.h"

typedef enum {
    ENCODER_LEFT = 0,
    ENCODER_RIGHT,
    ENCODER_COUNT
} Encoder_Id_t;

typedef struct {
    int32_t totalCount;
    int32_t deltaCount;
    float rpm;
    float linearSpeedMps;
} Encoder_Data_t;

Status_t Encoder_Init(void);
void Encoder_OnEdge(Encoder_Id_t id, Encoder_Edge_t edge,
                    bool phaseAHigh, bool phaseBHigh);
void Encoder_UpdateSpeed(float sampleTimeS);
Status_t Encoder_GetData(Encoder_Id_t id, Encoder_Data_t *data);
void Encoder_Reset(Encoder_Id_t id);

#endif

