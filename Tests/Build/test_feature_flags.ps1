$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$config = Get-Content -LiteralPath (Join-Path $project 'User\user_config.h') -Raw
$task = Get-Content -LiteralPath (Join-Path $project 'User\task.c') -Raw
$imuHeader = Get-Content -LiteralPath (Join-Path $project 'Hardware\imu.h') -Raw
$imuSource = Get-Content -LiteralPath (Join-Path $project 'Hardware\imu.c') -Raw
$timer = Get-Content -LiteralPath (Join-Path $project 'Bsp\timer.c') -Raw
$lcdFont = Get-Content -LiteralPath (Join-Path $project 'Hardware\lcd_font.c') -Raw

if ($config -notmatch '#define\s+CONFIG_CAR_CONTROL_ENABLE\s+1') {
    throw 'Car control requires its own feature switch'
}
if ($config -notmatch '#if\s+CONFIG_CAR_CONTROL_ENABLE' -or
    $config -notmatch '#error\s+"Car control requires motor, encoder, track and key"') {
    throw 'Feature dependency checks are missing from user_config.h'
}
foreach ($flag in @('CONFIG_KEY_ENABLE', 'CONFIG_CAR_CONTROL_ENABLE',
                     'CONFIG_TRACK_ENABLE', 'CONFIG_IMU_ENABLE',
                     'CONFIG_UART_ENABLE', 'CONFIG_LCD_ENABLE')) {
    if ($task -notmatch ('#if\s+' + $flag)) {
        throw "Task consumers are not controlled by $flag"
    }
}
if ($task -notmatch 'SCHEDULER_TASK_100_MS' -or
    $task -notmatch 'g_displayLine') {
    throw 'Normal LCD page must refresh one rotating line per 100 ms slot'
}
if ($imuHeader -notmatch '\bIMU_PeekSample\b' -or
    $imuSource -notmatch '\bIMU_PeekSample\b' -or
    $task -notmatch 'IMU_PeekSample') {
    throw 'Status display must inspect IMU data without consuming its new-data flag'
}
if ($timer -notmatch 'SysTick_Config[^;]*!=\s*0U') {
    throw 'Timer_Init must check the SysTick_Config return value'
}
if ($lcdFont -notmatch "text\[1\]\s*==\s*'\\0'" -or
    $lcdFont -notmatch "text\[2\]\s*==\s*'\\0'") {
    throw 'UTF-8 decoder must reject truncated sequences before reading their tail'
}

Write-Output 'FEATURE FLAG AND LIGHTWEIGHT DISPLAY CHECK PASSED'
