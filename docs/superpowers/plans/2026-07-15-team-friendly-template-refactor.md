# Team-Friendly MSPM0G3507 Template Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert the current BSP template into a beginner-friendly `User/Bsp/Hardware/Control` project while moving the active hardware to the final motor/encoder/IMU/LCD pins and providing a safe temporary five-channel tracker.

**Architecture:** `User` owns the competition state machine and the only beginner-facing configuration header. `Control` owns PID and car-control coordination; `Hardware` owns semantic module drivers; `Bsp` owns DriverLib/SysConfig-facing infrastructure. The LCD is a direct-draw ST7735S SPI1 driver without a full framebuffer. The temporary five-channel tracker has the same public error/data interface that the later UART/I2C 12-channel adapter will implement.

**Tech Stack:** MSPM0G3507, TI DriverLib, SysConfig 1.26.2, MSPM0 SDK 2.10.0.04, TI Clang 4.0.2 LTS, CCS, PowerShell build checks, ARM GCC host compile checks.

## Global Constraints

- Do not edit generated `ti_msp_dl_config.c/.h`; edit only `empty.syscfg` and regenerate.
- Use `apply_patch` for tracked file modifications; preserve a clean `main` branch by working in an isolated worktree during execution.
- Keep all public names in `Module_Action()` form; do not add a `BSP_` prefix.
- `PA2` is permanently reserved for the board ROSC circuit and must not be configured or referenced as a tracker input.
- Temporary tracker channels are `PA0`, `PA1`, `PA15`, `PB6`, `PB7`; temporary active-low key is `PB2` with internal pull-up.
- Final motor, encoder, UART2 IMU and SPI1 ST7735S pins are exactly those in `docs/superpowers/specs/2026-07-15-team-friendly-template-design.md`.
- The first LCD implementation uses SPI1 mode 3 at 4 MHz and no DMA or full-screen RAM framebuffer.
- Every production-code behavior change begins with a failing automated test or a failing static configuration check, followed by a passing rerun.

---

### Task 1: Establish a safe baseline and target-pin regression checks

**Files:**
- Create: `Tests/Build/test_target_pinout.ps1`
- Modify: `Tests/Build/verify_build.ps1`
- Modify: `Tests/Host/run_tests.ps1`

**Interfaces:**
- Consumes: current `empty.syscfg`, source files, SysConfig CLI and TI Clang paths.
- Produces: repeatable commands that fail when the final pin plan or source-directory compilation boundary regresses.

- [ ] **Step 1: Run the existing baseline checks before any code move**

Run:

```powershell
Set-Location E:\Desktop\zhengliu\MSPM0G3507_EContest_BSP
.\Tests\Host\run_tests.ps1
.\Tests\Build\test_no_host_objects.ps1
.\Tests\Build\test_vscode_config.ps1
.\Tests\Build\test_review_feedback.ps1
.\Tests\Build\verify_build.ps1
```

Expected: all scripts report `PASSED` or `COMPILE CLEANLY`; record any pre-existing failure before continuing.

- [ ] **Step 2: Write the failing final-pin static check**

Create `Tests/Build/test_target_pinout.ps1` with checks for the target names and the forbidden PA2 reference:

```powershell
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$syscfg = Get-Content -LiteralPath (Join-Path $project 'empty.syscfg') -Raw

function Require([string]$pattern, [string]$message) {
    if ($syscfg -cnotmatch $pattern) { throw $message }
}
function Forbid([string]$pattern, [string]$message) {
    if ($syscfg -cmatch $pattern) { throw $message }
}

Require 'TRACK_CH1[\s\S]*PA0'  'TRACK_CH1 must use PA0'
Require 'TRACK_CH2[\s\S]*PA1'  'TRACK_CH2 must use PA1'
Require 'TRACK_CH3[\s\S]*PA15' 'TRACK_CH3 must use PA15'
Require 'TRACK_CH4[\s\S]*PB6'  'TRACK_CH4 must use PB6'
Require 'TRACK_CH5[\s\S]*PB7'  'TRACK_CH5 must use PB7'
Require 'USER_KEY[\s\S]*PB2'   'temporary USER_KEY must use PB2'
Require 'LCD_SPI[\s\S]*SPI1'   'LCD must use SPI1'
Require 'sclkPin\.\$assign\s*=\s*"PB9"' 'LCD SCK must use PB9'
Require 'picoPin\.\$assign\s*=\s*"PB8"' 'LCD MOSI must use PB8'
Require 'LCD_CS[\s\S]*PA27' 'LCD_CS must use PA27'
Require 'LCD_DC[\s\S]*PA26' 'LCD_DC must use PA26'
Require 'LCD_RES[\s\S]*PA25' 'LCD_RES must use PA25'
Require 'LCD_BL[\s\S]*PA24' 'LCD_BL must use PA24'
Forbid 'TRACK_[A-Z0-9_]+[\s\S]*PA2' 'PA2 must never be a tracker signal'
Write-Output 'TARGET PINOUT CHECK PASSED'
```

