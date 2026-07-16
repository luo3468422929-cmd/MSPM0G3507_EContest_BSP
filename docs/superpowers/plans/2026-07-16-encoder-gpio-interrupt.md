# Encoder GPIO Interrupt Migration Plan

> **For agentic workers:** This plan is executed inline after the approved design in `docs/superpowers/specs/2026-07-16-encoder-gpio-interrupt-design.md`.

**Goal:** 将编码器 A 相边沿来源从定时器捕获中断切换为 GPIO 上升沿中断，保持现有引脚和固定时间窗测速行为。

**Architecture:** SysConfig 删除两个 CAPTURE 实例，把 PA17/PB19 配成 GPIO 上升沿中断；`board_pins.h` 只暴露 GPIO 逻辑宏；`encoder.c` 由 `GROUP1_IRQHandler()` 分发左右 A 相事件，继续读取 B 相判方向。纯软件测速窗口模块不变。

**Tech Stack:** MSPM0G3507、TI DriverLib、SysConfig、CCS、PowerShell 静态检查、ARM GCC 源码编译。

## Tasks

### Task 1: Lock the GPIO interrupt contract with tests

Files: `Tests/Build/test_target_pinout.ps1`, `Tests/Build/test_encoder_gpio_irq.ps1`, `Tests/Build/verify_build.ps1`.

- Require PA17/PB19 to be named GPIO inputs with `interruptEn=true` and `polarity="RISE"`.
- Require no CAPTURE instance or TIMA1/TIMG8 encoder handler.
- Require `GROUP1_IRQHandler`, GPIO enabled-status reads, flag clears, and left/right edge dispatch.
- Run the tests before implementation to observe the expected failure, then rerun after implementation.

### Task 2: Move SysConfig and board mapping to GPIO

Files: `empty.syscfg`, `Bsp/board_pins.h`.

- Remove CAPTURE imports and both capture configurations.
- Add `ENCODER_LEFT_A` to GPIOA on PA17 and `ENCODER_RIGHT_A` to GPIOB on PB19.
- Leave B phases on PA16/PB20 and all physical pin numbers unchanged.
- Map A phases and GPIOA/GPIOB IRQ numbers through `board_pins.h`.

### Task 3: Replace timer handlers with one GPIO group handler

File: `Hardware/encoder.c`.

- Enable GPIOA/GPIOB interrupt vectors in `Encoder_Init()`.
- In `GROUP1_IRQHandler()`, inspect enabled status for the left and right A pins, call the existing edge handlers, and clear each processed flag.
- Delete `TIMA1_IRQHandler()` and `TIMG8_IRQHandler()`.
- Preserve AB direction decoding, cumulative counts, and 50 ms speed-window calculations.

### Task 4: Verify and commit

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File Tests\Host\run_tests.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File Tests\Build\verify_build.ps1
```

Then commit the implementation as:

```text
feat: use GPIO interrupts for encoder edges
```

