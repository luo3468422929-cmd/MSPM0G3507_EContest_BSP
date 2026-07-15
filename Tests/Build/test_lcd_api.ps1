$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$headerPath = Join-Path $project 'Hardware\lcd.h'
$sourcePath = Join-Path $project 'Hardware\lcd.c'
if (-not (Test-Path -LiteralPath $headerPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
    throw 'Hardware/lcd.c and Hardware/lcd.h are required'
}
$header = Get-Content -LiteralPath $headerPath -Raw
$source = Get-Content -LiteralPath $sourcePath -Raw
foreach ($name in 'LCD_Init','LCD_Clear','LCD_Fill','LCD_ShowString',
                  'LCD_ShowInt','LCD_ShowFloat','LCD_SetBacklight') {
    if ($header -cnotmatch "\b$name\b") { throw "lcd.h missing $name" }
}
if ($source -cnotmatch '0x11' -or $source -cnotmatch '0x3A' -or
    $source -cnotmatch '0x29') {
    throw 'ST7735S initialization sequence is incomplete'
}
if ($source -cmatch 'OLED_|SSD1306_') {
    throw 'LCD source must not depend on OLED or SSD1306'
}
Write-Output 'LCD API CHECK PASSED'
