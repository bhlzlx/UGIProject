# =============================================================================
#  模块化编译基准 — import fx_lib 复用模块 (Slang 内部缓存)
#  用法: .\build_module.ps1 [-Repeat 10]
# =============================================================================
param([string]$SlangDir = $env:SLANG_PATH, [int]$Repeat = 10)

$dir = Split-Path -Parent $MyInvocation.MyCommand.Path; Set-Location $dir
$slangc = if ($SlangDir) { Join-Path $SlangDir "slangc.exe" } else { "slangc" }

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MODULE build (import 复用 x $Repeat 次)" -ForegroundColor Cyan
Write-Host "========================================"
Write-Host ""

$totalSw = [System.Diagnostics.Stopwatch]::StartNew()

# ---- 库预编译 (1次) ----
Write-Host "--- fx_lib.slang (target none, 1次) ---" -ForegroundColor Yellow
$swLib = [System.Diagnostics.Stopwatch]::StartNew()
$out = & $slangc -target none -o "$dir\fx_lib.slang-module" "$dir\fx_lib.slang" 2>&1
$swLib.Stop()
$libTime = $swLib.Elapsed.TotalSeconds
if ($LASTEXITCODE -ne 0) { Write-Host "  FAIL" -ForegroundColor Red; $out | Write-Host; exit 1 }
Write-Host ("  {0:F3}s" -f $libTime) -ForegroundColor Green

# ---- 5 个 shader, 各编译 Repeat 次 ----
$shaders = @("fx_main_a", "fx_main_b", "fx_main_c", "fx_main_d", "fx_main_e")
$entries  = @("fragmentMainA", "fragmentMainB", "fragmentMainC", "fragmentMainD", "fragmentMainE")
$allTimes = @()

for ($r = 1; $r -le $Repeat; $r++) {
    for ($i = 0; $i -lt $shaders.Count; $i++) {
        $s = $shaders[$i]; $entry = $entries[$i]
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        $out = & $slangc -target spirv -entry $entry -I $dir -o "$dir\$s.spv" "$dir\$s.slang" 2>&1
        $sw.Stop()
        $elapsed = $sw.Elapsed.TotalSeconds
        $allTimes += $elapsed
        if ($LASTEXITCODE -ne 0) { Write-Host "  FAIL: $s" -ForegroundColor Red; $out | Write-Host; exit 1 }
    }
    if ($r % 5 -eq 0) { Write-Host "  ... $r / $Repeat" -ForegroundColor DarkGray }
}

$totalSw.Stop()
$totalAll = $totalSw.Elapsed.TotalSeconds
$avg  = ($allTimes | Measure-Object -Average).Average
$min  = ($allTimes | Measure-Object -Minimum).Minimum
$max  = ($allTimes | Measure-Object -Maximum).Maximum

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  MODULE 结果" -ForegroundColor Cyan
Write-Host "========================================"
Write-Host ("  库预编译:  {0:F3}s (1次)" -f $libTime)
Write-Host ("  import 编译: {0} 次 (5 shader x {1} rounds)" -f $allTimes.Count, $Repeat)
Write-Host ("  单次平均: {0:F3}s" -f $avg) -ForegroundColor Yellow
Write-Host ("  最快/最慢: {0:F3}s / {1:F3}s" -f $min, $max) -ForegroundColor Yellow
Write-Host ("  总耗时:   {0:F2}s" -f $totalAll) -ForegroundColor Green
