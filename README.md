# MSPM0G3507 电赛小车模板

比赛时只改四处：`empty.syscfg`、`User/user_config.h`、`User/task.c`、`User/test.c`。

当前临时五路寻迹必须注意：**PA2 不接任何外设**；PCB 原 CH3 到 PA2 的连接要物理断开，再用杜邦线把 CH3 飞到 **PA15**；临时按键接 **PB2**，按下接 GND。PA2 属于地猛星板载 ROSC 相关电路，不能把 PA2 和 PA15 短接。

## 队员只需要认识四层

```text
User/       比赛任务、状态机、参数和单模块测试（平时主要改这里）
Control/    PID、循迹与双轮闭环控制
Hardware/   电机、编码器、寻迹、惯导、LCD、按键、LED
Bsp/        SysConfig/DriverLib 直接相关的板级、串口、SPI、定时器
```

主函数只有初始化和循环：

```c
System_Init();
Task_Init();
while (1) {
    Task_Run();
    __WFI();
}
```

`Task_Run()` 是无 RTOS 协同调度：5 ms 扫按键，10 ms 寻迹和电机控制，20 ms 处理惯导/指令，200 ms 更新 LCD 状态。不要在中断里跑 PID、刷屏或打印。

## 当前硬件

- MSPM0G3507 地猛星，TI DriverLib + SysConfig。
- TB6612 双路电机，PWM 为 PA12/PA13。
- 编码器 A 相使用 PA17/TIMA1_C0、PB19/TIMG8_C1 捕获，B 相 PA16/PB20。
- 串口惯导使用 UART2：PA21 TX、PA22 RX，115200 8N1。
- ST7735S 1.8 寸 128×160 LCD：SPI1 PB9/PB8，CS/DC/RES/BL 为 PA27/PA26/PA25/PA24。
- 临时五路数字寻迹：PA0、PA1、PA15、PB6、PB7；黑线极性由 `TRACK_BLACK_IS_HIGH` 切换。

完整引脚见 [docs/引脚资源分配表.md](docs/引脚资源分配表.md)。

## 第一次上板

1. 在 CCS 中导入工程，双击 `empty.syscfg` 确认无引脚冲突。
2. 先把车轮架空，保持 TB6612 电机电源断开。
3. 在 `User/task.c` 把 `Test_Select(TEST_NONE)` 临时改为 `TEST_LCD`，验证四色块。
4. 依次运行 `TEST_TRACK`、`TEST_KEY`、`TEST_MOTOR`、`TEST_ENCODER`、`TEST_IMU`。
5. 全部通过后改回 `TEST_NONE`，短按 PB2 启停，长按紧急停止。

详细顺序见 [docs/上板调试清单.md](docs/上板调试清单.md)。每个文件的用途见 [docs/工程文件说明.md](docs/工程文件说明.md)。

## 自动验证

在工程根目录运行：

```powershell
.\Tests\Host\run_tests.ps1
.\Tests\Build\verify_build.ps1
```

第二条会重新生成 SysConfig，并用 TI Clang 对全部正式源码进行 `-Wall -Wextra -Werror` 编译和完整链接。
