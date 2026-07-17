# BSP Template Hardening Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use executing-plans to implement this plan task-by-task with review checkpoints.

**Goal:** 在保留当前上板验证结果的同时，完成安全、并发、调度、通信、配置、测试和文档的收尾加固。

**Architecture:** 保持 `Bsp / Hardware / Control / User` 四层裸机架构。所有比赛现场会调整的选择和参数集中在 `User/user_config.h`，统一状态机负责启动、测试和急停，底层模块继续通过小而稳定的接口工作。

**Tech Stack:** MSPM0G3507、TI DriverLib/SysConfig、TI Arm Clang、C11、PowerShell、Windows MSVC Host tests、Git。

## Global Constraints

- 修改前基线为 `dfd711d`。
- 所有生产修复先增加会失败的自动检查或 Host 断言，再改实现。
- 不引入 RTOS、DMA 或未经上板验证的大规模架构替换。
- 不占用 PA24、PA0、PA1；八路循迹保持 PA28/PA31。
- 不改变已经验证的编码器 x2 解码公式和 TB6612 正反转基本逻辑。

---

### Task 1: 固化设计与测试基线

**Files:**
- Create: `docs/superpowers/specs/2026-07-17-bsp-template-hardening-design.md`
- Create: `docs/superpowers/plans/2026-07-17-bsp-template-hardening.md`

**Steps:**

1. 记录三种候选方案和已选平衡方案。
2. 明确安全、引脚、启动、控制和验证标准。
3. 检查文档不存在未完成占位内容或相互冲突的决定。
4. 提交设计和计划。

### Task 2: 统一启动、测试和急停状态机

**Files:**
- Modify: `User/user_config.h`
- Modify: `User/task.c`
- Modify: `User/test.h`
- Modify: `User/test.c`
- Modify: `Control/car_control.c`
- Test: `Tests/Build/test_safety_state.ps1`

**Steps:**

1. 写静态回归检查，要求测试入口集中、按键先于测试分发、电机测试具备快速急停和退出锁定。
2. 运行检查并确认它因旧实现失败。
3. 增加 `STARTUP_TEST`、自动启动延时和连续有效帧配置。
4. 增加 `TASK_ARMING`，只有循迹就绪后才使能电机。
5. 让 `Task` 每轮只扫描一次按键，并把事件传给 `Test_Run()`。
6. 给电机/PID 测试应用统一的按下急停；停止状态只能复位退出。
7. 取消速度测试对生产基础速度参数的污染。
8. 运行回归检查和 TI 编译。

### Task 3: 修复通信并发、调度补跑和 I2C 恢复

**Files:**
- Modify: `Bsp/ring_buffer.h`
- Modify: `Bsp/ring_buffer.c`
- Modify: `Bsp/uart.c`
- Modify: `Bsp/i2c.c`
- Modify: `Control/scheduler.c`
- Modify: `Tests/Host/test_main.c`
- Test: `Tests/Build/test_runtime_hardening.ps1`

**Steps:**

1. 增加调度器错过周期不补跑的 Host 断言，并先确认旧实现失败。
2. 增加静态检查，要求环形缓冲 SPSC 所有权、UART FIFO 排空和 I2C 恢复。
3. 用单调 `head/tail` 重写环形缓冲索引。
4. UART TX 等待 FIFO 空位，RX ISR 循环读取 FIFO。
5. I2C 错误后复位传输状态并重试一次。
6. 调度器把本次执行基准更新为实际 `nowMs`，跳过错过周期。
7. 执行 Host 与 TI 编译测试。

### Task 4: 清理引脚并配置化控制策略

**Files:**
- Modify: `empty.syscfg`
- Modify: `Bsp/board_pins.h`
- Modify: `Hardware/lcd.c`
- Modify: `Hardware/motor.c`
- Modify: `Hardware/track.c`
- Modify: `Hardware/track.h`
- Modify: `Control/car_control.c`
- Modify: `User/user_config.h`
- Test: `Tests/Build/test_pin_and_control_config.ps1`

