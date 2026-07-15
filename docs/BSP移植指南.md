# 驱动移植规范

把开源代码整理进本工程时按下面处理：

1. 删除开源代码里的 `main()`、全局延时、硬编码 GPIO 和重复的串口初始化。
2. DriverLib/SysConfig 直接相关的通道放 `Bsp/`；具有明确器件语义的驱动放 `Hardware/`；纯算法放 `Control/`。
3. 物理引脚只在 `empty.syscfg` 出现，驱动通过 `Bsp/board_pins.h` 使用逻辑别名。
4. 公共函数命名为 `Module_Init()`、`Module_Update()`、`Module_GetData()`；每个公共入口检查空指针、范围和初始化状态。
5. ISR 只搬运数据或计数；解析器和算法在 `Task_Run()` 调度中执行。
6. 在 `User/test.c` 增加一个独立测试入口，再接入 `User/task.c`。
7. 对纯算法补 `Tests/Host/test_main.c`；对引脚/API 补 `Tests/Build/*.ps1` 静态检查。

后续 12 路寻迹应实现新的总线适配器，但继续提供 `Track_Update()`、`Track_GetData()`，这样 `Control/car_control.c` 不需要跟着换协议。
