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
    $readme -cnotmatch '0x12' -or $readme -cnotmatch '0x30' -or
    $readme -cnotmatch 'PB2') {
    throw 'README must state the I2C tracker and key wiring'
}
Write-Output 'BEGINNER DOC CHECK PASSED'
