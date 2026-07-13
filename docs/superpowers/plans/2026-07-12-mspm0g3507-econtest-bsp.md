# MSPM0G3507 EContest BSP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 构建可直接复制的 MSPM0G3507 CCS + SysConfig 电赛 BSP 母工程。

**Architecture:** 使用轻量分层：硬件相关代码位于 BSP，纯算法位于 Components，闭环组合位于 Services，题目逻辑位于 App。模块通过明确结构体和函数接口通信，ISR 仅更新缓冲、计数和调度标志。

**Tech Stack:** C11/C99、TI Clang 4.0.4 LTS、MSPM0 SDK DriverLib 2.10.0.04、SysConfig 1.26.2、NoRTOS。

## Global Constraints

- 公共函数使用 `模块名_动作()`，禁止统一 `BSP_` 前缀。
- 不修改 SysConfig 生成的 `ti_msp_dl_config.c/.h`。
- 不使用动态内存、阻塞式 ISR 或跨模块全局变量耦合。
- 所有公开配置集中在 `BSP/Config/bsp_config.h`。
- 算法模块必须先通过宿主机测试。

---

### Task 1: 公共类型和纯算法组件

**Files:** `BSP/Inc/common.h`、`Components/PID/*`、`Components/Filter/*`、`Components/Protocol/*`、`Components/RingBuffer/*`、`Tests/Host/*`

- [x] 先写 PID、滤波、环形缓冲、惯导帧和寻迹位置测试。
- [x] 运行测试，确认因实现缺失失败。
- [x] 实现最小算法代码并完成严格交叉编译检查。

### Task 2: SysConfig 适配和基础 BSP

**Files:** `empty.syscfg`、`BSP/Config/*`、`BSP/Inc/board.h`、`led.h`、`key.h`、`timer.h`、`uart.h` 及对应源文件。

- [x] 定义模块开关、SysConfig 名称映射和静态检查。
- [x] 实现 Board、Tick、LED、按键和双 UART 环形接收。
- [x] 提供中断入口和基础自测。

### Task 3: 电机、编码器与速度闭环

**Files:** `motor.*`、`encoder.*`、`Services/MotorControl/*`

- [x] 实现 TB6612 正反转、滑行、刹车、STBY、限幅和斜坡。
- [x] 实现双编码器计数、方向、RPM 和线速度。
- [x] 组合双速度 PID，停车时清除控制状态。

### Task 4: 八路寻迹与寻迹服务

**Files:** `track.*`、`Services/LineFollow/*`

- [x] 实现模拟/数字编译期模式、滤波、阈值、归一化和校准。
- [x] 实现八路加权位置、全白/全黑和丢线策略。
- [x] 用方向 PID 生成左右轮速度目标。

### Task 5: OLED、UART 惯导和命令协议

**Files:** `i2c.*`、`spi.*`、`oled.*`、`imu_uart.*`、`Components/SSD1306/*`

- [x] 实现 I2C/SPI 发送适配和 SSD1306 显存接口。
- [x] 实现字符串、整数、浮点数和状态显示。
- [x] 实现惯导解析、在线判断、统计和非阻塞归零命令状态机。

### Task 6: 调度、示例和文档

**Files:** `Services/Scheduler/*`、`App/*`、`Examples/*`、`Docs/*`、`empty.c`

- [x] 实现 1/5/10/20/100 ms 协作任务标志。
- [x] 完成寻迹采集 + 双环 PID + OLED 综合 Demo。
- [x] 编写 CCS、SysConfig、模块移植、引脚表和比赛快速上手文档。

### Task 7: 验证

- [ ] 运行宿主机测试并检查 0 失败（当前环境无本机 C 执行器，仅完成测试源码交叉编译）。
- [x] 检查接口命名中无 `BSP_` 公共函数。
- [x] 检查应用层无 DriverLib 调用。
- [x] 完成 SysConfig + TI Clang + DriverLib 全量链接验证。
- [x] 对照设计逐项检查交付范围和已知硬件待板测项。
