/**
 * @file main.c
 * @brief 提供固件唯一入口：初始化整板和任务层，然后永久运行协同调度。
 *
 * 所属层：User。比赛流程不要直接堆在 main()，应写入 task.c；中断只做
 * 收数/计数，复杂业务留给 Task_Run()。
 */
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
    /* 裸机主循环永不退出；Task_Run 本身不应包含无限等待。 */
    while (1) {
        /* 所有业务都在 Task_Run 的协同调度中；中断只负责快速收数。 */
        Task_Run();
        __WFI();
    }
}
