$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ringHeader = Get-Content -LiteralPath (Join-Path $project 'Bsp\ring_buffer.h') -Raw
$ringSource = Get-Content -LiteralPath (Join-Path $project 'Bsp\ring_buffer.c') -Raw
$uartSource = Get-Content -LiteralPath (Join-Path $project 'Bsp\uart.c') -Raw
$i2cSource = Get-Content -LiteralPath (Join-Path $project 'Bsp\i2c.c') -Raw
$scheduler = Get-Content -LiteralPath (Join-Path $project 'Control\scheduler.c') -Raw
$config = Get-Content -LiteralPath (Join-Path $project 'User\user_config.h') -Raw

if ($ringHeader -match 'volatile\s+uint16_t\s+count') {
    throw 'Ring buffer must not share a mutable count between ISR and main context'
}
if ($ringSource -notmatch 'headSnapshot\s*-\s*tailSnapshot' -or
    $ringSource -notmatch 'buffer->head\s*=\s*\(uint16_t\)\(head\s*\+\s*1U\)') {
    throw 'Ring buffer must use monotonic SPSC head/tail counters'
}
if ($uartSource -notmatch 'DL_UART_Main_isTXFIFOFull' -or
    $uartSource -notmatch 'while\s*\(!DL_UART_Main_isRXFIFOEmpty') {
    throw 'UART must wait for TX FIFO space and drain RX FIFO in one IRQ'
}
if ($i2cSource -notmatch 'DL_I2C_resetControllerTransfer' -or
    $i2cSource -notmatch 'I2C_RECOVERY_RETRY_COUNT') {
    throw 'I2C transfer must reset and retry after timeout/NACK'
}
if ($config -notmatch '#define\s+I2C_RECOVERY_RETRY_COUNT\s+1U') {
    throw 'I2C recovery retry count must be configurable and default to one'
}
if ($scheduler -notmatch 'g_lastRunMs\[task\]\s*=\s*nowMs' -or
    $scheduler -match 'g_lastRunMs\[task\]\s*\+=') {
    throw 'Scheduler must skip missed slots instead of burst catch-up'
}

Write-Output 'RUNTIME HARDENING CHECK PASSED'
