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

Assert-FileNotMatches 'User\task.c' '%(?:[-+ #0]*)(?:\d+|\*)?(?:\.\d+|\.\*)?[F]' `
    'Display format strings must use lowercase %f for TI libc consistency'
Assert-FileNotMatches 'README.md' 'Docs/' `
    'README links must match the lowercase docs directory'
Assert-FileNotMatches 'Control\scheduler.c' 'task\s*<\s*SCHEDULER_TASK_5_MS' `
    'Scheduler must not compare an unsigned enum against its zero-valued first item'
Assert-FileMatches 'empty.syscfg' 'SPI1\.targetBitRate\s*=\s*4000000\s*;' `
    'LCD SPI controller must be configured for 4 MHz'
Assert-FileMatches 'Control\scheduler.h' 'SCHEDULER_TASK_100_MS' `
    'Scheduler must expose the 100 ms display slot'
Assert-FileMatches 'User\task.c' 'Scheduler_IsDue\(SCHEDULER_TASK_100_MS' `
    'LCD status page must rotate one line on the 100 ms slot'
Assert-FileMatches 'Bsp\common.h' 'STATUS_NOT_SUPPORTED\s*,\s*STATUS_DISABLED' `
    'STATUS_DISABLED must be appended without renumbering existing status values'
Assert-FileMatches 'Hardware\motor.c' 'if\s*\(!g_enabled\)[\s\S]*?return\s+STATUS_DISABLED\s*;' `
    'Motor_SetDuty must report disabled state accurately'
Assert-FileNotMatches '.clangd' "Suppress:\s*'\*'" `
    'clangd must not hide every diagnostic'

Write-Output 'CODE REVIEW REGRESSION CHECKS PASSED'