- [ ] **Step 3: Run the check and verify the expected red state**

Run:

```powershell
.\Tests\Build\test_target_pinout.ps1
```

Expected: FAIL because the current SysConfig still contains the old OLED/eight-channel pin plan.

- [ ] **Step 4: Extend the complete build script to include the final-pin check**

At the start of `Tests/Build/verify_build.ps1`, before invoking SysConfig, add:

```powershell
& (Join-Path $PSScriptRoot 'test_target_pinout.ps1')
if ($LASTEXITCODE -ne 0) { throw 'Target pinout check failed' }
```

- [ ] **Step 5: Commit the red-check scaffold**

```powershell
git add Tests/Build/test_target_pinout.ps1 Tests/Build/verify_build.ps1
git commit -m "test: add final pinout regression check"
```

### Task 2: Replace SysConfig resources and centralize beginner-facing configuration

**Files:**
- Create: `User/user_config.h`
- Create: `Bsp/board_pins.h`
- Move: `BSP/Inc/common.h`, `BSP/Inc|Src/board.*`, `BSP/Inc|Src/timer.*`, `BSP/Inc|Src/uart.*` to `Bsp/`
- Modify: `empty.syscfg`
- Modify: `Bsp/board.c`
- Modify: `.vscode/c_cpp_properties.json`
- Test: `Tests/Build/test_target_pinout.ps1`

**Interfaces:**
- Produces: `User/user_config.h` for parameters only; `Bsp/board_pins.h` for generated pin aliases only.
- Consumes: generated labels from `empty.syscfg`; no module may hard-code a physical pin outside `board_pins.h`.

- [ ] **Step 1: Add a failing configuration-boundary check**

Append to `Tests/Build/test_target_pinout.ps1`:

```powershell
$userConfig = Get-Content -LiteralPath (Join-Path $project 'User\user_config.h') -Raw
if ($userConfig -cmatch 'DL_GPIO_PIN_|GPIOA|GPIOB|PA[0-9]+|PB[0-9]+') {
    throw 'User configuration must not contain physical pin identifiers'
}
```

Run the script. Expected: FAIL because `User/user_config.h` does not exist.

- [ ] **Step 2: Create the `Bsp` directory before its generated-pin adapter**

Move `common.*`, `board.*`, `timer.*`, and `uart.*` from the old `BSP` tree into `Bsp/`. Update their include paths only; preserve behavior at this point. Keep the old `bsp_config.h` temporarily so these sources still compile while the remaining modules are migrated in later tasks.

- [ ] **Step 3: Rewrite `empty.syscfg` to the active hardware profile**

Keep SWD, SysTick, TIMG0 PWM, UART0 debug and add the following named resources through SysConfig GUI or equivalent script assignments:

```text
MOTOR_PWM: TIMG0 C0=PA12, C1=PA13
MOTOR_AIN1=PB18, MOTOR_AIN2=PA7, MOTOR_BIN1=PA8, MOTOR_BIN2=PA9, MOTOR_STBY=PB24
ENCODER_LEFT_A=PA17 input capture, ENCODER_LEFT_B=PA16 GPIO input
ENCODER_RIGHT_A=PB19 input capture, ENCODER_RIGHT_B=PB20 GPIO input
IMU_UART: UART2 TX=PA21, RX=PA22, 115200 RX interrupt
LCD_SPI: SPI1 SCK=PB9, PICO=PB8, controller, 8-bit MSB-first, MOTO3, 4 MHz
LCD_CS=PA27, LCD_DC=PA26, LCD_RES=PA25, LCD_BL=PA24 GPIO outputs
TRACK_CH1=PA0, TRACK_CH2=PA1, TRACK_CH3=PA15, TRACK_CH4=PB6, TRACK_CH5=PB7 GPIO inputs
USER_KEY=PB2 GPIO input with pull-up
```

