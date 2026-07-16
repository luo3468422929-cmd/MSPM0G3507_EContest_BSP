# Encoder Fixed-Window Speed Implementation Plan

> **For agentic workers:** This plan is executed inline in the current workspace with test-first checkpoints.

**Goal:** 将编码器速度估计改为可配置的 50 ms 固定时间窗计数，同时保留累计位置、AB 方向判断和现有低通滤波。

**Architecture:** 新增一个不依赖 DriverLib 的 `EncoderSpeedWindow` 纯算法模块，维护最近若干次编码器增量及其采样时间，形成 50 ms 滑动计数窗。`Hardware/encoder.c` 只负责读取硬件累计计数并调用该模块；10 ms 控制循环保持不变，测试模式 100 ms 调用也会自动使用实际 100 ms 窗口。

**Tech Stack:** MSPM0G3507、TI DriverLib、C11、CCS、ARM GCC 主机源码编译测试。

## Global Constraints

- 不修改 SysConfig 引脚分配和编码器 AB 中断硬件接法。
- 不删除累计计数和 B 相方向判断。
- 默认速度窗为 `0.05f` 秒；所有参数集中在 `User/user_config.h`。
- 速度未积累满窗口前不更新有效速度，避免启动瞬间的短窗放大。
- 保留现有 `ENCODER_SPEED_FILTER_ALPHA` 一阶滤波。
- 不将编码器算法模块直接依赖 DriverLib，保证主机端可独立测试。

---

### Task 1: Add failing host tests for fixed-window behavior

**Files:**
- Modify: `Tests/Host/test_main.c`
- Modify: `Tests/Host/run_tests.ps1`

**Interfaces:**
- Consumes: future `EncoderSpeedWindow_Init()` and `EncoderSpeedWindow_Push()`.
- Produces: tests proving 50 ms accumulation, sliding-window replacement, 100 ms fallback, and invalid-parameter handling.

- [ ] **Step 1: Add the test cases before production implementation**

Add `#include "encoder_speed_window.h"` and test:

```c
static void Test_EncoderSpeedWindow_Fixed50MsAndAdaptive100Ms(void)
{
    EncoderSpeedWindow_t window;
    float rpm = 0.0f;
    bool ready = false;
    const float countsPerWheelRev = 780.0f;

    CHECK_TRUE(EncoderSpeedWindow_Init(&window) == STATUS_OK);
    CHECK_TRUE(EncoderSpeedWindow_Push(&window, 1, 0.01f, 0.05f,
                                      countsPerWheelRev, &rpm, &ready) == STATUS_OK);
    CHECK_TRUE(!ready);
    for (uint8_t index = 0U; index < 4U; ++index) {
        CHECK_TRUE(EncoderSpeedWindow_Push(&window, 0, 0.01f, 0.05f,
                                          countsPerWheelRev, &rpm, &ready) == STATUS_OK);
    }
    CHECK_TRUE(ready);
    CHECK_NEAR(rpm, 60.0f / (countsPerWheelRev * 0.05f), 0.0001f);

    CHECK_TRUE(EncoderSpeedWindow_Push(&window, 0, 0.01f, 0.05f,
                                      countsPerWheelRev, &rpm, &ready) == STATUS_OK);
    CHECK_TRUE(ready);
    CHECK_NEAR(rpm, 60.0f / (countsPerWheelRev * 0.05f), 0.0001f);

    CHECK_TRUE(EncoderSpeedWindow_Init(&window) == STATUS_OK);
    CHECK_TRUE(EncoderSpeedWindow_Push(&window, 4, 0.1f, 0.05f,
                                      countsPerWheelRev, &rpm, &ready) == STATUS_OK);
    CHECK_TRUE(ready);
    CHECK_NEAR(rpm, 4.0f * 60.0f / (countsPerWheelRev * 0.1f), 0.0001f);
}

static void Test_EncoderSpeedWindow_InvalidParameters(void)
{
    EncoderSpeedWindow_t window;
    float rpm = 0.0f;
    bool ready = false;
    CHECK_TRUE(EncoderSpeedWindow_Init(NULL) == STATUS_INVALID_PARAM);
    CHECK_TRUE(EncoderSpeedWindow_Init(&window) == STATUS_OK);
    CHECK_TRUE(EncoderSpeedWindow_Push(NULL, 0, 0.01f, 0.05f,
                                      780.0f, &rpm, &ready) == STATUS_INVALID_PARAM);
    CHECK_TRUE(EncoderSpeedWindow_Push(&window, 0, 0.0f, 0.05f,
                                      780.0f, &rpm, &ready) == STATUS_INVALID_PARAM);
}
```

