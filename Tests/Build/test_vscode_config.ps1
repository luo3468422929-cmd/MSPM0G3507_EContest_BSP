$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$configPath = Join-Path $project '.vscode\c_cpp_properties.json'
$document = Get-Content -Raw -LiteralPath $configPath | ConvertFrom-Json
$configuration = $document.configurations[0]

$requiredIncludePaths = @(
    '${workspaceFolder}'
    '${workspaceFolder}/Debug'
    '${workspaceFolder}/Bsp'
    '${workspaceFolder}/Hardware'
    '${workspaceFolder}/Control'
    '${workspaceFolder}/User'
)

foreach ($path in $requiredIncludePaths) {
    if ($configuration.includePath -notcontains $path) {
        throw "VS Code includePath 缺少：$path"
    }
}

foreach ($retired in @('/App/'; '/BSP/'; '/Components/'; '/Services/')) {
    if (($configuration.includePath -join "`n") -match [regex]::Escape($retired)) {
        throw "VS Code includePath 仍包含旧目录：$retired"
    }
}

foreach ($define in @('__MSPM0G3507__'; '__USE_SYSCONFIG__')) {
    if ($configuration.defines -notcontains $define) {
        throw "VS Code defines 缺少：$define"
    }
}

if (-not (Test-Path -LiteralPath $configuration.compilerPath)) {
    throw "VS Code compilerPath 不存在：$($configuration.compilerPath)"
}

Write-Output 'VSCODE CONFIG CHECK PASSED'