Remove `OLED_I2C`, old OLED SPI resources, the PA2 tracker assignment, the old PA21/PA22 encoder assignments, old PA24/PB8/PB9 tracker assignments, and the old PB6 key assignment. Do not configure I2C0, UART3 or PA2 in this active profile.

- [ ] **Step 4: Create the two configuration headers**

Create `User/user_config.h` with no physical pins:

```c
#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

#define CONFIG_LCD_ENABLE               1
#define CONFIG_IMU_ENABLE               1
#define CONFIG_TRACK_ENABLE             1
#define CONFIG_KEY_ENABLE               1
#define TRACK_CHANNEL_COUNT             5U
#define TRACK_BLACK_IS_HIGH             1
#define TRACK_FILTER_LENGTH             4U
#define TRACK_ACTIVE_THRESHOLD          500U
#define MOTOR_MAX_DUTY                  1000
#define MOTOR_MIN_START_DUTY            80
#define MOTOR_RAMP_STEP                 40
#define ENCODER_PULSES_PER_MOTOR_REV    13.0f
#define ENCODER_COUNT_MULTIPLIER        2.0f
#define ENCODER_GEAR_RATIO              30.0f
#define ENCODER_WHEEL_DIAMETER_M        0.065f
#define CONTROL_SAMPLE_TIME_S           0.010f
#define CAR_DEFAULT_BASE_SPEED_RPM      120.0f

#endif
```

Create `Bsp/board_pins.h` as the only translation from logical names to SysConfig/DriverLib names. It must include `ti_msp_dl_config.h` and define aliases such as:

```c
#define PIN_LCD_CS_PORT        GPIOA
#define PIN_LCD_CS             GPIO_A_LCD_CS_PIN
#define PIN_LCD_DC_PORT        GPIOA
#define PIN_LCD_DC             GPIO_A_LCD_DC_PIN
#define PIN_LCD_RES_PORT       GPIOA
#define PIN_LCD_RES            GPIO_A_LCD_RES_PIN
#define PIN_LCD_BL_PORT        GPIOA
#define PIN_LCD_BL             GPIO_A_LCD_BL_PIN
#define PIN_TRACK_CH1_PORT     GPIOA
#define PIN_TRACK_CH1          GPIO_A_TRACK_CH1_PIN
#define PIN_TRACK_CH2_PORT     GPIOA
#define PIN_TRACK_CH2          GPIO_A_TRACK_CH2_PIN
#define PIN_TRACK_CH3_PORT     GPIOA
#define PIN_TRACK_CH3          GPIO_A_TRACK_CH3_PIN
#define PIN_TRACK_CH4_PORT     GPIOB
#define PIN_TRACK_CH4          GPIO_B_TRACK_CH4_PIN
#define PIN_TRACK_CH5_PORT     GPIOB
#define PIN_TRACK_CH5          GPIO_B_TRACK_CH5_PIN
#define PIN_USER_KEY_PORT      GPIOB
#define PIN_USER_KEY           GPIO_B_USER_KEY_PIN
```

Use the generated macro spelling emitted by SysConfig; do not invent a second physical-pin table.

- [ ] **Step 5: Run the static check and full SysConfig build until green**

Run:

```powershell
.\Tests\Build\test_target_pinout.ps1
.\Tests\Build\verify_build.ps1
```

Expected: pinout check passes. The full build may still fail on old source references; record this as the expected next refactor task.

- [ ] **Step 6: Commit configuration migration**

```powershell
git add empty.syscfg User/user_config.h Bsp/board_pins.h Bsp/board.c .vscode/c_cpp_properties.json Tests/Build
git commit -m "feat: configure final board pins and temporary five-channel tracker"
```

### Task 3: Create the simple source layout while preserving pure algorithm tests