Call both tests from `main()` and add `Hardware/encoder_speed_window.c` to the host compile source list.

- [ ] **Step 2: Run the host tests and confirm the expected RED failure**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests\Host\run_tests.ps1
```

Expected: compilation fails because `encoder_speed_window.h` and its implementation do not exist yet.

### Task 2: Implement the pure fixed-window algorithm

**Files:**
- Create: `Hardware/encoder_speed_window.h`
- Create: `Hardware/encoder_speed_window.c`

**Interfaces:**
- `EncoderSpeedWindow_Init(EncoderSpeedWindow_t *window)`.
- `EncoderSpeedWindow_Push(EncoderSpeedWindow_t *window, int32_t deltaCount, float sampleTimeS, float minimumWindowS, float countsPerWheelRev, float *rpm, bool *ready)`.

- [ ] **Step 1: Implement parameter validation and ring-window insertion**

Keep the newest samples in a bounded 16-entry ring. Drop the oldest entry if full, maintain signed count sum and elapsed-time sum, and remove old entries while the remaining window is still at least the effective window duration.

- [ ] **Step 2: Implement RPM output**

Use `rpm = signedCountSum * 60 / (countsPerWheelRev * elapsedTimeSum)` only when the accumulated time reaches `max(sampleTimeS, minimumWindowS)`. Set `ready` false before the window is full and true after producing a result.

- [ ] **Step 3: Run the host tests and confirm GREEN**

Run the same `Tests\Host\run_tests.ps1` command and require a clean compile with no warnings under `-Wall -Wextra -Werror`.

### Task 3: Integrate the window module into the hardware encoder

**Files:**
- Modify: `User/user_config.h`
- Modify: `Hardware/encoder.c`
- Modify: `Hardware/encoder.h` only if a declaration needs documentation.

**Interfaces:**
- Existing public APIs remain unchanged.
- `Encoder_UpdateSpeed(sampleTimeS)` continues to receive the actual elapsed sample interval.

- [ ] **Step 1: Add `ENCODER_SPEED_WINDOW_S 0.05f` to user configuration**

- [ ] **Step 2: Add one window state per wheel and reset it in `Encoder_Reset()`**

- [ ] **Step 3: Replace direct delta-to-RPM calculation with `EncoderSpeedWindow_Push()`**

Keep cumulative `totalCount`, signed `deltaCount`, first-order filtering, and linear-speed conversion. If the window is not ready, keep the previous speed value.

- [ ] **Step 4: Run host tests and the complete static/CCS build verification**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tests\Host\run_tests.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Tests\Build\verify_build.ps1
```

The host test must pass. If the full verification still stops on the existing Chinese-path PowerShell encoding issue, report that exact failure separately and run the individual encoder-related checks plus direct TI compilation.

### Task 4: Commit the implementation and report rollback point

**Files:**
- All files from Tasks 1–3.

- [ ] **Step 1: Review the diff and confirm only encoder speed files/config/tests changed**

- [ ] **Step 2: Commit with message**

```text
feat: use fixed-window encoder speed estimation
```

- [ ] **Step 3: Report both commits**

Report baseline rollback commit `5ab3fac` and the new implementation commit separately.

