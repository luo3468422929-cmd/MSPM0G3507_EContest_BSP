#include "project.h"

int main(void)
{
    if (System_Init() != STATUS_OK) {
        while (1) { __WFI(); }
    }
    if (Task_Init() != STATUS_OK) {
        while (1) { __WFI(); }
    }
    while (1) {
        Task_Run();
        __WFI();
    }
}