**Files:**
- Create: `User/project.h`, `User/main.c`, `User/task.h`, `User/task.c`, `User/test.h`, `User/test.c`
- Move: `Components/PID/pid.*` to `Control/pid.*`
- Move: `Services/Inc|Src/scheduler.*` to `Control/scheduler.*`
- Move: `Components/Filter/filter_average.*` and `Components/Track/track_math.*` to `Hardware/`
- Move: `Components/RingBuffer/ring_buffer.*`, `Components/Protocol/frame_protocol.*`, and `BSP/Inc|Src/command.*` to `Bsp/`
- Move: `Components/Protocol/imu_protocol.*` and `BSP/Inc|Src/imu_uart.*` to `Hardware/imu.*`
- Modify: `Tests/Host/run_tests.ps1`, `Tests/Host/test_main.c`, `.cproject`

**Interfaces:**
- Produces: `System_Init()`, `Task_Init()`, `Task_Run()` and beginner-facing `User/project.h`.
- Preserves: `Status_t`, `PID_t`, `PID_Config_t`, `Scheduler_Task_t` and existing PID behavior.

- [ ] **Step 1: Update host-test paths first and verify the red state**

In `Tests/Host/run_tests.ps1`, change only the include/source paths to the planned locations:

```powershell
(Join-Path $project 'Bsp\common.h')
(Join-Path $project 'Control\pid.c')
(Join-Path $project 'Hardware\filter_average.c')
(Join-Path $project 'Hardware\track_math.c')
(Join-Path $project 'Control\scheduler.c')
```

Run `Tests/Host/run_tests.ps1`. Expected: FAIL because the target files do not exist.

- [ ] **Step 2: Move the pure modules without changing their public APIs**

Move the listed `.c/.h` pairs to their target directories, update their direct includes from `common.h`, and update the test script include paths. Keep these signatures unchanged:

```c
Status_t PID_Init(PID_t *pid, const PID_Config_t *config);
float PID_UpdatePosition(PID_t *pid, float target, float feedback);
float PID_UpdateIncremental(PID_t *pid, float target, float feedback);
void Scheduler_Init(uint32_t nowMs);
bool Scheduler_IsDue(Scheduler_Task_t task, uint32_t nowMs);
```

Move the ring buffer and command-frame sources into `Bsp/`; keep their existing public interfaces so UART debugging and future host-command support are not lost. Merge `imu_protocol.c/.h` into the `Hardware/imu.c/.h` implementation as private parser functions; preserve `IMU_GetSample()`, `IMU_Process()`, `IMU_IsOnline()` and `IMU_StartYawZero()` as the public IMU API.

- [ ] **Step 3: Add the simple User-layer entry API**

Create `User/task.h`:

```c
#ifndef USER_TASK_H
#define USER_TASK_H

#include "common.h"

Status_t System_Init(void);
Status_t Task_Init(void);
void Task_Run(void);

#endif
```

Create `User/main.c`:

```c
#include "project.h"

int main(void)
{
    if (System_Init() != STATUS_OK) {
        while (1) { __WFI(); }
    }
    if (Task_Init() != STATUS_OK) {
        while (1) { __WFI(); }
    }
    while (1) {
        Task_Run();
        __WFI();
    }
}
```

Create `User/project.h` to include only `user_config.h`, `task.h`, `test.h`, public Control headers and public Hardware headers.

- [ ] **Step 4: Verify green host compile and source-boundary compilation**

Run:

```powershell
.\Tests\Host\run_tests.ps1
```

Expected: `TEST SOURCES COMPILE CLEANLY`.

Update `.cproject` so `Tests/**`, `docs/**`, `.vscode/**` and `Debug/**` are excluded from the CCS source root. Run a CCS build or `Tests/Build/verify_build.ps1`; expected temporary failures only for modules not yet moved in Tasks 4–6.

- [ ] **Step 5: Commit the layout foundation**

```powershell
git add User Bsp Control Hardware Tests/Host .cproject
git commit -m "refactor: introduce beginner-friendly source layout"
```

### Task 4: Migrate board, motor, encoder, key, LED and five-channel tracker

