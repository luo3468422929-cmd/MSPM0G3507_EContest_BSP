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

# 优先用 MSVC 生成并运行本机程序，确保 CHECK_TRUE 断言真的执行。
# 仅安装 CCS 的电脑可能没有 MSVC；此时给出明确警告，后续 TI 全工程验证仍可继续。
$programFilesX86 = [Environment]::GetFolderPath(
    [Environment+SpecialFolder]::ProgramFilesX86)
$vswhere = if ([string]::IsNullOrWhiteSpace($programFilesX86)) {
    $null
} else {
    Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
}
$vsInstall = $null
if (($null -ne $vswhere) -and (Test-Path -LiteralPath $vswhere)) {
    $vsInstall = (& $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath | Select-Object -First 1)
}

if ([string]::IsNullOrWhiteSpace($vsInstall)) {
    Write-Warning 'MSVC C++ Build Tools not found; native Host assertions were skipped'
} else {
    $vsDevCmd = Join-Path $vsInstall 'Common7\Tools\VsDevCmd.bat'
    if (-not (Test-Path -LiteralPath $vsDevCmd)) {
        throw "VsDevCmd.bat not found: $vsDevCmd"
    }

    $nativeExe = Join-Path $build 'host_tests.exe'
    $nativeIncludes = $includeDirs | ForEach-Object { '/I"' + $_ + '"' }
    $nativeSources = $sources | ForEach-Object { '"' + $_ + '"' }
    $nativeArgs = @('/nologo'; '/DHOST_TEST'; '/std:c11'; '/utf-8';
                    '/W4'; '/WX') + $nativeIncludes + $nativeSources +
                  @('/Fe:"' + $nativeExe + '"')
    $nativeCommand = 'call "' + $vsDevCmd +
                     '" -arch=x64 -host_arch=x64 >nul && cl ' +
                     ($nativeArgs -join ' ')
    Push-Location $build
    try {
        & $env:ComSpec /d /s /c $nativeCommand
        if ($LASTEXITCODE -ne 0) {
            throw "Native host-test compile failed: $LASTEXITCODE"
        }
        $nativeOutput = (& $nativeExe | Out-String)
        if ($LASTEXITCODE -ne 0) {
            throw "Native host tests failed: $LASTEXITCODE`n$nativeOutput"
        }
        if ($nativeOutput -notmatch 'ALL HOST TESTS PASSED') {
            throw "Native host-test success marker is missing`n$nativeOutput"
        }
        Write-Output $nativeOutput.Trim()
    } finally {
        Pop-Location
    }
}

# 再用 ARM GCC 严格编译同一批源码，补充检查 ARM 目标相关告警。
$armCompilerCandidates = @()
if (-not [string]::IsNullOrWhiteSpace($env:ARM_NONE_EABI_GCC)) {
    $armCompilerCandidates += $env:ARM_NONE_EABI_GCC
}
$armFromPath = Get-Command 'arm-none-eabi-gcc.exe' -ErrorAction SilentlyContinue
if ($null -ne $armFromPath) {
    $armCompilerCandidates += $armFromPath.Source
}
$armCompilerCandidates += `
    'C:\ST\STM32CubeCLT_1.18.0\GNU-tools-for-STM32\bin\arm-none-eabi-gcc.exe'
$armCompiler = $armCompilerCandidates |
    Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1

if ([string]::IsNullOrWhiteSpace($armCompiler)) {
    Write-Warning 'ARM GCC not found; optional cross-compiler warning check was skipped'
} else {
    $index = 0
    foreach ($source in $sources) {
        $object = Join-Path $build ('test_' + $index + '.o')
        $args = @('-DHOST_TEST'; '-std=c11'; '-Wall'; '-Wextra'; '-Werror'; '-mcpu=cortex-m0plus'; '-mthumb') +
                $includeArgs + @('-c'; $source; '-o'; $object)
        & $armCompiler @args
        if ($LASTEXITCODE -ne 0) { throw "Test compile failed: $source" }
        $index++
    }
    Write-Output "ARM HOST-TEST SOURCES COMPILE CLEANLY: $armCompiler"
}
