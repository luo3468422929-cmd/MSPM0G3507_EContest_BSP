#include "project.h"

int main(void)
{
    /* 板级初始化失败时不要继续启动电机或依赖尚未工作的延时。 */
    if (System_Init() != STATUS_OK) {
        while (1) { __WFI(); }
    }
    if (Task_Init() != STATUS_OK) {
        while (1) { __WFI(); }
    }
    while (1) {
        /* 所有业务都在 Task_Run 的协同调度中；中断只负责快速收数。 */
        Task_Run();
        __WFI();
    }
}