**Files:**
- Move: current motor/encoder/key/led sources to `Hardware/`
- Create: `Hardware/track.c`, `Hardware/track.h`
- Modify: `User/user_config.h`, `User/test.c`, `Tests/Host/test_main.c`
- Test: `Tests/Build/test_target_pinout.ps1`, `Tests/Host/run_tests.ps1`

**Interfaces:**
- Produces: `Motor_*`, `Encoder_*`, `Key_*`, `LED_*`, `Track_*`, `Timer_*`, `UART_*` and `Board_EmergencyStop()` from the simplified paths.
- `Track_Data_t` changes from fixed 8 elements to `TRACK_CHANNEL_COUNT` elements and reports a five-bit `activeMask`.

- [ ] **Step 1: Add a failing five-channel position test**

Add to `Tests/Host/test_main.c`:

```c
static void Test_TrackMath_FiveChannels(void)
{
    const float weights[5] = {-4.0f, -2.0f, 0.0f, 2.0f, 4.0f};
    CHECK_NEAR(TrackMath_WeightedPosition(0x04U, weights, 5U), 0.0f, 0.0001f);
    CHECK_NEAR(TrackMath_WeightedPosition(0x03U, weights, 5U), -3.0f, 0.0001f);
    CHECK_NEAR(TrackMath_WeightedPosition(0x18U, weights, 5U), 3.0f, 0.0001f);
}
```

Call it from `main`. Run the host test. Expected: FAIL because the existing track math interface does not expose this five-channel signature.

- [ ] **Step 2: Implement generic five-channel track math**

In `Hardware/track_math.h` define:

```c
float TrackMath_WeightedPosition(uint8_t activeMask,
                                 const float *weights,
                                 uint8_t count);
```

Implement it in `Hardware/track_math.c` with this behavior:

```c
float TrackMath_WeightedPosition(uint8_t activeMask,
                                 const float *weights,
                                 uint8_t count)
{
    float sum = 0.0f;
    uint8_t active = 0U;
    for (uint8_t i = 0U; i < count; ++i) {
        if ((activeMask & (uint8_t)(1U << i)) != 0U) {
            sum += weights[i];
            active++;
        }
    }
    return (active == 0U) ? 0.0f : (sum / (float)active);
}
```

- [ ] **Step 3: Implement the GPIO5 tracker adapter**

Use this public data structure in `Hardware/track.h`:

```c
typedef struct {
    uint16_t raw[TRACK_CHANNEL_COUNT];
    uint16_t filtered[TRACK_CHANNEL_COUNT];
    float positionError;
    uint8_t activeMask;
    bool lineFound;
} Track_Data_t;
```

`Track_Update()` must read exactly `PIN_TRACK_CH1` through `PIN_TRACK_CH5`, convert each level to `0U` or `1000U` according to `TRACK_BLACK_IS_HIGH`, run the existing average filter, build `activeMask`, calculate the weighted error using `{-4,-2,0,2,4}`, and set `lineFound` when the mask is nonzero. `Track_SetRawSamples()` remains available for host tests and future UART/I2C adapters.

- [ ] **Step 4: Move the existing hardware modules and remap final pin aliases**

Keep public Motor and Encoder APIs unchanged. Replace all old `bsp_config.h` physical-pin references with `board_pins.h` aliases. Ensure motor A/B direction follows the final mapping:

```c
AIN1 = PB18; AIN2 = PA7;
BIN1 = PA8;  BIN2 = PA9;
```

Configure the encoder ISR adapter to use left A=PA17/B=PA16 and right A=PB19/B=PB20. Configure `Key_Init()` to use `PIN_USER_KEY`, active-low with the internal pull-up.

Remove the old `adc_track.*` source from the active profile: the temporary board has no ADC tracker inputs and no ADC SysConfig instance. Preserve its conversion notes in `docs/模块接口手册.md`; a future analog tracker can add a new `Hardware/track_adc.c` adapter without changing the public `Track_*` interface.

- [ ] **Step 5: Implement beginner-visible hardware tests**

`User/test.c` must provide:

```c
void Test_Run(uint32_t nowMs);
void Test_Select(Test_Id_t id);
```

