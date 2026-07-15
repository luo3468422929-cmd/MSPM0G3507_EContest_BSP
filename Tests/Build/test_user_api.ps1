$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$taskPath = Join-Path $project 'User\task.c'
$mainPath = Join-Path $project 'User\main.c'
if (-not (Test-Path -LiteralPath $taskPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $mainPath -PathType Leaf)) {
    throw 'User/main.c and User/task.c are required'
}
$userSource = (Get-Content -LiteralPath $taskPath -Raw) +
              (Get-Content -LiteralPath $mainPath -Raw)
foreach ($name in 'System_Init','Task_Init','Task_Run','CarControl_Update') {
    if ($userSource -cnotmatch "\b$name\b") { throw "User API missing $name" }
}
if ($userSource -cmatch '\bDL_[A-Za-z0-9_]+') {
    throw 'User layer must not call DriverLib directly'
}
Write-Output 'USER API CHECK PASSED'
