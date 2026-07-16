# 八路 I2C 灰度模块接入 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task with verification checkpoints.

**Goal:** 将借用的亚博八路 MCU 灰度模块接入 MSPM0G3507 工程的 I2C0，并在不改变控制层 `Track_*` 接口的情况下替换临时五路 GPIO 采集。

**Architecture:** SysConfig 负责 I2C0 PA28/PA31 的硬件实例；`Bsp/i2c.c` 提供有超时的通用寄存器读写；`Hardware/track.c` 只负责模块地址 `0x12`、寄存器 `0x30` 和 X1~X8 位映射。现有 `Control` 和任务调度继续调用 `Track_Update()`。

**Tech Stack:** MSPM0 SDK 2.10 Driverlib、CCS TI Arm Clang、SysConfig 1.26、PowerShell 静态检查、现有 CCS 全工程构建脚本。

## Global Constraints

- 目标器件固定为 MSPM0G3507 `LQFP-48(PT)`。
- I2C0 SDA 固定 PA28，SCL 固定 PA31；调试 UART 和 IMU 引脚不变。
- 模块七位地址固定为 `0x12`，数字状态寄存器固定为 `0x30`。
- `Track_Data_t` 的 `activeMask` bit0~bit7 统一表示 X1~X8 的黑线状态。
- I2C 轮询必须有有限超时，通信失败不得卡死主循环。
- 代码中的引脚只允许来自 SysConfig/`Bsp/board_pins.h`，不在 `User/user_config.h` 写 PA/PB 标识。

### Task 1: Add the failing acceptance check

**Files:**
- Create: `Tests/Build/test_eight_track_i2c.ps1`
- Modify: `Tests/Build/verify_build.ps1`

**Interfaces:**
- The test reads `empty.syscfg`, `User/user_config.h`, `Bsp/i2c.h`, `Bsp/i2c.c`, and `Hardware/track.c`.
- It will later prove the I2C instance, sensor constants, eight-channel mapping, and timeout guards are present.

- [ ] **Step 1: Write the failing test**

  Require the following exact conditions:

  ```powershell
  Require-Pattern 'I2C1\.\$name\s*=\s*"TRACK_I2C"' 'TRACK_I2C name is missing'
  Require-Pattern 'I2C1\.peripheral\.\$assign\s*=\s*"I2C0"' 'TRACK_I2C must use I2C0'
  Require-Pattern 'I2C1\.peripheral\.sdaPin\.\$assign\s*=\s*"PA28"' 'TRACK_I2C SDA must use PA28'
  Require-Pattern 'I2C1\.peripheral\.sclPin\.\$assign\s*=\s*"PA31"' 'TRACK_I2C SCL must use PA31'
  Require-Pattern '#define TRACK_CHANNEL_COUNT\s+8U' 'tracker must have eight channels'
  Require-Pattern '#define TRACK_I2C_ADDRESS\s+0x12U' 'tracker I2C address must be 0x12'
  Require-Pattern '#define TRACK_I2C_STATUS_REGISTER\s+0x30U' 'tracker status register must be 0x30'
  Require-Pattern 'DL_I2C_startControllerTransfer' 'I2C transfer call is missing'
  Require-Pattern 'TRACK_I2C_TIMEOUT_LOOPS' 'I2C timeout guard is missing'
  Require-Pattern 'TRACK_CHANNEL_COUNT == 8U' 'track implementation is not eight-channel'
  ```

- [ ] **Step 2: Run the test and verify RED**

  Run `pwsh.exe -NoLogo -NoProfile -NonInteractive -File Tests/Build/test_eight_track_i2c.ps1`.
  It must fail on the current five-channel/no-I2C project, proving the check is meaningful.

### Task 2: Configure I2C0 and centralize tracker parameters

**Files:**
- Modify: `empty.syscfg`
- Modify: `User/user_config.h`
- Modify: `Bsp/board_pins.h`

**Interfaces:**
- SysConfig produces `TRACK_I2C_INST` for the generic I2C wrapper.
- `board_pins.h` exports `PIN_TRACK_I2C_INST` and no longer exports obsolete five-channel GPIO pins.

- [ ] **Step 1: Add I2C0 SysConfig instance**

  Add the I2C module and configure it as a controller at Fast (400 kHz), named `TRACK_I2C`,
  with `sdaPin=PA28` and `sclPin=PA31`. Do not create replacement GPIO names for X1~X8.

- [ ] **Step 2: Replace tracker configuration macros**

  In `User/user_config.h`, replace the five-channel block with:

  ```c
  #define TRACK_CHANNEL_COUNT               8U
  #define TRACK_I2C_ADDRESS                 0x12U
  #define TRACK_I2C_STATUS_REGISTER         0x30U
  #define TRACK_I2C_TIMEOUT_LOOPS           200000U
  #define TRACK_BLACK_IS_HIGH               0
  #define TRACK_ACTIVE_THRESHOLD            500U
  ```

- [ ] **Step 3: Update the generated-name mapping**

  Add `#define PIN_TRACK_I2C_INST TRACK_I2C_INST` and remove the five `PIN_TRACK_CH*`
  definitions. Do not edit generated `Debug/syscfg` files by hand; the verification script regenerates them.

- [ ] **Step 4: Run the acceptance check**

  The check should still be RED because the I2C wrapper and track implementation are not present yet.

### Task 3: Add the bounded generic I2C register wrapper

