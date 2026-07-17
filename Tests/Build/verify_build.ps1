$ErrorActionPreference = 'Stop'

& (Join-Path $PSScriptRoot 'test_target_pinout.ps1')
& (Join-Path $PSScriptRoot 'test_encoder_gpio_irq.ps1')
& (Join-Path $PSScriptRoot 'test_encoder_x2_irq.ps1')
& (Join-Path $PSScriptRoot 'test_encoder_calibration.ps1')
& (Join-Path $PSScriptRoot 'test_motor_reverse_state.ps1')
& (Join-Path $PSScriptRoot 'test_pid_mode.ps1')
& (Join-Path $PSScriptRoot 'test_eight_track_i2c.ps1')
& (Join-Path $PSScriptRoot 'test_lcd_api.ps1')
& (Join-Path $PSScriptRoot 'test_lcd_font_bitmap_api.ps1')
& (Join-Path $PSScriptRoot 'test_imu_lcd_display.ps1')
& (Join-Path $PSScriptRoot 'test_user_api.ps1')
& (Join-Path $PSScriptRoot 'test_beginner_docs.ps1')
& (Join-Path $PSScriptRoot 'test_safety_state.ps1')
& (Join-Path $PSScriptRoot 'test_runtime_hardening.ps1')
& (Join-Path $PSScriptRoot 'test_pin_and_control_config.ps1')
& (Join-Path $PSScriptRoot 'test_feature_flags.ps1')
& (Join-Path $PSScriptRoot 'test_no_host_objects.ps1')
& (Join-Path $PSScriptRoot 'test_review_feedback.ps1')
& (Join-Path $PSScriptRoot 'test_vscode_config.ps1')
& (Join-Path (Split-Path -Parent $PSScriptRoot) 'Host\run_tests.ps1')

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$sdk = if (-not [string]::IsNullOrWhiteSpace($env:MSPM0_SDK_ROOT)) {
    $env:MSPM0_SDK_ROOT
} else {
    'C:\TI\mspm0_sdk_2_10_00_04'
}
$sysconfig = if (-not [string]::IsNullOrWhiteSpace($env:SYSCONFIG_CLI)) {
    $env:SYSCONFIG_CLI
} else {
    'C:\TI\sysconfig_1.26.2\sysconfig_cli.bat'
}
if (-not (Test-Path -LiteralPath (Join-Path $sdk '.metadata\product.json'))) {
    throw "MSPM0 SDK not found. Set MSPM0_SDK_ROOT: $sdk"
}
if (-not (Test-Path -LiteralPath $sysconfig)) {
    throw "SysConfig CLI not found. Set SYSCONFIG_CLI: $sysconfig"
}
$vscodeConfig = Get-Content -LiteralPath `
    (Join-Path $project '.vscode\c_cpp_properties.json') -Raw |
    ConvertFrom-Json
$configuredCompiler = $vscodeConfig.configurations[0].compilerPath
$compilerCandidates = @()
if (-not [string]::IsNullOrWhiteSpace($env:TI_ARM_CLANG)) {
    $compilerCandidates += $env:TI_ARM_CLANG
}
$compilerCandidates += @(
    $configuredCompiler,
    'C:\TI\ti_cgt_arm_llvm_4.0.2.LTS\bin\tiarmclang.exe')
$compiler = $compilerCandidates |
    Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1
if ([string]::IsNullOrWhiteSpace($compiler)) {
    throw 'TI Arm Clang was not found in VS Code configuration or fallback path'
}
$compilerRoot = Split-Path -Parent (Split-Path -Parent $compiler)
Write-Output "TI COMPILER: $compiler"
$output = Join-Path $project 'Debug\full_verify'
$syscfgOutput = Join-Path $project 'Debug\syscfg'

New-Item -ItemType Directory -Path $output -Force | Out-Null

$sysconfigArgs = @(
    '--product'; (Join-Path $sdk '.metadata\product.json')
    '--compiler'; 'ticlang'
    '--script'; (Join-Path $project 'empty.syscfg')
    '--output'; $syscfgOutput
    '--treatWarningsAsErrors'
)
& $sysconfig @sysconfigArgs
if ($LASTEXITCODE -ne 0) { throw "SysConfig failed: $LASTEXITCODE" }

$includeDirs = @(
    $syscfgOutput
    (Join-Path $project 'Bsp')
    (Join-Path $project 'Hardware')
    (Join-Path $project 'Control')
    (Join-Path $project 'User')
    (Join-Path $sdk 'source')
    (Join-Path $sdk 'source\third_party\CMSIS\Core\Include')
)
$includeArgs = @()
foreach ($dir in $includeDirs) { $includeArgs += ('-I' + $dir) }

$sourceFiles = @(
    (Join-Path $syscfgOutput 'ti_msp_dl_config.c')
    (Join-Path $sdk 'source\ti\devices\msp\m0p\startup_system_files\ticlang\startup_mspm0g350x_ticlang.c')
) + (Get-ChildItem -LiteralPath @(
    (Join-Path $project 'Bsp')
    (Join-Path $project 'Hardware')
    (Join-Path $project 'Control')
    (Join-Path $project 'User')
) -Recurse -Filter '*.c' | ForEach-Object FullName)

$objects = @()
$sourceIndex = 0
foreach ($source in $sourceFiles) {
    $object = Join-Path $output ('source_' + $sourceIndex + '.o')
    $compileArgs = @(
        ('@' + (Join-Path $syscfgOutput 'device.opt'))
        '-mcpu=cortex-m0plus'
        '-march=thumbv6m'
        '-mfloat-abi=soft'
        '-mthumb'
        '-std=c11'
        '-O2'
        '-gdwarf-3'
        '-Wall'
        '-Wextra'
        '-Werror'
        '-c'
    ) + $includeArgs + @($source; '-o'; $object)
    & $compiler @compileArgs
    if ($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    $objects += $object
    $sourceIndex++
}

$firmware = Join-Path $output 'MSPM0G3507_EContest_BSP.out'
$map = Join-Path $output 'MSPM0G3507_EContest_BSP.map'
$linkArgs = @('-Wl,-u,_c_int00') + $objects + @(
    '-ldevice.cmd.genlibs'
    ('-L' + (Join-Path $sdk 'source'))
    ('-L' + $syscfgOutput)
    (Join-Path $syscfgOutput 'device_linker.cmd')
    ('-Wl,-m,' + $map)
    '-Wl,--rom_model'
    '-Wl,--warn_sections'
    ('-L' + (Join-Path $compilerRoot 'lib'))
    '-llibc.a'
    '-o'
    $firmware
)
& $compiler @linkArgs
if ($LASTEXITCODE -ne 0) { throw "Link failed: $LASTEXITCODE" }

# 当前应用会用 snprintf 格式化浮点数；map 中必须包含 TI libc 的浮点转换实现。
if (-not (Select-String -LiteralPath $map -Pattern '.text._pconv_f' -SimpleMatch -Quiet)) {
    throw 'TI libc floating-point printf support was not linked'
}

Write-Output "BUILD VERIFIED: $firmware"
