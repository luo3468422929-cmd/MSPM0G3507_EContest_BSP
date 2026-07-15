$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$sdk = 'C:\TI\mspm0_sdk_2_10_00_04'
$sysconfig = 'C:\TI\sysconfig_1.26.2\sysconfig_cli.bat'
$compilerRoot = 'C:\TI\ti_cgt_arm_llvm_4.0.2.LTS'
$compiler = Join-Path $compilerRoot 'bin\tiarmclang.exe'
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
    (Join-Path $project 'BSP\Config')
    (Join-Path $project 'BSP\Inc')
    (Join-Path $project 'Components\PID')
    (Join-Path $project 'Components\Filter')
    (Join-Path $project 'Components\RingBuffer')
    (Join-Path $project 'Components\Protocol')
    (Join-Path $project 'Components\Track')
    (Join-Path $project 'Components\SSD1306')
    (Join-Path $project 'Services\Inc')
    (Join-Path $project 'App\Inc')
    (Join-Path $project 'Examples')
    (Join-Path $sdk 'source')
    (Join-Path $sdk 'source\third_party\CMSIS\Core\Include')
)
$includeArgs = @()
foreach ($dir in $includeDirs) { $includeArgs += ('-I' + $dir) }

$sourceFiles = @(
    (Join-Path $project 'empty.c')
    (Join-Path $syscfgOutput 'ti_msp_dl_config.c')
    (Join-Path $sdk 'source\ti\devices\msp\m0p\startup_system_files\ticlang\startup_mspm0g350x_ticlang.c')
) + (Get-ChildItem -LiteralPath @(
    (Join-Path $project 'BSP\Src')
    (Join-Path $project 'Components')
    (Join-Path $project 'Services\Src')
    (Join-Path $project 'App\Src')
    (Join-Path $project 'Examples')
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
