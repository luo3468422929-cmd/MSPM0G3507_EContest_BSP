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
$protocolHeader = Read-ProjectFile 'Hardware\track_protocol.h'
$protocolSource = Read-ProjectFile 'Hardware\track_protocol.c'
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

Require-Pattern $userConfig '#define TRACK_CHANNEL_COUNT\s+12U' `
    'tracker must have twelve channels'
Require-Pattern $userConfig '#define TRACK_I2C_ADDRESS\s+0x48U' `
    'tracker I2C address must be 0x48'
if ($userConfig -cmatch 'TRACK_I2C_STATUS_REGISTER') {
    throw 'Path Fish uses raw I2C reads and must not define a status register'
}
Require-Pattern $userConfig '#define TRACK_I2C_STALE_UPDATE_LIMIT\s+[0-9]+U' `
    'tracker stale half-frame limit must be configurable'

Require-Pattern $i2cHeader 'Status_t\s+I2C_Read\s*\(' `
    'raw I2C_Read API is missing'
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

Require-Pattern $trackHeader 'uint16_t\s+activeMask' `
    '12-channel tracker mask must be uint16_t'
Require-Pattern $trackSource 'TrackProtocol_PushHalf' `
    'track implementation must assemble Path Fish half frames'
Require-Pattern $trackSource 'I2C_Read\s*\(\s*TRACK_I2C_ADDRESS' `
    'track must use raw I2C reads at the configured address'
Require-Pattern $trackSource 'TRACK_PROTOCOL_FULL_MASK' `
    'track must recognize all twelve active channels'
if ($trackSource -cmatch 'I2C_ReadRegister|TRACK_I2C_STATUS_REGISTER|7U\s*-\s*index') {
    throw 'old eight-channel register mapping must not remain in track.c'
}

Require-Pattern $protocolHeader 'TRACK_PROTOCOL_HALF_FRAME_SIZE\s+7U' `
    'Path Fish half-frame size must be seven bytes'
Require-Pattern $protocolHeader 'TRACK_PROTOCOL_FULL_MASK\s+0x0FFFU' `
    'Path Fish full mask must cover twelve channels'
Require-Pattern $protocolSource "frame\[0\].*'#'" `
    'Path Fish parser must recognize the # half-frame header'
Require-Pattern $protocolSource "frame\[0\].*'!'" `
    'Path Fish parser must recognize the ! half-frame header'

Require-Pattern $testSource 'TRACK mask=%03X error=%.2f i2c=%s' `
    'TEST_TRACK must print all twelve mask bits'
Require-Pattern $testSource 'bits=%s' `
    'TEST_TRACK must print a left-to-right twelve-character bit string'
Require-Pattern $testGuide 'SDA[^\r\n]*PA28[\s\S]*SCL[^\r\n]*PA31' `
    'test guide must document PA28/PA31 tracker wiring'
Require-Pattern $testGuide '0x48[\s\S]*7 字节' `
    'test guide must document address 0x48 and seven-byte half frames'

Write-Output 'TWELVE TRACK I2C CHECK PASSED'
