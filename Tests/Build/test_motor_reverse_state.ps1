$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$motor = Get-Content -LiteralPath (Join-Path $project 'Hardware\motor.c') -Raw

if ($motor -notmatch 'int16_t\s+logicalDuty\s*=\s*duty') {
    throw 'Motor_Apply must preserve the logical duty before direction inversion'
}
if ($motor -notmatch 'g_motor\[id\]\.appliedDuty\s*=\s*logicalDuty') {
    throw 'Motor_Apply must store logical duty for ramp state'
}
Write-Output 'MOTOR REVERSE RAMP STATE CHECK PASSED'
