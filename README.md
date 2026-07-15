# MSPM0G3507 电赛 BSP 模板工程

本工程面向嘉立创地猛星 MSPM0G3507，使用 TI DriverLib、SysConfig、TI Clang 和 NoRTOS。核心目标是比赛现场只修改 `empty.syscfg`、`BSP/Config/bsp_config.h` 与应用参数，不进入驱动 `.c` 文件改逻辑。

## 已包含模块

- TB6612 双路直流电机：PWM、正反转、滑行、刹车、STBY、限幅、斜坡。
- 双编码器：方向计数、RPM、线速度和一阶低通。
- PID：位置式、增量式、积分分离、限幅和抗饱和。
- 八路寻迹：数字/ADC 模式、滑动均值、标定、归一化、加权位置和状态识别。
- SSD1306 OLED：硬件 I2C 默认，SPI 可选，字符串/整数/浮点数显示。
- UART：双实例、中断接收、环形缓冲、超时发送和溢出统计。
- 通用命令帧：不定长载荷、校验、重同步和命令回调。
- 串口惯导：解析 `5A AA` 角速度帧、`5A BB` Yaw 帧，支持非阻塞 Yaw 归零。
- LED、按键消抖、1 ms Tick、协作式调度。
- 寻迹方向环 + 左右轮速度环综合 Demo。

## 入口

`empty.c` 只负责启动 `App_Init()` 和重复调用 `App_Run()`。综合逻辑位于 `App/Src/app_main.c`，应用层不直接操作 DriverLib。

## 首次使用

1. 用 CCS 导入本目录。
2. 打开 `empty.syscfg`，确认本机仍使用 MSPM0 SDK 2.10.0.04 和 SysConfig 1.26.2。
3. 对照 `docs/引脚资源分配表.md` 接线。
4. 先断开电机电源构建工程，排除 SysConfig 和编译问题。
5. 按 `docs/电赛快速上手指南.md` 逐模块上板测试。

## 关键规则

- 公共函数使用 `模块名_动作()`，例如 `Motor_SetDuty()`，不使用统一 `BSP_` 前缀。
- 不编辑 `ti_msp_dl_config.c/.h`。
- 电机方向、编码器方向、传感器极性和参数只改 `bsp_config.h`。
- 引脚复用和外设实例先改 `.syscfg`，再同步配置宏。
- 默认寻迹配置为八路数字输入；模拟灰度阵列按 `docs/SysConfig配置指南.md` 启用 ADC0 MEM0～MEM7。

## 文档索引

- `docs/CCS工程导入指南.md`
- `docs/SysConfig配置指南.md`
- `docs/模块接口手册.md`
- `docs/BSP移植指南.md`
- `docs/引脚资源分配表.md`
- `docs/电赛快速上手指南.md`
