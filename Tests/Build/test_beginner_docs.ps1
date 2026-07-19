$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$readme = Get-Content -LiteralPath (Join-Path $project 'README.md') -Raw
foreach ($path in 'empty.syscfg','User/user_config.h','User/task.c','User/test.c') {
    if ($readme -cnotmatch [regex]::Escape($path)) {
        throw "README must direct beginners to $path"
    }
}
if ($readme -cmatch 'Components/|Services/|Examples/|App/') {
    throw 'README must not present retired source folders as daily entry points'
}
if ($readme -cnotmatch 'PA28' -or $readme -cnotmatch 'PA31' -or
    $readme -cnotmatch '0x48' -or $readme -cnotmatch '7 字节' -or
    $readme -cnotmatch 'PB2') {
    throw 'README must state the I2C tracker and key wiring'
}
$overviewPath = Join-Path $project 'docs\项目总览与使用指南.md'
if (-not (Test-Path -LiteralPath $overviewPath -PathType Leaf)) {
    throw 'Beginner project overview is missing'
}
$overview = Get-Content -LiteralPath $overviewPath -Raw
foreach ($token in 'STARTUP_TEST','ARMING','CH340','PA0','PA1','verify_build.ps1') {
    if ($overview -cnotmatch [regex]::Escape($token)) {
        throw "Project overview is missing: $token"
    }
}
Write-Output 'BEGINNER DOC CHECK PASSED'
