$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$build = Join-Path ([System.IO.Path]::GetTempPath()) 'mspm0g3507-bsp-host-tests'
New-Item -ItemType Directory -Path $build -Force | Out-Null

$sources = @(
    (Join-Path $PSScriptRoot 'test_main.c')
    (Join-Path $project 'Control\pid.c')
    (Join-Path $project 'Hardware\filter_average.c')
    (Join-Path $project 'Bsp\ring_buffer.c')
    (Join-Path $project 'Hardware\imu_protocol.c')
    (Join-Path $project 'Bsp\frame_protocol.c')
    (Join-Path $project 'Hardware\track_math.c')
    (Join-Path $project 'Hardware\encoder_speed_window.c')
    (Join-Path $project 'Control\scheduler.c')
)
$includeDirs = @(
    (Join-Path $project 'Bsp')
    (Join-Path $project 'Hardware')
    (Join-Path $project 'Control')
    (Join-Path $project 'User')
)
$includeArgs = @()
foreach ($dir in $includeDirs) { $includeArgs += ('-I' + $dir) }

# 没有本机 x86 C 编译器时，仍使用 ARM GCC 对测试和真实实现做严格编译。
$armCompiler = 'C:\ST\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-gcc.exe'
if (-not (Test-Path -LiteralPath $armCompiler)) {
    throw 'No native C compiler or ARM GCC was found'
}
$index = 0
foreach ($source in $sources) {
    $object = Join-Path $build ('test_' + $index + '.o')
    $args = @('-DHOST_TEST'; '-std=c11'; '-Wall'; '-Wextra'; '-Werror'; '-mcpu=cortex-m0plus'; '-mthumb') +
            $includeArgs + @('-c'; $source; '-o'; $object)
    & $armCompiler @args
    if ($LASTEXITCODE -ne 0) { throw "Test compile failed: $source" }
    $index++
}
Write-Output 'TEST SOURCES COMPILE CLEANLY (execution requires a native C compiler or target board)'
