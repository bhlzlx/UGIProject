# =============================================================================
#  单体编译基准 — 每个 shader 内联全部库代码，独立完整编译
#  用法: .\build_monolithic.ps1 [-Repeat 10]
# =============================================================================
param([string]$SlangDir = $env:SLANG_PATH, [int]$Repeat = 10)

$dir = Split-Path -Parent $MyInvocation.MyCommand.Path; Set-Location $dir
$slangc = if ($SlangDir) { Join-Path $SlangDir "slangc.exe" } else { "slangc" }

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MONOLITHIC  (每个内联完整库 x $Repeat 次)" -ForegroundColor Cyan
Write-Host "========================================"
Write-Host ""

$shaders = @("fx_main_a_monolith", "fx_main_b_monolith", "fx_main_c_monolith", "fx_main_d_monolith", "fx_main_e_monolith")
$allTimes = @()
$totalSw = [System.Diagnostics.Stopwatch]::StartNew()

for ($r = 1; $r -le $Repeat; $r++) {
    foreach ($s in $shaders) {
        $entry = "fragmentMain$($s[8].ToString().ToUpper())"
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        $out = & $slangc -target spirv -entry $entry -o "$dir\$s.spv" "$dir\$s.slang" 2>&1
        $sw.Stop()
        $elapsed = $sw.Elapsed.TotalSeconds
        $allTimes += $elapsed
        if ($LASTEXITCODE -ne 0) { Write-Host "  FAIL: $s" -ForegroundColor Red; $out | Write-Host; exit 1 }
    }
    if ($r % 5 -eq 0) { Write-Host "  ... $r / $Repeat" -ForegroundColor DarkGray }
}

$totalSw.Stop()

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MONOLITHIC 结果" -ForegroundColor Cyan
Write-Host "========================================"
$avg  = ($allTimes | Measure-Object -Average).Average
$min  = ($allTimes | Measure-Object -Minimum).Minimum
$max  = ($allTimes | Measure-Object -Maximum).Maximum
$total = $totalSw.Elapsed.TotalSeconds
Write-Host ("  编译次数: {0}  (5 shader x {1} rounds)" -f $allTimes.Count, $Repeat)
Write-Host ("  单次平均: {0:F3}s" -f $avg) -ForegroundColor Yellow
Write-Host ("  最快/最慢: {0:F3}s / {1:F3}s" -f $min, $max) -ForegroundColor Yellow
Write-Host ("  总耗时:   {0:F2}s" -f $total) -ForegroundColor Green
