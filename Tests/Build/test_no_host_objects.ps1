$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$hostTestDir = Join-Path $project 'Tests\Host'
$unexpectedObjects = Get-ChildItem -LiteralPath $hostTestDir -File |
    Where-Object { $_.Extension -in '.o', '.obj', '.exe', '.pdb' }

if ($unexpectedObjects.Count -ne 0) {
    $names = $unexpectedObjects.Name -join ', '
    throw "源码目录不能遗留本机编译产物：$names"
}

Write-Output 'HOST TEST OBJECT CHECK PASSED'
