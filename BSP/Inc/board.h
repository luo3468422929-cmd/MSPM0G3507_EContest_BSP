#ifndef BOARD_H
#define BOARD_H

#include "common.h"

Status_t Board_Init(void);
void Board_EmergencyStop(void);
Status_t Board_SelfTest(void);

#endif

