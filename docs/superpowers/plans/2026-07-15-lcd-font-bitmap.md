# LCD Font and Bitmap Extension Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Add selected 16×16 Chinese glyphs and RGB565 bitmap drawing to the validated ST7735S LCD module without changing its pin, SPI, or initialization behavior.

**Architecture:** Keep `Hardware/lcd.c/.h` as the transport and existing ASCII drawing layer. Add `lcd_font.c/.h` for a const Flash-resident UTF-8-to-glyph table and `lcd_bitmap.c/.h` for bounds-checked RGB565 streaming. Add a board-test path that exercises both new APIs while preserving `TEST_LCD`'s existing color blocks.

**Tech Stack:** MSPM0G3507, TI Driverlib, SysConfig-generated SPI1, TI Arm Clang, PowerShell static checks, existing host/build verification scripts.

## Global Constraints

- Do not modify LCD pins, SPI mode, initialization sequence, `LCD_X_OFFSET`, `LCD_Y_OFFSET`, or `LCD_MADCTL`.
- Do not import STM32 headers, HAL calls, dynamic allocation, PNG/JPEG decoders, or a framebuffer.
- Store glyph and bitmap data in `const` Flash memory.
- Preserve existing `LCD_ShowString`, `LCD_ShowInt`, `LCD_ShowFloat`, and `LCD_Fill` behavior.
- Public invalid arguments return `STATUS_INVALID_PARAM` without starting an SPI transfer.

---

### Task 1: Add the public extension interfaces and static checks

**Files:**
- Create: `Hardware/lcd_font.h`
- Create: `Hardware/lcd_bitmap.h`
- Create: `Tests/Build/test_lcd_font_bitmap_api.ps1`
- Modify: `Tests/Build/verify_build.ps1`

**Interfaces:**
- Produces `LCD_ShowChinese16(uint16_t, uint16_t, const char *, uint16_t, uint16_t)`.
- Produces `LCD_ShowBitmap(uint16_t, uint16_t, uint16_t, uint16_t, const uint8_t *)`.

- [ ] **Step 1: Write the failing static check**

The check must require both headers, both declarations, and no STM32 include in the new implementation names.

- [ ] **Step 2: Run the check and verify it fails**

Run `.TestsBuild	est_lcd_font_bitmap_api.ps1` from the project root. Expected: FAIL because the two headers do not exist.

- [ ] **Step 3: Add the declarations and wire the check into `verify_build.ps1`**

Use `Status_t` and the exact signatures above. Invoke the new script immediately after `test_lcd_api.ps1`.

- [ ] **Step 4: Run the check and verify it passes**

Expected output: `LCD FONT BITMAP API CHECK PASSED`.

### Task 2: Implement the 16×16 Chinese glyph module

**Files:**
- Create: `Hardware/lcd_font.c`
- Modify: `Hardware/lcd.c` only if a private pixel-window helper must be exposed through an internal header.

**Interfaces:**
- Consumes the public LCD pixel-window/write path and `Timer`-independent status conventions.
- Produces `LCD_ShowChinese16` with UTF-8 matching for the selected common competition words.

- [ ] **Step 1: Add a const glyph table**

Store each glyph as 32 bytes in row-major 1-bit format. Include the characters `电`, `赛`, `循`, `迹`, `电`, `机`, `速`, `度`, `角`, `度`, `启`, `动`, `停`, `止`, `正`, `常`, `错`, `误`, `等`, `待`, and `惯` once each, with no writable global buffers.

- [ ] **Step 2: Implement UTF-8 lookup and drawing**

Decode one three-byte UTF-8 code point at a time, find its glyph, open a 16×16 address window, and stream foreground/background RGB565 pixels. Reject `NULL`, malformed UTF-8, unsupported characters, or out-of-bounds coordinates with `STATUS_INVALID_PARAM`.

- [ ] **Step 3: Compile the module with the host syntax checks**

Run `.TestsHostun_tests.ps1` and the API check. Expected: no compile errors and `LCD FONT BITMAP API CHECK PASSED`.

### Task 3: Implement the RGB565 bitmap module

**Files:**
- Create: `Hardware/lcd_bitmap.c`
- Modify: `Hardware/lcd_bitmap.h` only as needed for comments or constants.

**Interfaces:**
- Consumes `LCD_Fill`-compatible bounds and the existing LCD SPI pixel path.
- Produces `LCD_ShowBitmap` with row-major, high-byte-first RGB565 input.

- [ ] **Step 1: Validate all inputs before I/O**

Return `STATUS_INVALID_PARAM` for `NULL`, zero width/height, or any rectangle outside `128×160`.

- [ ] **Step 2: Stream the bitmap**

Set one address window and send exactly `width * height * 2` bytes in source order. Do not allocate a second buffer or call a vendor STM32 function.

- [ ] **Step 3: Run the full target verification**

Run `.TestsBuilderify_build.ps1`. Expected: `BUILD VERIFIED: ...MSPM0G3507_EContest_BSP.out`.

### Task 4: Add board smoke-test coverage and documentation

**Files:**
- Modify: `User/test.c`
- Modify: `docs/模块接口手册.md`
- Modify: `docs/上板调试清单.md`

**Interfaces:**
- Consumes `LCD_ShowChinese16` and `LCD_ShowBitmap`.
- Produces a `TEST_LCD` page that keeps the existing four color blocks and adds a short Chinese status line; bitmap smoke test uses a small const checkerboard, not the full vendor image.

- [ ] **Step 1: Add a small const checkerboard asset in `User/test.c`**

Use a 16×16 RGB565 array and call `LCD_ShowBitmap(96U, 96U, 16U, 16U, asset)` after the current `LCD OK` text.

- [ ] **Step 2: Add the Chinese smoke test**

Call `LCD_ShowChinese16(8U, 96U, "电赛", LCD_COLOR_YELLOW, LCD_COLOR_BLACK)` and preserve the current color blocks.

- [ ] **Step 3: Document usage and resource limits**

Document the two new functions, UTF-8 source encoding, 16×16 size, RGB565 byte order, and the fact that only selected glyphs are built in.

- [ ] **Step 4: Re-run all checks**

Run `.TestsHostun_tests.ps1`, `.TestsBuilderify_build.ps1`, and `git diff --check`. Expected: all existing checks and the new API check pass.

### Task 5: Commit the extension

**Files:** all files from Tasks 1–4 only.

- [ ] **Step 1: Inspect the diff**

Confirm no generated `Debug` files, STM32 headers, or user-local `User/task.c` test selection changes are staged.

- [ ] **Step 2: Commit**

```powershell
git add Hardware/lcd_font.h Hardware/lcd_font.c Hardware/lcd_bitmap.h Hardware/lcd_bitmap.c Tests/Build/test_lcd_font_bitmap_api.ps1 Tests/Build/verify_build.ps1 User/test.c docs/模块接口手册.md docs/上板调试清单.md
git commit -m "feat: add LCD Chinese font and bitmap APIs"
```
