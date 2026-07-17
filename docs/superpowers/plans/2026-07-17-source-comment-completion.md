# 正式源码中文注释补全 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 为四层目录的全部 60 个正式 `.c/.h` 文件补齐适合电赛新手阅读的中文注释，并增加一份代码阅读指南。

**Architecture:** 不调整目录、接口或控制逻辑，只在现有文件中增加 Doxygen 风格文件说明、公共接口说明和关键流程注释。按 `Bsp → Hardware → Control → User` 顺序处理，每层完成后检查注释覆盖和源码差异，最后执行完整 Host、SysConfig 与 TI Clang 验证。

**Tech Stack:** C11、TI DriverLib、MSPM0 SDK 2.10.0.04、SysConfig 1.26.2、TI Arm Clang 4.0.4、PowerShell、Markdown。

## Global Constraints

- 只修改注释和文档，不改变接口、宏取值、引脚、时序、状态机和硬件行为。
- 每个正式 `.c/.h` 文件必须以包含 `@file`、`@brief` 的中文说明开头。
- `.h` 重点说明公共类型、单位、参数、返回值和调用注意事项。
- `.c` 重点说明静态状态、硬件顺序、中断、数据流和安全错误路径。
- 不给普通赋值、括号或一眼可懂的判断增加逐行复述。
- 不引入待办/待定占位标记，注释不得与 `empty.syscfg`、`User/user_config.h` 不一致。
- 最终执行 `Tests/Build/verify_build.ps1` 并确认 Git 中没有未提交的功能改动。

---

## 文件映射

统一文件头格式：

```c
/**
 * @file module.c
 * @brief 用一句中文说明本文件的职责。
 *
 * 所属层：说明分层和调用边界。
 * 主要依赖：列出实际依赖的下层接口。
 * 比赛修改：说明现场通常改哪里，以及哪些硬件约束不能随意改。
 */
```

公共接口格式：

```c
/**
 * @brief 说明调用结果，而不是复述函数名。
 * @param argument 说明含义、单位和合法范围。
 * @return STATUS_OK 表示成功；其余状态说明失败原因。
 * @note 写明调用周期、初始化前提或 ISR 限制。
 */
```

### Task 1: 建立注释覆盖基线

**Files:**
- Read: `Bsp/*.c`, `Bsp/*.h`
- Read: `Hardware/*.c`, `Hardware/*.h`
- Read: `Control/*.c`, `Control/*.h`
- Read: `User/*.c`, `User/*.h`

**Interfaces:**
- Consumes: 当前 HEAD `b28e0dd` 的 60 个正式源码文件。
- Produces: 文件清单、零注释文件清单和实施前差异基线。

- [ ] **Step 1: 确认工作区没有遗留改动**

Run:

```powershell
git status --short
git rev-parse --short HEAD
```

Expected: 状态为空，HEAD 为 `b28e0dd` 或其直接后续计划提交。

- [ ] **Step 2: 统计正式源码文件**

Run:

```powershell
$files = Get-ChildItem -LiteralPath Bsp,Hardware,Control,User -File |
    Where-Object { $_.Extension -in '.c','.h' }
"SOURCE FILES: $($files.Count)"
```

Expected: `SOURCE FILES: 60`。

- [ ] **Step 3: 保存实施前验证证据**

Run:

```powershell
.\Tests\Build\verify_build.ps1
```

Expected: Host 输出 `ALL HOST TESTS PASSED`，结尾输出 `BUILD VERIFIED`。

### Task 2: 补齐 Bsp 层注释

**Files:**
- Modify: `Bsp/board_pins.h`, `Bsp/board.c`, `Bsp/board.h`, `Bsp/common.h`
- Modify: `Bsp/command.c`, `Bsp/command.h`, `Bsp/frame_protocol.c`, `Bsp/frame_protocol.h`
- Modify: `Bsp/i2c.c`, `Bsp/i2c.h`, `Bsp/ring_buffer.c`, `Bsp/ring_buffer.h`
- Modify: `Bsp/spi.c`, `Bsp/spi.h`, `Bsp/timer.c`, `Bsp/timer.h`, `Bsp/uart.c`, `Bsp/uart.h`

