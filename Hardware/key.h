#ifndef KEY_H
#define KEY_H

#include "common.h"

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_PRESSED,
    KEY_EVENT_RELEASED,
    KEY_EVENT_CLICKED,
    KEY_EVENT_LONG_PRESSED
} Key_Event_t;

Status_t Key_Init(void);
void Key_Scan(uint32_t nowMs);
Key_Event_t Key_GetEvent(void);
bool Key_IsPressed(void);

#endif

