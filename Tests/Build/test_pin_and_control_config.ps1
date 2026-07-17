$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$syscfg = Get-Content -LiteralPath (Join-Path $project 'empty.syscfg') -Raw
$pins = Get-Content -LiteralPath (Join-Path $project 'Bsp\board_pins.h') -Raw
$lcd = Get-Content -LiteralPath (Join-Path $project 'Hardware\lcd.c') -Raw
$config = Get-Content -LiteralPath (Join-Path $project 'User\user_config.h') -Raw
$car = Get-Content -LiteralPath (Join-Path $project 'Control\car_control.c') -Raw
$track = Get-Content -LiteralPath (Join-Path $project 'Hardware\track.c') -Raw

foreach ($forbiddenPin in @('PA24', 'PA0', 'PA1')) {
    if ($syscfg -match ('["'']' + [regex]::Escape($forbiddenPin) + '["'']')) {
        throw "$forbiddenPin must remain unconfigured in SysConfig"
    }
}
if ($pins -match 'PIN_LCD_BL' -or $lcd -match 'PIN_LCD_BL') {
    throw 'LCD BL is hardwired to 3.3 V and must not consume an MCU pin'
}
if ($lcd -notmatch '\(void\)on' -or
    $lcd -notmatch 'BL.*3\.3') {
    throw 'LCD_SetBacklight must remain as a documented compatibility no-op'
}

$requiredMacros = @(
    'SPEED_PID_LEFT_KP', 'SPEED_PID_RIGHT_KP', 'STEERING_PID_KP',
    'STEERING_OUTPUT_LIMIT', 'CAR_MAX_TARGET_RPM',
    'CAR_ALLOW_REVERSE_WHEEL', 'TRACK_SENSOR_REVERSED',
    'TRACK_ALL_ACTIVE_IS_LINE'
)
foreach ($macro in $requiredMacros) {
    if ($config -notmatch ('#define\s+' + $macro + '\b')) {
        throw "Central control configuration is missing $macro"
    }
}
if ($car -match '\.kp\s*=\s*6\.0f' -or
    $car -match '\.kp\s*=\s*20\.0f') {
    throw 'PID gains must come from user_config.h instead of literals'
}
if ($car -notmatch 'CAR_MAX_TARGET_RPM' -or
    $car -notmatch 'CAR_ALLOW_REVERSE_WHEEL') {
    throw 'Wheel targets must use the configured speed/reverse limits'
}
if ($track -notmatch 'TRACK_SENSOR_REVERSED' -or
    $track -notmatch 'TRACK_ALL_ACTIVE_IS_LINE') {
    throw 'Track orientation and all-active policy must be configurable'
}

Write-Output 'PIN AND CONTROL CONFIG CHECK PASSED'