**Steps:**

1. 增加回归检查，禁止 SysConfig 中 PA24、PA0、PA1，要求控制参数来自统一配置。
2. 运行检查并确认旧实现失败。
3. 删除 LCD BL 和无用途 PA0/PA1 GPIO 项，保持 PA28/PA31 I2C 不变。
4. 把 LCD 背光接口改为兼容空操作并解释硬件原因。
5. 集中速度环、转向环、转速上限和最低占空比参数。
6. 增加循迹左右翻转、全黑策略和普通循迹禁止反转配置。
7. 重新生成 SysConfig 并执行完整编译。

### Task 5: 降低显示阻塞并落实功能开关

**Files:**
- Modify: `User/task.c`
- Modify: `User/test.c`
- Modify: `User/user_config.h`
- Modify: `Bsp/board.c`
- Modify: `Hardware/imu.h`
- Modify: `Hardware/imu.c`
- Test: `Tests/Build/test_feature_flags.ps1`

**Steps:**

1. 增加检查，要求可选模块的任务调用受 `CONFIG_*` 控制，并校验依赖关系。
2. 正常状态页每次只刷新一行，五行轮换更新。
3. PID 测试保留 VOFA 100 ms 数据，降低人类可读串口和 LCD 刷新负担。
4. 增加不消费新数据标志的 IMU 查看接口，保留原消费接口兼容性。
5. 检查 SysTick 初始化返回值并保证失败路径不依赖已经失败的 tick。
6. 运行回归和完整编译。

### Task 6: 让 Host 测试真正执行

**Files:**
- Modify: `Tests/Host/run_tests.ps1`
- Modify: `Tests/Host/test_main.c`
- Modify: `Tests/Build/verify_build.ps1`
- Modify: `Tests/Build/verify_ccs_build.ps1`

**Steps:**

1. 先证明旧脚本只生成对象文件、没有运行断言。
2. 发现 Visual Studio Build Tools 环境，编译、链接并运行 Host 测试程序。
3. 保留 ARM GCC 严格编译作为交叉检查。
4. 把未接入总验证入口的静态检查全部接入。
5. 优先使用 CCS 工程对应的 TI Clang 4.0.4，保留 SDK 编译器回退路径。
6. 验证输出包含 `ALL HOST TESTS PASSED`。

### Task 7: 整理注释和队友文档

**Files:**
- Create: `docs/项目总览与使用指南.md`
- Modify: `README.md`
- Modify: `docs/电赛快速上手指南.md`
- Modify: `docs/模块接口手册.md`
- Modify: `docs/SysConfig配置指南.md`
- Modify: `.clangd`
- Modify: 关键源文件中文注释

**Steps:**

1. 用一份总览解释目录依赖、上电流程、测试流程和比赛现场改法。
2. 给出修改引脚、选择测试、改 PID/速度、增加模块的准确入口。
3. 同步 LCD 背光、PA0/PA1、GPIO x2 编码器、八路 I2C 循迹和自动就绪门槛。
4. 只在状态机、安全、并发所有权和公式等关键处补中文注释，避免逐行废话。
5. 取消 `.clangd` 全局隐藏诊断。
6. 检查当前文档与代码宏、文件路径、默认值一致。

### Task 8: 全量验证、复审与提交

**Files:**
- Verify: entire repository

**Steps:**

1. 执行 Host 可执行测试。
2. 执行 `Tests/Build/verify_build.ps1`。
3. 强制执行 SysConfig 和 TI Clang 4.0.4 完整编译链接。
4. 执行 `git diff --check` 并确认没有意外生成物。
5. 请求独立代码复审，修复全部 Critical/Important 反馈。
6. 再次运行全量验证。
7. 提交最终加固结果，记录提交号和仍需上板确认的项目。