For `TEST_TRACK`, call `Track_Update()` every 10 ms and print `activeMask` and `positionError` through `UART_Printf`. For `TEST_KEY`, print `PRESSED`, `CLICKED`, and `LONG` events. Motor test must leave the default state stopped and require an explicit test selection.

- [ ] **Step 6: Verify green**

Run:

```powershell
.\Tests\Host\run_tests.ps1
.\Tests\Build\test_target_pinout.ps1
.\Tests\Build\verify_build.ps1
```

Expected: all tests pass; no source may mention `TRACK_D0` through `TRACK_D7` or use PA2 as tracker input.

- [ ] **Step 7: Commit hardware migration**

```powershell
git add Bsp Hardware User Tests
git commit -m "feat: add final motor hardware and temporary five-channel tracker"
```

### Task 5: Replace SSD1306 OLED with ST7735S LCD

**Files:**
- Create: `Hardware/lcd.c`, `Hardware/lcd.h`
- Modify: `Bsp/spi.c`, `Bsp/spi.h`, `Bsp/board.c`, `User/test.c`
- Delete: old OLED/I2C/SSD1306 source files after replacement is green
- Test: `Tests/Build/test_lcd_api.ps1`, `Tests/Build/verify_build.ps1`

**Interfaces:**
- Produces: `LCD_Init`, `LCD_Clear`, `LCD_Fill`, `LCD_ShowString`, `LCD_ShowInt`, `LCD_ShowFloat`, `LCD_SetBacklight`.
- Removes: all `OLED_*`, `SSD1306_*`, `OLED_I2C_*` and `OLED_USE_SPI` references.

- [ ] **Step 1: Create the failing LCD API static test**

Create `Tests/Build/test_lcd_api.ps1`:

```powershell
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$header = Get-Content -LiteralPath (Join-Path $project 'Hardware\lcd.h') -Raw
$source = Get-Content -LiteralPath (Join-Path $project 'Hardware\lcd.c') -Raw
foreach ($name in 'LCD_Init','LCD_Clear','LCD_Fill','LCD_ShowString','LCD_SetBacklight') {
    if ($header -cnotmatch "\b$name\b") { throw "lcd.h missing $name" }
}
if ($source -cnotmatch '0x11' -or $source -cnotmatch '0x3A' -or $source -cnotmatch '0x29') {
    throw 'ST7735S initialization sequence is incomplete'
}
if ($source -cmatch 'OLED_|SSD1306_') { throw 'LCD source must not depend on OLED or SSD1306' }
Write-Output 'LCD API CHECK PASSED'
```

Run it. Expected: FAIL because the LCD files do not exist.

- [ ] **Step 2: Extend SPI1 transport before LCD logic**

Expose this API in `Bsp/spi.h`:

```c
Status_t SPI_Init(void);
Status_t SPI_Write(const uint8_t *data, uint16_t length);
```

`SPI_Write()` must reject `NULL` or zero length, send every byte through `DL_SPI_transmitData8(LCD_SPI_INST, ...)`, wait for `DL_SPI_isBusy()` with a finite timeout, and return `STATUS_TIMEOUT` on expiry. It must not reference OLED configuration names.

- [ ] **Step 3: Implement the ST7735S command transport and initialization**

Use these command helpers in `Hardware/lcd.c`:

```c
static Status_t LCD_WriteCommandData(uint8_t command,
                                     const uint8_t *data,
                                     uint8_t length);
static Status_t LCD_SetAddressWindow(uint16_t x1, uint16_t y1,
                                     uint16_t x2, uint16_t y2);
```

`LCD_WriteCommandData()` holds CS low across the command and its parameter bytes, uses DC low for the command and high for data, and always returns CS high. `LCD_Init()` performs reset low/high delays with `Timer_DelayMs`, sends the vendor ST7735S register sequence, writes `0x3A, 0x05` for RGB565, then `0x29` for display on. Define these configuration constants in `lcd.h` or `board_pins.h`:

```c
#define LCD_WIDTH     128U
#define LCD_HEIGHT    160U
#define LCD_X_OFFSET  2U
#define LCD_Y_OFFSET  1U
#define LCD_MADCTL    0x00U
```

RGB565 pixel data is always high byte first. No readback API is implemented because the LCD has no MISO connection.

- [ ] **Step 4: Implement direct drawing and status text**

