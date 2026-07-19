# 12-Channel Path Fish I2C Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the current eight-channel register-based tracker adapter with the 12-channel Path Fish direct-read I2C protocol while preserving the public `Track_*` API and safe-stop behavior.

**Architecture:** `Bsp/i2c.*` adds a bounded raw-read transaction, `Hardware/track_protocol.*` parses the vendor's two seven-byte half frames without DriverLib dependencies, and `Hardware/track.*` converts a complete 12-bit module mask into the project's normalized samples, state and position error. Application code continues to consume `Track_Data_t`.

**Tech Stack:** C11, TI MSPM0 DriverLib/SysConfig, TI Arm Clang, host-side GCC tests, PowerShell verification scripts.

## Global Constraints

- Keep I2C0 on PA28 SDA and PA31 SCL at 400 kHz.
- Keep PB2 as the active-low emergency-stop key; do not allocate UART3 PB2/PB3 to the tracker.
- Use the unshifted 7-bit I2C address `0x48`.
- Never wait indefinitely for I2C FIFO or bus state.
- Publish sensor data only after both `'#'` and `'!'` halves pass validation.
- Preserve `bit0=leftmost channel` and keep the position-error range near `-7.0..+7.0`.
- Do not modify or discard existing uncommitted CCS/debug configuration files.

---

### Task 1: Add and prove the pure 12-channel protocol and math behavior

**Files:**
- Create: `Hardware/track_protocol.h`
- Create: `Hardware/track_protocol.c`
- Modify: `Hardware/track_math.h`
- Modify: `Hardware/track_math.c`
- Modify: `Tests/Host/test_main.c`
- Modify: `Tests/Host/run_tests.ps1`

**Interfaces:**
- Produces: `TrackProtocol_Reset()` and `TrackProtocol_PushHalf()`.
- Produces: `TrackMath_WeightedPosition(uint16_t activeMask, const float *weights, uint8_t count)` supporting up to 12 channels.

- [ ] Add host tests for high mask bits, 12-channel weighting, normal/reversed half order, duplicate halves, invalid header, invalid payload and reset-on-error.
- [ ] Run `Tests/Host/run_tests.ps1` and confirm the new tests fail because the protocol module and 16-bit math API do not exist.
- [ ] Implement the minimal pure-C parser and 16-bit weighted-position support.
- [ ] Re-run host tests and confirm all tests pass.

### Task 2: Add bounded raw I2C reads and integrate the 12-channel adapter

**Files:**
- Modify: `Bsp/i2c.h`
- Modify: `Bsp/i2c.c`
- Modify: `Hardware/track.h`
- Modify: `Hardware/track.c`
- Modify: `User/user_config.h`

**Interfaces:**
- Produces: `Status_t I2C_Read(uint8_t address, uint8_t *data, uint8_t length)`.
- Preserves: `Track_Init()`, `Track_Update()`, `Track_GetData()` and `Track_GetPositionError()`.

- [ ] Add source-level verification expectations for raw reads, address `0x48`, 12 channels, `uint16_t activeMask` and full-mask `0x0FFF`.
- [ ] Run the target tracker check and confirm it fails against the eight-channel implementation.
- [ ] Implement bounded raw reads using existing retry/recovery helpers.
- [ ] Integrate the stateful two-half parser, normalized 12-channel weights, stale/incomplete-frame handling, installation reversal and black-level inversion.
- [ ] Run host and tracker checks and confirm they pass.

### Task 3: Update diagnostics, target checks and user documentation

**Files:**
- Modify: `User/test.c`
- Modify: `User/test.h`
- Modify: `User/task.c`
- Replace: `Tests/Build/test_eight_track_i2c.ps1` with `Tests/Build/test_twelve_track_i2c.ps1`
- Modify: `Tests/Build/test_target_pinout.ps1`
- Modify: `Tests/Build/verify_build.ps1`
- Modify: `README.md`
- Modify: `docs/引脚资源分配表.md`
- Modify: `docs/SysConfig配置指南.md`
- Modify: `docs/模块逐项测试流程.md`
- Modify: `docs/上板调试清单.md`
- Modify: `docs/项目总览与使用指南.md`
- Modify: `docs/工程文件说明.md`

**Interfaces:**
- `TEST_TRACK` prints a three-digit mask plus a 12-character left-to-right bit string.
- LCD normal-mode tracker status uses a three-digit mask.

- [ ] Add failing static checks that reject the old `0x12/0x30/eight-channel` adapter and require the new parser and diagnostics.
- [ ] Update diagnostics and all beginner-facing documents to the Path Fish wiring, calibration and expected masks.
- [ ] Run the complete static/host suite and correct any stale eight-channel assumptions.

### Task 4: Full target verification and delivery commit

**Files:**
- Verify all changed production, test and documentation files.

**Interfaces:**
- Produces a TI Arm Clang linked firmware image through the existing verification workflow.

- [ ] Run `Tests/Build/verify_build.ps1` from a fresh invocation.
- [ ] Inspect `git diff --check`, `git diff --stat` and `git status --short`.
- [ ] Confirm the pre-existing `.cproject`, `.theia/launch.json` and `targetConfigs/MSPM0G3507.ccxml` changes remain present and are not included in the feature commit.
- [ ] Commit only the 12-channel migration files with message `feat: migrate tracker to 12-channel Path Fish`.
