$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$testHeader = Get-Content -LiteralPath (Join-Path $project 'User\test.h') -Raw
$testSource = Get-Content -LiteralPath (Join-Path $project 'User\test.c') -Raw
$carHeader = Get-Content -LiteralPath (Join-Path $project 'Control\car_control.h') -Raw
$carSource = Get-Content -LiteralPath (Join-Path $project 'Control\car_control.c') -Raw
$config = Get-Content -LiteralPath (Join-Path $project 'User\user_config.h') -Raw

if ($testHeader -notmatch '\bTEST_PID\b') { throw 'TEST_PID is missing' }
if ($testSource -notmatch '\bTest_RunPid\b' -or
    $testSource -notmatch '\bCarControl_UpdateSpeedTest\b') {
    throw 'TEST_PID must call the standalone speed-control update'
}
if ($testSource -notmatch 'pid:%\.2f,') {
    throw 'TEST_PID must emit a FireWater CSV frame'
}
if ($carHeader -notmatch '\bCarControl_UpdateSpeedTest\b' -or
    $carSource -notmatch '\bCarControl_UpdateSpeedTest\b') {
    throw 'CarControl speed-test API is missing'
}
if ($config -notmatch '#define\s+PID_TEST_TARGET_RPM\s+40\.0f') {
    throw 'PID test target must default to 40 RPM'
}
if ($config -notmatch '#define\s+PID_TEST_VOFA_ENABLE\s+1') {
    throw 'VOFA telemetry must be enabled for PID testing'
}
Write-Output 'PID TEST MODE CHECK PASSED'