Implement `LCD_Fill()` by opening a single address window and streaming `(x2-x1+1)*(y2-y1+1)` RGB565 pixels. Bounds-check all public coordinates. `LCD_Clear(color)` calls `LCD_Fill(0, 0, LCD_WIDTH-1, LCD_HEIGHT-1, color)`. Implement the existing compact ASCII font directly in `lcd.c`; `LCD_ShowString()` draws immediately and performs no full-screen refresh.

- [ ] **Step 5: Add the LCD smoke-test entry and verify green**

`TEST_LCD` in `User/test.c` must show red, green, blue and white rectangles, then render `LCD OK`. Add `test_lcd_api.ps1` to the start of `verify_build.ps1` and run:

```powershell
.\Tests\Build\test_lcd_api.ps1
.\Tests\Build\verify_build.ps1
```

Expected: both pass. On hardware, confirm color order and adjust only `LCD_MADCTL`, `LCD_X_OFFSET`, `LCD_Y_OFFSET` if needed.

- [ ] **Step 6: Delete obsolete display files and commit**

Delete the old OLED/I2C/SSD1306 source and headers only after the full build passes. Then:

```powershell
git add Bsp Hardware User Tests empty.syscfg
git rm -r Components/SSD1306 BSP/Inc/oled.h BSP/Src/oled.c BSP/Inc/i2c.h BSP/Src/i2c.c BSP/Inc/spi.h BSP/Src/spi.c
git commit -m "feat: replace OLED with ST7735S LCD"
```

### Task 6: Merge service logic into `Control` and create the beginner task loop

**Files:**
- Create: `Control/car_control.c`, `Control/car_control.h`
- Modify: `User/task.c`, `User/test.c`, `Bsp/board.c`, `Hardware/imu.c`
- Delete: old `Services/*`, `App/*`, root `empty.c`
- Test: `Tests/Host/test_main.c`, `Tests/Build/verify_build.ps1`

**Interfaces:**
- Produces:

```c
Status_t CarControl_Init(void);
void CarControl_Enable(bool enable);
void CarControl_SetBaseSpeed(float rpm);
void CarControl_Update(void);
void CarControl_Stop(void);
const CarControl_Data_t *CarControl_GetData(void);
```

- `Task_Init()` and `Task_Run()` replace `App_Init()` and `App_Run()`.

- [ ] **Step 1: Add a failing compile-time interface check**

Create `Tests/Build/test_user_api.ps1`:

```powershell
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$task = Get-Content -LiteralPath (Join-Path $project 'User\task.c') -Raw
$main = Get-Content -LiteralPath (Join-Path $project 'User\main.c') -Raw
foreach ($name in 'System_Init','Task_Init','Task_Run','CarControl_Update') {
    if (($task + $main) -cnotmatch "\b$name\b") { throw "User API missing $name" }
}
if (($task + $main) -cmatch '\bDL_[A-Za-z0-9_]+') { throw 'User layer must not call DriverLib directly' }
Write-Output 'USER API CHECK PASSED'
```

Run it. Expected: FAIL before `CarControl` and the new task files are complete.

- [ ] **Step 2: Merge velocity and line-follow logic under `CarControl`**

Move the existing `MotorControl_Data_t` state and left/right PID objects into `Control/car_control.c`. `CarControl_Update()` executes, in order:

```c
Track_Update();
Encoder_UpdateSpeed(CONTROL_SAMPLE_TIME_S);
/* update steering PID from Track_GetPositionError() */
/* update left/right speed PID from Encoder_GetData() */
Motor_SetDutyPair(leftDuty, rightDuty);
Motor_RampUpdate();
```

When disabled or line is absent, command zero target speed and stop safely. Preserve separate left and right PID configuration structures.

- [ ] **Step 3: Implement the User task state machine**

Use this state type in `User/task.c`:

```c
typedef enum {
    TASK_WAIT_START = 0,
    TASK_RUNNING,
    TASK_STOPPED
} Task_State_t;
```

