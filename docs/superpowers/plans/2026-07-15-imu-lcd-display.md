
# NCU 惯导 LCD 显示实施计划

> **For agentic workers:** 本计划按现有工程结构执行，使用测试先行和一次最小改动原则。

**Goal:** 在 TEST_IMU 测试模式下，将现有协议已经解析出的 yaw、gyroZ 和在线/等待状态显示到 ST7735S LCD。

**Architecture:** 不修改 NCU UART 接收层和协议解析层，只在 User/test.c 的 Test_RunImu() 中增加显示调用。IMU_Process() 继续负责收包，IMU_GetSample() 提供数据，LCD_* 接口负责显示；串口调试输出保留。

**Tech Stack:** MSPM0G3507、TI DriverLib、CCS、SysConfig、C11、PowerShell 静态构建检查。

## Global Constraints

- 只使用 ImuSample_t 中已有的 yawDeg 和 gyroZDps 字段。
- 不新增 pitch、roll、gyroX、gyroY。
- 不修改 UART2 惯导引脚和协议解析。
- LCD 显示刷新周期保持 100 ms。
- 无有效惯导数据时显示 IMU WAIT。
- 保留现有调试串口输出。
- 不在 LCD 显示逻辑中直接调用 DriverLib。

---

### Task 1: 添加惯导 LCD 显示静态测试

**Files:**
- Create: Tests/Build/test_imu_lcd_display.ps1
- Modify: Tests/Build/verify_build.ps1

**Interfaces:**
- Consumes: User/test.c 中的 Test_RunImu() 源码。
- Produces: 检查 yaw、gyroZ、WAIT 文本和 LCD 显示 API 都存在。

- [ ] **Step 1: 写一个会失败的静态测试**

测试应读取 User/test.c，提取 Test_RunImu() 到 Test_RunLcd() 之间的函数文本，并检查：

~~~powershell
$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$source = Get-Content -LiteralPath (Join-Path $project 'User\test.c') -Raw
$match = [regex]::Match(
    $source,
    'static void Test_RunImu\(uint32_t nowMs\)(.*?)static void Test_RunLcd',
    [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $match.Success) { throw 'Test_RunImu not found' }
$function = $match.Groups[1].Value
foreach ($token in 'LCD_ShowString','LCD_ShowFloat','sample.yawDeg','sample.gyroZDps','IMU WAIT') {
    if ($function -notmatch [regex]::Escape($token)) {
        throw "IMU LCD display missing: $token"
    }
}
Write-Output 'IMU LCD DISPLAY CHECK PASSED'
~~~

- [ ] **Step 2: 运行测试并确认它因功能尚未存在而失败**

运行：

~~~powershell
.\Tests\Build\test_imu_lcd_display.ps1
~~~

预期：失败，并提示缺少 LCD_ShowString 或 LCD_ShowFloat。

- [ ] **Step 3: 将测试接入完整验证入口**

在 Tests/Build/verify_build.ps1 的已有静态检查区域增加：

~~~powershell
& (Join-Path $PSScriptRoot 'test_imu_lcd_display.ps1')
~~~

- [ ] **Step 4: 提交测试脚本**

~~~text
git add Tests/Build/test_imu_lcd_display.ps1 Tests/Build/verify_build.ps1
git commit -m "test: require IMU LCD display output"
~~~

### Task 2: 在 TEST_IMU 中实现 LCD 显示

**Files:**
- Modify: User/test.c 中的 Test_RunImu()

**Interfaces:**
- Consumes: IMU_Process(uint32_t)、IMU_GetSample(ImuSample_t *)、IMU_IsOnline(uint32_t)、LCD_ShowString() 和 LCD_ShowFloat()。
- Produces: 每 100 ms 更新 LCD 上的 YAW、GYRO Z、ONLINE/OFFLINE 或 IMU WAIT。

- [ ] **Step 1: 在有效数据分支显示 yaw 和 gyroZ**

在 IMU_GetSample() 返回 STATUS_OK 后，保留 UART_Printf，并增加以下显示逻辑：

~~~c
(void)LCD_ShowString(0U, 0U, "IMU TEST       ",
                     LCD_COLOR_WHITE, LCD_COLOR_BLACK);
(void)LCD_ShowString(0U, 24U, "YAW:",
                     LCD_COLOR_WHITE, LCD_COLOR_BLACK);
(void)LCD_ShowFloat(36U, 24U, sample.yawDeg, 1U,
                   LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
(void)LCD_ShowString(0U, 48U, "GYRO Z:",
                     LCD_COLOR_WHITE, LCD_COLOR_BLACK);
(void)LCD_ShowFloat(48U, 48U, sample.gyroZDps, 1U,
                   LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
(void)LCD_ShowString(0U, 72U,
                     IMU_IsOnline(nowMs) ? "ONLINE " : "OFFLINE",
                     LCD_COLOR_GREEN, LCD_COLOR_BLACK);
~~~

- [ ] **Step 2: 在无数据分支显示等待状态**

在 IMU_GetSample() 返回非 STATUS_OK 时，保留 UART_SendString()，并增加：

~~~c
(void)LCD_ShowString(0U, 0U, "IMU TEST       ",
                     LCD_COLOR_WHITE, LCD_COLOR_BLACK);
(void)LCD_ShowString(0U, 24U, "IMU WAIT       ",
                     LCD_COLOR_YELLOW, LCD_COLOR_BLACK);
~~~

- [ ] **Step 3: 运行静态测试**

运行：

~~~powershell
.\Tests\Build\test_imu_lcd_display.ps1
~~~

预期：输出 IMU LCD DISPLAY CHECK PASSED。

- [ ] **Step 4: 编译并运行完整验证**

运行：

~~~powershell
.\Tests\Host\run_tests.ps1
.\Tests\Build\verify_build.ps1
~~~

预期：主机测试通过、LCD API 检查通过、IMU LCD DISPLAY CHECK PASSED，并生成 BUILD VERIFIED。

- [ ] **Step 5: 提交实现**

~~~text
git add User/test.c
git commit -m "feat: show IMU yaw and gyro on LCD"
~~~

### Task 3: 上板验证

**Files:**
- Modify only when selecting test: User/task.c

**Interfaces:**
- Consumes: 已编译的 TEST_IMU 测试模式。
- Produces: LCD 显示实时惯导数据。

- [ ] **Step 1: 选择 TEST_IMU**

在 User/task.c 中设置：

~~~c
Test_Select(TEST_IMU);
~~~

- [ ] **Step 2: 确认惯导接线**

~~~text
NCU TX → PA22 / UART2 RX
NCU RX → PA21 / UART2 TX
NCU GND → 主控 GND
~~~

- [ ] **Step 3: 下载并观察 LCD**

预期显示：

~~~text
IMU TEST
YAW:    xxx.x
GYRO Z: xxx.x
ONLINE
~~~

无数据时显示：

~~~text
IMU WAIT
~~~

- [ ] **Step 4: 同时观察调试串口**

串口继续输出：

~~~text
IMU yaw=xxx.x gyroZ=xxx.x ONLINE
~~~

- [ ] **Step 5: 上板通过后恢复正常模式**

~~~c
Test_Select(TEST_NONE);
~~~