**Interfaces:**
- Consumes: SysConfig 生成宏、DriverLib、`Status_t`。
- Produces: 面向上层的板级初始化、时基、UART/I2C/SPI、协议和缓冲接口说明。

- [ ] **Step 1: 为 18 个文件增加统一文件头**

每个文件按“文件映射”的统一格式写明：Bsp 层只封装 MCU 通道，不解释具体传感器业务；物理引脚只来自 `empty.syscfg` 和 `board_pins.h`。

- [ ] **Step 2: 注释板级与基础类型**

在 `board.h/.c` 说明 `Board_Init()` 的安全初始化顺序和 `Board_EmergencyStop()` 的 STBY 低电平出口；在 `board_pins.h` 说明它只映射 SysConfig 逻辑名；在 `common.h` 说明状态码和限幅辅助函数。

- [ ] **Step 3: 注释通信与协议**

在 UART、I2C、SPI、环形缓冲、AA55 帧和命令分发文件中说明：

```c
/* UART ISR 只搬运字节；协议解析、打印和业务处理留在主循环。 */
/* I2C 所有轮询均有有限超时，失败后复位传输状态并最多重试一次。 */
/* RingBuffer 为单生产者/单消费者模型：ISR 写 head，主循环写 tail。 */
```

公共函数注释必须写明地址是 7 位 I2C 地址、长度为 0 的处理、缓冲区所有权及 ISR 调用限制。

- [ ] **Step 4: 注释时基与中断入口**

在 `timer.h/.c` 说明 1 ms SysTick、uint32 回绕安全减法及延时依赖中断；在 `uart.c` 的两个 IRQ 前说明对应硬件实例和 RX FIFO 排空策略。

- [ ] **Step 5: 检查 Bsp 只增加注释**

Run:

```powershell
git diff --check -- Bsp
git diff --word-diff=porcelain -- Bsp
```

Expected: 无空白错误；新增内容仅为注释，原有 C 语句没有改变。

### Task 3: 补齐 Hardware 传感器与执行器注释

**Files:**
- Modify: `Hardware/motor.c`, `Hardware/motor.h`, `Hardware/encoder.c`, `Hardware/encoder.h`
- Modify: `Hardware/encoder_decode.h`, `Hardware/encoder_speed_window.c`, `Hardware/encoder_speed_window.h`, `Hardware/encoder_verification.h`
- Modify: `Hardware/track.c`, `Hardware/track.h`, `Hardware/track_math.c`, `Hardware/track_math.h`
- Modify: `Hardware/imu.c`, `Hardware/imu.h`, `Hardware/imu_protocol.c`, `Hardware/imu_protocol.h`
- Modify: `Hardware/key.c`, `Hardware/key.h`, `Hardware/led.c`, `Hardware/led.h`
- Modify: `Hardware/filter_average.c`, `Hardware/filter_average.h`

**Interfaces:**
- Consumes: Bsp GPIO、PWM、UART、I2C 和 1 ms 时基接口。
- Produces: 电机、编码器、循迹、惯导、按键、LED 和滤波公共接口说明。

- [ ] **Step 1: 为 22 个文件增加统一文件头**

文件头写明具体器件/算法、实际接线假设、由谁调用以及比赛换器件时保留哪个公共接口。

- [ ] **Step 2: 注释 TB6612 和编码器完整数据流**

说明 STBY、正反转、短刹车、最小启动占空比和缓升；说明四路 GPIO 上升沿 x2 解码、方向反转宏、50 ms 速度窗、低通滤波、计数/圈和 RPM 单位。IRQ 注释明确四路使用独立 `if`，中断中只更新计数。

- [ ] **Step 3: 注释循迹和惯导协议**

说明八路 I2C 地址 `0x12`、状态寄存器 `0x30`、bit0=X1、bit7=X8、黑白极性、全黑策略和通信失败停车语义；说明 NCU 当前只输出 yaw/gyroZ、UART 环形缓冲取数、在线超时和协议重同步。

- [ ] **Step 4: 注释按键、LED 与通用算法**

说明按键低有效、消抖事件、上电前已按住时 `Key_IsPressed()` 的意义；说明 LED 仅作状态提示；说明滑动平均、循迹加权位置、编码器速度窗和多圈校验工具的输入输出及单位。

