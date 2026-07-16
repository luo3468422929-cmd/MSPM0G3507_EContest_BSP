$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$syscfg = Get-Content -LiteralPath (Join-Path $project 'empty.syscfg') -Raw
$pins = Get-Content -LiteralPath (Join-Path $project 'Bsp\board_pins.h') -Raw
$encoder = Get-Content -LiteralPath (Join-Path $project 'Hardware\encoder.c') -Raw
$control = Get-Content -LiteralPath (Join-Path $project 'Control\car_control.c') -Raw

function Require-EncoderPin([string]$name, [string]$pin, [string]$gpio)
{
    $logicalName = 'ENCODER_' + $name
    $pattern = '\$name\s*=\s*"' + $logicalName + '"[\s\S]*?' +
               'direction\s*=\s*"INPUT"[\s\S]*?' +
               'interruptEn\s*=\s*true[\s\S]*?' +
               'polarity\s*=\s*"RISE"[\s\S]*?' +
               'internalResistor\s*=\s*"PULL_UP"[\s\S]*?' +
               'pin\.\$assign\s*=\s*"' + $pin + '"'
    if ($syscfg -cnotmatch $pattern) {
        throw "x2 encoder input $name must be pull-up GPIO rising interrupt on $pin"
    }
    if ($pins -cnotmatch ('PIN_ENCODER_' + $name + '\s+' + $gpio + '_' + $logicalName + '_PIN')) {
        throw "board_pins.h missing encoder mapping for $name"
    }
}

Require-EncoderPin 'LEFT_A' 'PA17' 'GPIO_A'
Require-EncoderPin 'LEFT_B' 'PA16' 'GPIO_A'
Require-EncoderPin 'RIGHT_A' 'PB19' 'GPIO_B'
Require-EncoderPin 'RIGHT_B' 'PB20' 'GPIO_B'

foreach ($handler in @(
    'Encoder_HandleLeftARising',
    'Encoder_HandleLeftBRising',
    'Encoder_HandleRightARising',
    'Encoder_HandleRightBRising')) {
    if ($encoder -cnotmatch [regex]::Escape($handler)) {
        throw "x2 encoder source handler is missing: $handler"
    }
}

if ($encoder -cmatch 'else\s+if') {
    throw 'encoder IRQ handler must process each pending source with independent if statements'
}
if ($control -cnotmatch 'void\s+CarControl_Update\s*\(\s*uint32_t\s+nowMs\s*\)') {
    throw 'CarControl_Update must accept current tick for actual encoder dt'
}
if ($control -cnotmatch 'elapsedMs') {
    throw 'CarControl_Update must calculate actual elapsed time before Encoder_UpdateSpeed'
}
if ($control -cnotmatch 'void\s+CarControl_Enable\s*\(\s*bool\s+enable\s*\)[\s\S]*?g_encoderTimeInitialized\s*=\s*false') {
    throw 'enabling the car must reset encoder dt to exclude stopped time'
}
if ($control -cnotmatch 'void\s+CarControl_Stop\s*\(\s*void\s*\)[\s\S]*?g_encoderTimeInitialized\s*=\s*false') {
    throw 'stopping the car must reset encoder dt to exclude stopped time'
}

Write-Output 'ENCODER X2 GPIO IRQ AND ACTUAL DT CHECK PASSED'