**Files:**
- Create: `Bsp/i2c.h`
- Create: `Bsp/i2c.c`

**Interfaces:**

  ```c
  Status_t I2C_ReadRegister(uint8_t address, uint8_t reg,
                            uint8_t *data, uint8_t length);
  Status_t I2C_WriteRegister(uint8_t address, uint8_t reg,
                             const uint8_t *data, uint8_t length);
  ```

- [ ] **Step 1: Write the API-level source checks**

  Extend `test_eight_track_i2c.ps1` to require both prototypes, NULL/length validation,
  `DL_I2C_fillControllerTXFIFO`, `DL_I2C_startControllerTransfer`, RX FIFO reads, and a loop
  bounded by `TRACK_I2C_TIMEOUT_LOOPS`.

- [ ] **Step 2: Run the check and verify RED**

  Run the same PowerShell test; it must report the missing `Bsp/i2c.h`/`.c` API.

- [ ] **Step 3: Implement the minimal polling transaction**

  Use `PIN_TRACK_I2C_INST`. For a register read, send the register byte as a TX transfer,
  wait for IDLE with the finite loop bound, then start an RX transfer and copy bytes from
  `DL_I2C_receiveControllerData()`. For writes, send register plus payload in one TX transfer.
  Check address `<= 0x7F`, non-NULL buffers, nonzero length, controller IDLE before starting,
  `DL_I2C_CONTROLLER_STATUS_ERROR`/busy-bus errors, and return `STATUS_ERROR` on timeout.

- [ ] **Step 4: Run source checks**

  The I2C API checks must pass while the track-specific checks remain RED until Task 4.

### Task 4: Replace the five-channel GPIO adapter with the eight-channel I2C adapter

**Files:**
- Modify: `Hardware/track.c`
- Modify: `Hardware/track.h`
- Modify: `Hardware/track_math.h`

**Interfaces:**
- Preserve `Status_t Track_Init(void)`, `Status_t Track_Update(void)`, `Track_SetRawSamples`,
  `Track_GetData`, and `Track_GetPositionError`.
- `Track_Update()` reads one byte from `TRACK_I2C_ADDRESS/STATUS_REGISTER`, maps X1=bit7 through
  X8=bit0 into `raw[0..7]` as 1000/0 black indicators, and computes position using weights
  `{-7,-5,-3,-1,1,3,5,7}`.

- [ ] **Step 1: Add a failing mapping check**

  Require the source to contain all eight `(rawMask & (1U << (7U - index)))`-equivalent mapping
  behavior, `TrackMath_WeightedPosition`, and no `PIN_TRACK_CH1` references.

- [ ] **Step 2: Run the check and verify RED**

  The old GPIO implementation must fail this check.

- [ ] **Step 3: Implement the adapter**

  Remove the GPIO port/pin arrays and average filters. Keep external sample injection and
  calibration APIs for compatibility; calibration operates on injected samples only. On I2C
  failure, return `STATUS_ERROR`, set `g_data.lineFound=false` only for the communication state,
  and retain the previous position error so the control layer does not receive a false hard turn.

- [ ] **Step 4: Update documentation comments**

  Change the header description from “temporary five-channel digital adapter” to the eight-channel
  I2C adapter and document `activeMask` bit0=X1 through bit7=X8.

- [ ] **Step 5: Run the mapping check**

  The complete tracker acceptance script must pass.

### Task 5: Integrate test output and build verification

**Files:**
- Modify: `User/test.c`
- Modify: `User/task.c`
- Modify: `Tests/Build/test_target_pinout.ps1`
- Modify: `Tests/Build/verify_build.ps1`
- Modify: `docs/模块逐项测试流程.md`

**Interfaces:**
- `TEST_TRACK` remains selected from `User/task.c` and reports `mask`, error, and I2C status.
- The normal car-control path continues to call the unchanged Track API.

- [ ] **Step 1: Add failing integration checks**

  Require `verify_build.ps1` to invoke `test_eight_track_i2c.ps1`, require `TEST_TRACK`, and
  require the test guide to show 5V/GND/SDA/SCL wiring and the calibration sequence.

- [ ] **Step 2: Run RED checks**

  Run `Tests/Build/verify_build.ps1`; record any pre-existing user-local changes separately,
  and do not modify them just to silence a check.

- [ ] **Step 3: Implement diagnostics**

  Rate-limit `TEST_TRACK` to 100 ms, print `TRACK mask=%02X error=%.2f i2c=OK/ERR`, and leave
  the current test-selection mechanism intact. Add the physical wiring and X1~X8 expected mask
  table to the Chinese test guide.

- [ ] **Step 4: Run full verification**

  Run `pwsh.exe -NoLogo -NoProfile -NonInteractive -File Tests/Build/verify_build.ps1`.
  Expected output includes `TARGET PINOUT CHECK PASSED`, `EIGHT TRACK I2C CHECK PASSED`, all
  existing API checks, and `BUILD VERIFIED: ...Debug/full_verify/MSPM0G3507_EContest_BSP.out`.

- [ ] **Step 5: Review diff and commit only this feature**

  Use `git status --short`, `git diff --check`, and `git diff --stat`. Stage only the new I2C/
  tracker files, related tests/docs, and SysConfig changes; leave unrelated user modifications
  unstaged. Commit with `feat: add eight-channel I2C tracker adapter`.