`System_Init()` must call `SYSCFG_DL_init()` through `Board_Init()`, then initialize timer, UART, motor, encoder, track, IMU, key, LED and LCD in dependency order. `Task_Run()` must call `Key_Scan()` at 5 ms, `CarControl_Update()` at 10 ms only in `TASK_RUNNING`, `IMU_Process()` at 20 ms, and LCD status drawing at 200 ms. A key click toggles `TASK_WAIT_START`/`TASK_RUNNING`; `TASK_STOPPED` always calls `CarControl_Stop()`.

- [ ] **Step 4: Verify green and remove the obsolete source tree**

Run:

```powershell
.\Tests\Host\run_tests.ps1
.\Tests\Build\test_user_api.ps1
.\Tests\Build\verify_build.ps1
```

Expected: all pass. Then delete the old `App`, `Services`, `Examples`, `Components`, `BSP`, and root `empty.c` only after their replacements are built and the source tree contains no include reference to them.

- [ ] **Step 5: Commit control and User migration**

```powershell
git add User Control Bsp Hardware Tests .cproject
git rm -r App Services Examples Components BSP empty.c
git commit -m "refactor: simplify task loop and control layout"
```

### Task 7: Make the project self-explanatory and complete final verification

**Files:**
- Modify: `README.md`, `docs/模块接口手册.md`, `docs/SysConfig配置指南.md`, `docs/引脚资源分配表.md`, `docs/电赛快速上手指南.md`, `.vscode/c_cpp_properties.json`, `Tests/Build/test_vscode_config.ps1`
- Create: `docs/上板调试清单.md`
- Test: all build scripts and CCS Debug build

**Interfaces:**
- Produces: a four-file beginner workflow: `empty.syscfg`, `User/user_config.h`, `User/task.c`, `User/test.c`.

- [ ] **Step 1: Add a failing beginner-documentation check**

Create `Tests/Build/test_beginner_docs.ps1`:

```powershell
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$readme = Get-Content -LiteralPath (Join-Path $project 'README.md') -Raw
foreach ($path in 'empty.syscfg','User/user_config.h','User/task.c','User/test.c') {
    if ($readme -cnotmatch [regex]::Escape($path)) { throw "README must direct beginners to $path" }
}
if ($readme -cmatch 'Components/|Services/|Examples/') { throw 'README must not present retired source folders as daily entry points' }
Write-Output 'BEGINNER DOC CHECK PASSED'
```

Run it. Expected: FAIL against the old README.

- [ ] **Step 2: Rewrite the README first screen and update links**

The first section of `README.md` must state:

```text
比赛时只改四处：empty.syscfg、User/user_config.h、User/task.c、User/test.c。
PA2 不接任何外设；五路寻迹 CH3 经飞线接 PA15；按键临时接 PB2。
```

Update all documents to the new directory names and final pin assignments. `docs/上板调试清单.md` must prescribe this exact order: LCD color test, tracker polarity test, key test, motor wheels lifted, encoder direction, IMU data.

- [ ] **Step 3: Update VS Code and build scripts**

Replace all old `BSP/Inc`, `Components/*`, `Services/Inc`, `App/Inc` include directories with `${workspaceFolder}/Bsp`, `${workspaceFolder}/Hardware`, `${workspaceFolder}/Control`, `${workspaceFolder}/User`. Update `test_vscode_config.ps1` to require exactly these directories. Update `verify_build.ps1` to enumerate only `Bsp`, `Hardware`, `Control`, and `User` C sources plus SysConfig/startup sources.

- [ ] **Step 4: Run all checks and real CCS build**

Run:

```powershell
.\Tests\Host\run_tests.ps1
.\Tests\Build\test_no_host_objects.ps1
.\Tests\Build\test_target_pinout.ps1
.\Tests\Build\test_lcd_api.ps1
.\Tests\Build\test_user_api.ps1
.\Tests\Build\test_beginner_docs.ps1
.\Tests\Build\test_vscode_config.ps1
.\Tests\Build\test_review_feedback.ps1
.\Tests\Build\verify_build.ps1
```

Then build the `Debug` configuration in CCS. Expected: zero compiler errors and no generated source file tracked by Git.

- [ ] **Step 5: Commit documentation, verification and cleanup**

```powershell
git add README.md docs .vscode Tests .cproject .gitignore
git commit -m "docs: document simple competition workflow"
git status --short --branch
```

Expected: clean worktree on the implementation branch.
