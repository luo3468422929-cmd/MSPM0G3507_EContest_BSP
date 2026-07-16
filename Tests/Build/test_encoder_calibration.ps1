$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$config = Get-Content -LiteralPath (Join-Path $project 'User\user_config.h') -Raw
$encoder = Get-Content -LiteralPath (Join-Path $project 'Hardware\encoder.c') -Raw

if ($config -notmatch '#define\s+ENCODER_PULSES_PER_MOTOR_REV\s+11\.0f') {
    throw 'encoder calibration must use the manufacturer value of 11 pulses per motor revolution'
}
if ($config -notmatch '#define\s+ENCODER_COUNT_MULTIPLIER\s+2\.0f') {
    throw 'encoder calibration must keep x2 decoding'
}
if ($config -notmatch '#define\s+ENCODER_COUNTS_PER_WHEEL_REV\s+450\.0f') {
    throw 'encoder calibration must use the measured 450 counts per output-shaft revolution'
}
if ($encoder -notmatch 'float\s+countsPerWheelRev\s*=\s*ENCODER_COUNTS_PER_WHEEL_REV') {
    throw 'encoder speed calculation must use the direct calibrated count-per-wheel value'
}
if ($encoder -match 'ENCODER_PULSES_PER_MOTOR_REV\s*\*') {
    throw 'encoder speed calculation must not reconstruct counts from the old assumed gear ratio'
}

Write-Output 'ENCODER CALIBRATION CHECK PASSED'
