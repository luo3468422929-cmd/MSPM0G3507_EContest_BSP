$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

function Read-ProjectFile {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $path = Join-Path $project $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "required file is missing: $RelativePath"
    }
    return Get-Content -LiteralPath $path -Raw
}

function Require-Pattern {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Message
    )

    if ($Text -cnotmatch $Pattern) {
        throw $Message
    }
}

$syscfg = Read-ProjectFile 'empty.syscfg'
$userConfig = Read-ProjectFile 'User\user_config.h'
$i2cHeader = Read-ProjectFile 'Bsp\i2c.h'
$i2cSource = Read-ProjectFile 'Bsp\i2c.c'
$trackHeader = Read-ProjectFile 'Hardware\track.h'
$trackSource = Read-ProjectFile 'Hardware\track.c'
$testSource = Read-ProjectFile 'User\test.c'
$testGuide = Read-ProjectFile 'docs\模块逐项测试流程.md'

Require-Pattern $syscfg 'I2C1\.\$name\s*=\s*"TRACK_I2C"' `
    'TRACK_I2C name is missing'
Require-Pattern $syscfg 'I2C1\.peripheral\.\$assign\s*=\s*"I2C0"' `
    'TRACK_I2C must use I2C0'
Require-Pattern $syscfg 'I2C1\.peripheral\.sdaPin\.\$assign\s*=\s*"PA28"' `
    'TRACK_I2C SDA must use PA28'
Require-Pattern $syscfg 'I2C1\.peripheral\.sclPin\.\$assign\s*=\s*"PA31"' `
    'TRACK_I2C SCL must use PA31'

Require-Pattern $userConfig '#define TRACK_CHANNEL_COUNT\s+8U' `
    'tracker must have eight channels'
Require-Pattern $userConfig '#define TRACK_I2C_ADDRESS\s+0x12U' `
    'tracker I2C address must be 0x12'
Require-Pattern $userConfig '#define TRACK_I2C_STATUS_REGISTER\s+0x30U' `
    'tracker status register must be 0x30'
Require-Pattern $userConfig '#define TRACK_I2C_TIMEOUT_LOOPS\s+[0-9]+U' `
    'tracker I2C timeout must be configurable'

Require-Pattern $i2cHeader 'Status_t\s+I2C_ReadRegister\s*\(' `
    'I2C_ReadRegister API is missing'
Require-Pattern $i2cHeader 'Status_t\s+I2C_WriteRegister\s*\(' `
    'I2C_WriteRegister API is missing'
Require-Pattern $i2cSource 'DL_I2C_fillControllerTXFIFO' `
    'I2C TX FIFO API is missing'
Require-Pattern $i2cSource 'DL_I2C_startControllerTransfer' `
    'I2C transfer API is missing'
Require-Pattern $i2cSource 'DL_I2C_receiveControllerData' `
    'I2C RX API is missing'
Require-Pattern $i2cSource 'TRACK_I2C_TIMEOUT_LOOPS' `
    'I2C timeout guard is missing'
Require-Pattern $i2cSource 'STATUS_INVALID_PARAM' `
    'I2C parameter validation is missing'
Require-Pattern $i2cSource 'STATUS_TIMEOUT' `
    'I2C timeout return is missing'
Require-Pattern $i2cSource 'DL_I2C_CONTROLLER_STATUS_ERROR' `
    'I2C bus error check is missing'

Require-Pattern $trackHeader 'TRACK_CHANNEL_COUNT' `
    'track header must use configured channel count'
Require-Pattern $trackSource 'TRACK_CHANNEL_COUNT\s*==\s*8U' `
    'track implementation must be eight-channel'
Require-Pattern $trackSource 'TRACK_I2C_ADDRESS' `
    'track must use configured I2C address'
Require-Pattern $trackSource 'TRACK_I2C_STATUS_REGISTER' `
    'track must use configured status register'
Require-Pattern $trackSource '7U\s*-\s*index' `
    'track must map X1 bit7 through X8 bit0'
if ($trackSource -cmatch 'PIN_TRACK_CH[1-5]') {
    throw 'old five-channel GPIO pins must not remain in track.c'
}

Require-Pattern $testSource 'TRACK mask=%02X error=%.2f i2c=%s' `
    'TEST_TRACK must report the I2C link status'
Require-Pattern $testGuide 'SDA[^\r\n]*PA28[\s\S]*SCL[^\r\n]*PA31' `
    'test guide must document PA28/PA31 tracker wiring'
Require-Pattern $testGuide '0x12[\s\S]*0x30' `
    'test guide must document the tracker address and register'

Write-Output 'EIGHT TRACK I2C CHECK PASSED'