- [ ] **Step 5: 检查本批只增加注释**

Run:

```powershell
git diff --check -- Hardware
git diff --word-diff=porcelain -- Hardware
```

Expected: 无空白错误，原有可执行语句不变。

### Task 4: 补齐 Hardware LCD 子模块注释

**Files:**
- Modify: `Hardware/lcd.c`, `Hardware/lcd.h`
- Modify: `Hardware/lcd_font.c`, `Hardware/lcd_font.h`
- Modify: `Hardware/lcd_bitmap.c`, `Hardware/lcd_bitmap.h`

**Interfaces:**
- Consumes: SPI1 与 CS/DC/RES GPIO；BL 固定接 3.3 V。
- Produces: ST7735S 初始化、颜色、字符、数字、位图接口说明。

- [ ] **Step 1: 为 6 个文件增加统一文件头**

说明当前为 ST7735S 直写方案、没有全屏帧缓冲、BL 不受 PA24 控制、字库与位图分别放在哪个文件。

- [ ] **Step 2: 注释 LCD 命令与绘制边界**

说明命令/数据切换、地址窗口、RGB565、坐标范围、字符串裁剪、透明/背景色行为和 SPI 阻塞影响；初始化表按“复位、退出睡眠、像素格式、方向、开显示”分段解释。

- [ ] **Step 3: 注释字库与位图存储格式**

说明 5×7 ASCII 索引范围、未知字符替代行为、位图宽高和 RGB565 数组排列，不逐字节注释字体常量。

- [ ] **Step 4: 检查 LCD 只增加注释**

Run:

```powershell
git diff --check -- Hardware/lcd* Hardware/lcd_font* Hardware/lcd_bitmap*
```

Expected: 无空白错误，绘图和初始化语句不变。

### Task 5: 补齐 Control 层注释

**Files:**
- Modify: `Control/pid.c`, `Control/pid.h`
- Modify: `Control/car_control.c`, `Control/car_control.h`
- Modify: `Control/scheduler.c`, `Control/scheduler.h`

**Interfaces:**
- Consumes: 寻迹结果、编码器 RPM、电机接口和实际毫秒时基。
- Produces: 位置式/增量式 PID、双轮闭环和无 RTOS 周期调度说明。

- [ ] **Step 1: 为 6 个文件增加统一文件头**

说明 Control 层不直接访问 DriverLib，不拥有物理引脚。

- [ ] **Step 2: 注释 PID 类型、参数和公式**

写明 `kp/ki/kd`、采样周期、积分分离、死区和输出/积分限幅；位置式和增量式分别说明状态量及调用周期要求，不展开与实现无关的控制理论。

- [ ] **Step 3: 注释 CarControl 数据流**

使用以下流程说明：

```text
编码器更新 → 读取循迹 → 转向 PID → 左右目标 RPM → 双速度 PID → TB6612
```

解释为何编码器更新位于可能阻塞的 I2C 前、丢线为何清 PID 并停车、速度测试为何不覆盖正式基础速度。

- [ ] **Step 4: 注释调度器错过周期策略**

说明 `Scheduler_IsDue()` 使用无符号差值处理 tick 回绕，并跳过错过的周期而不是连续补跑 PID。

- [ ] **Step 5: 检查 Control 只增加注释**

Run:

```powershell
git diff --check -- Control
git diff --word-diff=porcelain -- Control
```

Expected: 无空白错误，PID 参数和公式表达式未变化。

### Task 6: 补齐 User 层注释

**Files:**
- Modify: `User/main.c`, `User/project.h`, `User/user_config.h`
- Modify: `User/task.c`, `User/task.h`, `User/task_safety.h`
- Modify: `User/test.c`, `User/test.h`

**Interfaces:**
- Consumes: Control 与 Hardware 的公共接口。
- Produces: 比赛配置入口、主循环、状态机和单模块测试使用说明。

- [ ] **Step 1: 为 8 个文件增加统一文件头**

明确新手日常只改 `user_config.h`、`task.c` 和必要的 `test.c`；物理引脚只改 `empty.syscfg`。

- [ ] **Step 2: 注释 main 与配置分组**

