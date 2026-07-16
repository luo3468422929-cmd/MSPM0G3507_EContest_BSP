$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$syscfg = Get-Content -LiteralPath (Join-Path $project 'empty.syscfg') -Raw
$pins = Get-Content -LiteralPath (Join-Path $project 'Bsp\board_pins.h') -Raw
$encoder = Get-Content -LiteralPath (Join-Path $project 'Hardware\encoder.c') -Raw

if ($syscfg -cmatch 'CAPTURE1|CAPTURE2|ENCODER_LEFT_CAPTURE|ENCODER_RIGHT_CAPTURE') {
    throw 'encoder SysConfig must not contain timer capture instances'
}
foreach ($pattern in @(
    'PIN_ENCODER_LEFT_A_PORT\s+GPIO_A_PORT',
    'PIN_ENCODER_LEFT_A\s+GPIO_A_ENCODER_LEFT_A_PIN',
    'PIN_ENCODER_LEFT_B_PORT\s+GPIO_A_PORT',
    'PIN_ENCODER_LEFT_B\s+GPIO_A_ENCODER_LEFT_B_PIN',
    'PIN_ENCODER_RIGHT_A_PORT\s+GPIO_B_PORT',
    'PIN_ENCODER_RIGHT_A\s+GPIO_B_ENCODER_RIGHT_A_PIN',
    'PIN_ENCODER_RIGHT_B_PORT\s+GPIO_B_PORT',
    'PIN_ENCODER_RIGHT_B\s+GPIO_B_ENCODER_RIGHT_B_PIN')) {
    if ($pins -cnotmatch $pattern) {
        throw "board_pins.h missing GPIO encoder mapping: $pattern"
    }
}
if ($encoder -cnotmatch 'void\s+GROUP1_IRQHandler\s*\(void\)') {
    throw 'encoder GPIO group interrupt handler is missing'
}
foreach ($pattern in @(
    'DL_GPIO_getEnabledInterruptStatus',
    'DL_GPIO_clearInterruptStatus',
    'Encoder_HandleLeftARising',
    'Encoder_HandleLeftBRising',
    'Encoder_HandleRightARising',
    'Encoder_HandleRightBRising')) {
    if ($encoder -cnotmatch [regex]::Escape($pattern)) {
        throw "encoder GPIO IRQ handling is missing: $pattern"
    }
}
if ($encoder -cmatch 'void\s+TIMA1_IRQHandler|void\s+TIMG8_IRQHandler') {
    throw 'timer capture IRQ handlers must be removed'
}

Write-Output 'ENCODER GPIO IRQ CHECK PASSED'
