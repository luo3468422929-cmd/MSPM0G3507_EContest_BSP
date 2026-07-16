$ErrorActionPreference = 'Stop'

$project = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$source = Get-Content -LiteralPath (Join-Path $project 'User\test.c') -Raw
$match = [regex]::Match(
    $source,
    'static void Test_RunImu\(uint32_t nowMs\)(.*?)static void Test_RunLcd',
    [System.Text.RegularExpressions.RegexOptions]::Singleline)
if (-not $match.Success) { throw 'Test_RunImu not found' }
$function = $match.Groups[1].Value
foreach ($token in 'LCD_ShowString','LCD_ShowFloat','sample.yawDeg','sample.gyroZDps','IMU WAIT') {
    if ($function -notmatch [regex]::Escape($token)) {
        throw "IMU LCD display missing: $token"
    }
}
Write-Output 'IMU LCD DISPLAY CHECK PASSED'
