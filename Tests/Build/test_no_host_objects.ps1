$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$hostTestDir = Join-Path $project 'Tests\Host'
$unexpectedObjects = Get-ChildItem -LiteralPath $hostTestDir -File -Filter '*.o'

if ($unexpectedObjects.Count -ne 0) {
    $names = $unexpectedObjects.Name -join ', '
    throw "Tests/Host 根目录不能存在链接对象文件：$names"
}

Write-Output 'HOST TEST OBJECT CHECK PASSED'
