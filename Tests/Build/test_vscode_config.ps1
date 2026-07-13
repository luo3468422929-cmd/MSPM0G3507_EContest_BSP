$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$configPath = Join-Path $project '.vscode\c_cpp_properties.json'
$document = Get-Content -Raw -LiteralPath $configPath | ConvertFrom-Json
$configuration = $document.configurations[0]

$requiredIncludePaths = @(
    '${workspaceFolder}'
    '${workspaceFolder}/Debug'
    '${workspaceFolder}/App/Inc'
    '${workspaceFolder}/BSP/Config'
    '${workspaceFolder}/BSP/Inc'
    '${workspaceFolder}/Components/Filter'
    '${workspaceFolder}/Components/PID'
    '${workspaceFolder}/Components/Protocol'
    '${workspaceFolder}/Components/RingBuffer'
    '${workspaceFolder}/Components/SSD1306'
    '${workspaceFolder}/Components/Track'
    '${workspaceFolder}/Services/Inc'
)

foreach ($path in $requiredIncludePaths) {
    if ($configuration.includePath -notcontains $path) {
        throw "VS Code includePath 缺少：$path"
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