说明 `main` 只初始化并永久调用 `Task_Run()`；为功能开关、测试入口、方向极性、机械参数、循迹、PID、显示/通信参数写明单位、范围和修改顺序。

- [ ] **Step 3: 注释正常状态机**

在 `task.c` 说明 `WAIT_START/ARMING/RUNNING/STOPPED`，自动启动的 1 秒与连续 5 帧条件，上电前按住 PB2 的安全判断，5/10/20/100 ms 任务分工，以及测试优先于正常模式的原因。

- [ ] **Step 4: 注释测试框架**

说明 `STARTUP_TEST → Test_Select → Test_Run → 具体 Test_RunXxx` 调用链；逐个测试写明所需模块、输出、阶段时长、结束方式及电机测试必须架空。

- [ ] **Step 5: 检查 User 只增加注释**

Run:

```powershell
git diff --check -- User
git diff --word-diff=porcelain -- User
```

Expected: 无空白错误，`STARTUP_TEST`、PID 数值和状态转换条件未变化。

### Task 7: 新增代码阅读指南并验收覆盖率

**Files:**
- Create: `docs/代码阅读顺序与注释规范.md`
- Modify: `README.md`

**Interfaces:**
- Consumes: 已注释完成的四层源码。
- Produces: 新手阅读路径和后续模块注释模板。

- [ ] **Step 1: 编写阅读指南**

文档必须包含：

```text
推荐顺序：User/main.c → User/task.c → Control/car_control.c
          → Hardware 具体器件 → Bsp MCU 通道
```

并解释四层修改边界、接口追踪示例、换器件步骤、注释模板和硬件约束注释的复测要求。

- [ ] **Step 2: 在 README 增加阅读指南入口**

在首次使用段落加入 `docs/代码阅读顺序与注释规范.md` 链接，不改变现有四个比赛入口说明。

- [ ] **Step 3: 检查 60 个文件均有文件头**

Run:

```powershell
$files = Get-ChildItem -LiteralPath Bsp,Hardware,Control,User -File |
    Where-Object { $_.Extension -in '.c','.h' }
$missing = foreach ($file in $files) {
    $text = Get-Content -LiteralPath $file.FullName -Raw
    if (($text -notmatch '@file\s+') -or ($text -notmatch '@brief\s+')) {
        $file.FullName
    }
}
if ($files.Count -ne 60) { throw "Expected 60 files, got $($files.Count)" }
if ($missing) { throw "Missing file comments:`n$($missing -join "`n")" }
'SOURCE COMMENT COVERAGE PASSED'
```

Expected: `SOURCE COMMENT COVERAGE PASSED`。

- [ ] **Step 4: 检查没有占位符**

Run:

```powershell
$patterns = @('TO' + 'DO', 'T' + 'BD')
rg -n ($patterns -join '|') Bsp Hardware Control User README.md docs/代码阅读顺序与注释规范.md
```

Expected: 无输出。

### Task 8: 完整验证、复审和提交

**Files:**
- Verify: all modified source comments and documentation

**Interfaces:**
- Consumes: Task 2-7 的全部注释和文档。
- Produces: 可回退的注释补全提交。

- [ ] **Step 1: 检查差异性质**

Run:

```powershell
git diff --check
git diff --stat
git diff --word-diff=porcelain -- Bsp Hardware Control User
```

Expected: C/H 中只有注释变化；README 和新阅读文档只有说明变化。

- [ ] **Step 2: 执行完整验证**

Run:

```powershell
.\Tests\Build\verify_build.ps1
```

Expected: 所有静态检查通过、`ALL HOST TESTS PASSED`、ARM GCC 检查通过、SysConfig 成功、TI Clang 输出 `BUILD VERIFIED`。

- [ ] **Step 3: 提交注释补全**

Run:

```powershell
git add -- Bsp Hardware Control User README.md docs/代码阅读顺序与注释规范.md
git diff --cached --check
git commit -m "docs: complete beginner-focused source comments"
```

Expected: 提交成功，且没有包含 `Debug/` 或其他构建产物。

- [ ] **Step 4: 确认最终状态**

Run:

```powershell
git status --short
git log -3 --oneline
```

Expected: 工作区为空，最新提交为注释补全提交。
