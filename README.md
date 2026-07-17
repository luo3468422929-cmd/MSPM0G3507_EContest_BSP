# MSPM0G3507 电赛小车模板

这是地猛星 MSPM0G3507（LQFP-48）电赛模板，使用 TI DriverLib + SysConfig，已接入 TB6612、GPIO x2 编码器、亚博八路 I2C 循迹、ST7735S LCD、UART 惯导、按键、PID 和模块测试。

第一次使用先看 [项目总览与使用指南](docs/项目总览与使用指南.md)，上板时按 [模块逐项测试流程](docs/模块逐项测试流程.md) 操作。

## 比赛时主要改四处

- `empty.syscfg`：换引脚、外设实例和时钟；不要编辑生成的 `ti_msp_dl_config.c/.h`。
- `User/user_config.h`：选择 `STARTUP_TEST`，修改功能开关、方向、轮径、速度和 PID。
- `User/task.c`：写题目状态机和多模块协同流程。
- `User/test.c`：增加或修改单模块测试。

## 四层目录

```text
User/       比赛流程、统一配置和模块测试
Control/    PID、循迹控制和裸机调度
Hardware/   TB6612、编码器、循迹、惯导、LCD、按键、LED
Bsp/        DriverLib/SysConfig 相关的 UART、I2C、SPI、定时器和板级初始化
```

依赖方向是 `User -> Control/Hardware -> Bsp`。`User` 不直接调用 `DL_*`，中断中不打印、不刷屏、不跑 PID。

## 当前关键硬件

- 调试串口：UART0，PA10/PA11，开发板已连接 CH340，直接插 USB 数据线即可使用，115200 8N1。
- 八路循迹：I2C0，SDA=PA28、SCL=PA31，地址 `0x12`，状态寄存器 `0x30`。
- 临时按键：PB2 接 GND，内部上拉；正常运行或电机测试中按下即急停。
- LCD：SPI1，SCK=PB9、MOSI=PB8、CS/DC/RES=PA27/PA26/PA25；BL 直连 3.3 V，不占 PA24。
- 编码器：左 A/B=PA17/PA16，右 A/B=PB19/PB20，四路 GPIO 上升沿中断、x2 解码。
- PA0、PA1、PA2、PA24 当前均不分配；尤其 PA0/PA1 不作为现场首选 I2C。

完整表见 [引脚资源分配表](docs/引脚资源分配表.md)。

## 启动与测试

在 `User/user_config.h` 选择测试：

```c
#define STARTUP_TEST TEST_LCD   /* LCD 测试 */
#define STARTUP_TEST TEST_NONE  /* 正常小车 */
```

正常模式上电先显示 `ARMING`，等待 1 秒并连续收到 5 帧有效循迹数据后才使能电机。运行或 MOTOR/PID 测试中按下 PB2，程序立即拉低 TB6612 STBY 并进入 `STOPPED`；为防误启动，只能复位恢复。

## 一条命令验证

在工程根目录运行：

```powershell
.\Tests\Build\verify_build.ps1
```

该脚本会执行全部静态回归检查、真正运行 Windows Host 断言、用 ARM GCC 交叉检查、重新生成 SysConfig，并使用 CCS 配置的 TI Clang 4.0.4 完整编译和链接。
