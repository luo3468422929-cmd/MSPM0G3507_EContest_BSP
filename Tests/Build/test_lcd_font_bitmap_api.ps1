$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$fontHeader = Join-Path $project 'Hardware\lcd_font.h'
$fontSource = Join-Path $project 'Hardware\lcd_font.c'
$bitmapHeader = Join-Path $project 'Hardware\lcd_bitmap.h'
$bitmapSource = Join-Path $project 'Hardware\lcd_bitmap.c'
foreach ($path in $fontHeader, $fontSource, $bitmapHeader, $bitmapSource) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        throw "LCD extension file missing: $path"
    }
}

$font = Get-Content -LiteralPath $fontHeader -Raw
$fontSourceText = Get-Content -LiteralPath $fontSource -Raw
$bitmap = Get-Content -LiteralPath $bitmapHeader -Raw
$bitmapSourceText = Get-Content -LiteralPath $bitmapSource -Raw
if ($font -cnotmatch 'LCD_ShowChinese16') { throw 'lcd_font.h missing LCD_ShowChinese16' }
if ($bitmap -cnotmatch 'LCD_ShowBitmap') { throw 'lcd_bitmap.h missing LCD_ShowBitmap' }
if ($fontSourceText -cmatch 'stm32|STM32|HAL_|GPIO_ResetBits') {
    throw 'LCD font source contains STM32 dependencies'
}
if ($bitmapSourceText -cmatch 'stm32|STM32|HAL_|GPIO_ResetBits') {
    throw 'LCD bitmap source contains STM32 dependencies'
}
if ($fontSourceText -cnotmatch 'STATUS_INVALID_PARAM' -or
    $bitmapSourceText -cnotmatch 'STATUS_INVALID_PARAM') {
    throw 'LCD extension sources must validate parameters'
}
Write-Output 'LCD FONT BITMAP API CHECK PASSED'
