$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$syscfgPath = Join-Path $project 'empty.syscfg'
$syscfg = Get-Content -LiteralPath $syscfgPath -Raw

function Require-NamedPin {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Pin
    )

    $nameText = [regex]::Escape('$name = "' + $Name + '"')
    $pinText = [regex]::Escape('pin.$assign = "' + $Pin + '"')
    $pattern = $nameText + '(?:(?!\$name\s*=)[\s\S])*?' + $pinText
    if ($syscfg -cnotmatch $pattern) {
        throw "$Name must use $Pin"
    }
}

function Require-Pattern {
    param(
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Message
    )
    if ($syscfg -cnotmatch $Pattern) {
        throw $Message
    }
}

Require-NamedPin -Name 'MOTOR_AIN1' -Pin 'PB18'
Require-NamedPin -Name 'MOTOR_AIN2' -Pin 'PA7'
Require-NamedPin -Name 'MOTOR_BIN1' -Pin 'PA8'
Require-NamedPin -Name 'MOTOR_BIN2' -Pin 'PA9'
Require-NamedPin -Name 'MOTOR_STBY' -Pin 'PB24'
Require-NamedPin -Name 'ENCODER_LEFT_B' -Pin 'PA16'
Require-NamedPin -Name 'ENCODER_RIGHT_B' -Pin 'PB20'
Require-NamedPin -Name 'TRACK_CH1' -Pin 'PA0'
Require-NamedPin -Name 'TRACK_CH2' -Pin 'PA1'
Require-NamedPin -Name 'TRACK_CH3' -Pin 'PA15'
Require-NamedPin -Name 'TRACK_CH4' -Pin 'PB6'
Require-NamedPin -Name 'TRACK_CH5' -Pin 'PB7'
Require-NamedPin -Name 'USER_KEY' -Pin 'PB2'
Require-NamedPin -Name 'LCD_CS' -Pin 'PA27'
Require-NamedPin -Name 'LCD_DC' -Pin 'PA26'
Require-NamedPin -Name 'LCD_RES' -Pin 'PA25'
Require-NamedPin -Name 'LCD_BL' -Pin 'PA24'

Require-Pattern 'PWM1\.peripheral\.ccp0Pin\.\$assign\s*=\s*"PA12"' `
    'left motor PWM must use PA12/TIMG0_C0'
Require-Pattern 'PWM1\.peripheral\.ccp1Pin\.\$assign\s*=\s*"PA13"' `
    'right motor PWM must use PA13/TIMG0_C1'
Require-Pattern 'CAPTURE1\.peripheral\.\$assign\s*=\s*"TIMA1"[\s\S]*?CAPTURE1\.peripheral\.ccp0Pin\.\$assign\s*=\s*"PA17"' `
    'left encoder A must use PA17/TIMA1_C0'
Require-Pattern 'CAPTURE2\.peripheral\.\$assign\s*=\s*"TIMG8"[\s\S]*?CAPTURE2\.peripheral\.ccp1Pin\.\$assign\s*=\s*"PB19"' `
    'right encoder A must use PB19/TIMG8_C1'
Require-Pattern 'UART2\.peripheral\.\$assign\s*=\s*"UART2"[\s\S]*?UART2\.peripheral\.txPin\.\$assign\s*=\s*"PA21"[\s\S]*?UART2\.peripheral\.rxPin\.\$assign\s*=\s*"PA22"' `
    'IMU must use UART2 TX=PA21 RX=PA22'

if ($syscfg -cnotmatch '\$name\s*=\s*"LCD_SPI"[\s\S]*?peripheral\.\$assign\s*=\s*"SPI1"') {
    throw 'LCD_SPI must use SPI1'
}
if ($syscfg -cnotmatch 'sclkPin\.\$assign\s*=\s*"PB9"') {
    throw 'LCD SCK must use PB9'
}
if ($syscfg -cnotmatch '(?:pico|mosi)Pin\.\$assign\s*=\s*"PB8"') {
    throw 'LCD MOSI must use PB8'
}
Require-Pattern 'SPI1\.targetBitRate\s*=\s*4000000' 'LCD SPI must run at 4 MHz'
Require-Pattern 'SPI1\.polarity\s*=\s*"1"' 'LCD SPI must use CPOL=1'
Require-Pattern 'SPI1\.phase\s*=\s*"1"' 'LCD SPI must use CPHA=1'
if ($syscfg -cmatch 'addModule\("/ti/driverlib/I2C"') {
    throw 'temporary active profile must not allocate I2C'
}
if ($syscfg -cmatch 'pin\.\$assign\s*=\s*"PA2"') {
    throw 'PA2 is reserved by the board ROSC circuit and must stay unassigned'
}

$userConfigPath = Join-Path $project 'User\user_config.h'
if (-not (Test-Path -LiteralPath $userConfigPath -PathType Leaf)) {
    throw 'User/user_config.h is missing'
}
$userConfig = Get-Content -LiteralPath $userConfigPath -Raw
if ($userConfig -cmatch 'DL_GPIO_PIN_|GPIOA|GPIOB|\bPA[0-9]+\b|\bPB[0-9]+\b') {
    throw 'User configuration must not contain physical pin identifiers'
}

Write-Output 'TARGET PINOUT CHECK PASSED'
