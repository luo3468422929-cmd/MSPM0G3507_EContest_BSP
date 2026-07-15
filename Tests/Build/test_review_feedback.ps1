$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

function Assert-FileMatches {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $path = Join-Path $project $RelativePath
    $content = Get-Content -LiteralPath $path -Raw
    if ($content -cnotmatch $Pattern) {
        throw $Message
    }
}

function Assert-FileNotMatches {
    param(
        [Parameter(Mandatory = $true)][string]$RelativePath,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Message
    )

    $path = Join-Path $project $RelativePath
    $content = Get-Content -LiteralPath $path -Raw
    if ($content -cmatch $Pattern) {
        throw $Message
    }
}

Assert-FileNotMatches 'App\Src\app_main.c' '%(?:[-+ #0]*)(?:\d+|\*)?(?:\.\d+|\.\*)?[F]' `
    'App display format strings must use lowercase %f for TI libc consistency'
Assert-FileNotMatches 'README.md' 'Docs/' `
    'README links must match the lowercase docs directory'
Assert-FileNotMatches 'Services\Src\scheduler.c' 'task\s*<\s*SCHEDULER_TASK_5_MS' `
    'Scheduler must not compare an unsigned enum against its zero-valued first item'
Assert-FileMatches 'empty.syscfg' 'I2C1\.basicControllerBusSpeed\s*=\s*400000\s*;' `
    'OLED I2C controller must be configured for 400 kHz'
Assert-FileMatches 'Services\Inc\scheduler.h' 'SCHEDULER_TASK_200_MS' `
    'Scheduler must expose the 200 ms display task'
Assert-FileMatches 'App\Src\app_main.c' 'Scheduler_IsDue\(SCHEDULER_TASK_200_MS' `
    'OLED refresh must run on the 200 ms task'
Assert-FileMatches 'BSP\Inc\common.h' 'STATUS_NOT_SUPPORTED\s*,\s*STATUS_DISABLED' `
    'STATUS_DISABLED must be appended without renumbering existing status values'
Assert-FileMatches 'BSP\Src\motor.c' 'if\s*\(!g_enabled\)[\s\S]*?return\s+STATUS_DISABLED\s*;' `
    'Motor_SetDuty must report disabled state accurately'

Write-Output 'CODE REVIEW REGRESSION CHECKS PASSED'
