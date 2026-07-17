$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$config = Get-Content -LiteralPath (Join-Path $project 'User\user_config.h') -Raw
$task = Get-Content -LiteralPath (Join-Path $project 'User\task.c') -Raw
$testHeader = Get-Content -LiteralPath (Join-Path $project 'User\test.h') -Raw
$testSource = Get-Content -LiteralPath (Join-Path $project 'User\test.c') -Raw
$carSource = Get-Content -LiteralPath (Join-Path $project 'Control\car_control.c') -Raw

if ($config -notmatch '#define\s+STARTUP_TEST\s+TEST_NONE') {
    throw 'Startup test selection must be centralized in user_config.h'
}
if ($config -notmatch '#define\s+AUTO_START_DELAY_MS\s+1000U' -or
    $config -notmatch '#define\s+AUTO_START_VALID_FRAMES\s+5U') {
    throw 'Safe automatic startup delay/readiness defaults are missing'
}
if ($task -notmatch '\bTASK_ARMING\b') {
    throw 'Task state machine must have an arming/readiness state'
}
if ($task -notmatch 'Test_Select\(STARTUP_TEST\)') {
    throw 'Task_Init must use the centralized startup test selection'
}
if ($testHeader -notmatch 'Test_Run\(uint32_t\s+nowMs,\s*Key_Event_t\s+event\)' -or
    $testHeader -notmatch '\bTest_UsesMotor\b') {
    throw 'Tests must receive the single task-level key event and expose motor usage'
}
if ($testSource -match '\bKey_Scan\s*\(' -or
    $testSource -match '\bKey_GetEvent\s*\(') {
    throw 'Test module must not scan/read the key independently'
}
if ($task -notmatch 'KEY_EVENT_PRESSED' -or
    $task -notmatch 'Key_IsPressed\(\)' -or
    $task -notmatch 'Test_UsesMotor' -or
    $task -notmatch 'Board_EmergencyStop\(\)') {
    throw 'Running and motor-test modes must check both key events and held level'
}
if ($task -notmatch 'TaskSafety_ShouldStop' -or
    $task -notmatch 'frameReady\s*=\s*!Key_IsPressed\(\)') {
    throw 'Arming must reject a key held before reset and use tested safety logic'
}
if ($testSource -notmatch 'case\s+TEST_MOTOR:[\s\S]*?CONFIG_KEY_ENABLE') {
    throw 'Motor test must require the emergency-stop key module'
}
if ($config -notmatch '#define\s+MOTOR_TEST_RUN_MS\s+5000U') {
    throw 'Open-loop motor test must default to a short five-second direction interval'
}
if ($carSource -match 'g_data\.baseSpeedRpm\s*=\s*\(targetLeftRpm') {
    throw 'PID test must not overwrite the production base-speed setting'
}

Write-Output 'SAFETY STATE CHECK PASSED'
