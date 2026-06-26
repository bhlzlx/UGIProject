# =============================================================================
#  Slang 编译脚本
#
#  三步流程:
#    1. 库 → 中间件模块 (target=none, 只做 IR, 不生成 GPU 字节码)
#    2. 顶点着色器 → SPIR-V (独立编译)
#    3. 片元着色器 → SPIR-V (import 库, 链接后生成 SPIR-V)
#
#  用法: .\build_slang.ps1 [-SlangDir "C:\slang"]
# =============================================================================

param(
    [string]$SlangDir = $env:SLANG_PATH
)

$ErrorActionPreference = "Continue"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Slang → SPIR-V  (profile: glsl_460)" -ForegroundColor Cyan
Write-Host "========================================"
Write-Host ""

# ---- 定位 slangc ----
$slangc = $null
if ($SlangDir) {
    $c = Join-Path $SlangDir "slangc.exe"
    if (-not (Test-Path $c)) { $c = Join-Path $SlangDir "bin\slangc.exe" }
    if (Test-Path $c) { $slangc = $c }
}
if (-not $slangc) {
    $slangc = (Get-Command slangc.exe -ErrorAction SilentlyContinue).Source
}
if (-not $slangc) {
    Write-Host "[ERROR] 找不到 slangc.exe" -ForegroundColor Red
    exit 1
}
Write-Host "slangc: $slangc" -ForegroundColor Green
& $slangc --version 2>&1 | Write-Host
Write-Host ""

# ---- 清理 ----
Remove-Item -Force -ErrorAction SilentlyContinue "$scriptDir\fgui_lib.slang-module"
Remove-Item -Force -ErrorAction SilentlyContinue "$scriptDir\fgui_image.vert.spv"
Remove-Item -Force -ErrorAction SilentlyContinue "$scriptDir\fgui_image.frag.spv"

# ---- 通用参数 ----
$commonArgs = @(
    "-target", "spirv",
    "-profile", "glsl_460",
    "-capability", "vulkan_1_0"
)

# =============================================================================
#  Step 1: 库 → 中间件 (.slang-module)
# =============================================================================
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor DarkGray
Write-Host "[Step 1/3] fgui_lib.slang → fgui_lib.slang-module" -ForegroundColor Yellow

$libArgs = @(
    "-target", "none",
    "-o", "$scriptDir\fgui_lib.slang-module",
    "$scriptDir\fgui_lib.slang"
)
Write-Host "  slangc $($libArgs -join ' ')" -ForegroundColor DarkGray
& $slangc @libArgs 2>&1 | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkGray }
if ($LASTEXITCODE -ne 0) {
    Write-Host "[FAIL] 库模块编译失败" -ForegroundColor Red
    exit 1
}
Write-Host "[ OK ]  fgui_lib.slang-module ($((Get-Item $scriptDir\fgui_lib.slang-module).Length) bytes)" -ForegroundColor Green

# =============================================================================
#  Step 2: 顶点着色器 → SPIR-V
# =============================================================================
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor DarkGray
Write-Host "[Step 2/3] fgui_image.vert.slang → fgui_image.vert.spv" -ForegroundColor Yellow

$vertArgs = $commonArgs + @(
    "-entry", "main",
    "-o", "$scriptDir\fgui_image.vert.spv",
    "$scriptDir\fgui_image.vert.slang"
)
Write-Host "  slangc $($vertArgs -join ' ')" -ForegroundColor DarkGray
& $slangc @vertArgs 2>&1 | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkGray }
if ($LASTEXITCODE -ne 0) {
    Write-Host "[FAIL] 顶点着色器编译失败" -ForegroundColor Red
    exit 1
}
Write-Host "[ OK ]  fgui_image.vert.spv ($((Get-Item $scriptDir\fgui_image.vert.spv).Length) bytes)" -ForegroundColor Green

# =============================================================================
#  Step 3: 片元着色器 → SPIR-V (import fgui_lib)
# =============================================================================
Write-Host "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" -ForegroundColor DarkGray
Write-Host "[Step 3/3] fgui_image.frag.slang (import fgui_lib)" -ForegroundColor Yellow

# --- 先试只靠 import + -I ---
$fragArgs = $commonArgs + @(
    "-entry", "fragmentMain",
    "-I", $scriptDir,
    "-o", "$scriptDir\fgui_image.frag.spv",
    "$scriptDir\fgui_image.frag.slang"
)
Write-Host "  slangc $($fragArgs -join ' ')" -ForegroundColor DarkGray
& $slangc @fragArgs 2>&1 | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkGray }

if ($LASTEXITCODE -ne 0) {
    # --- 回退: 加 -r 预编译模块链接 ---
    Write-Host "  [RETRY] 加上 -r 预编译模块重试..." -ForegroundColor Yellow
    $fragArgs2 = $commonArgs + @(
        "-entry", "fragmentMain",
        "-I", $scriptDir,
        "-r", "$scriptDir\fgui_lib.slang-module",
        "-o", "$scriptDir\fgui_image.frag.spv",
        "$scriptDir\fgui_image.frag.slang"
    )
    Write-Host "  slangc $($fragArgs2 -join ' ')" -ForegroundColor DarkGray
    & $slangc @fragArgs2 2>&1 | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkGray }
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[FAIL] 片元着色器编译失败" -ForegroundColor Red
        exit 1
    }
}
Write-Host "[ OK ]  fgui_image.frag.spv ($((Get-Item $scriptDir\fgui_image.frag.spv).Length) bytes)" -ForegroundColor Green

# =============================================================================
#  验证
# =============================================================================
$spirvVal = $null
if ($env:VULKAN_SDK) {
    $v = Join-Path $env:VULKAN_SDK "Bin\spirv-val.exe"
    if (Test-Path $v) { $spirvVal = $v }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  产出" -ForegroundColor Cyan
Write-Host "========================================"
Get-ChildItem "$scriptDir\*.slang-module", "$scriptDir\*.spv" -ErrorAction SilentlyContinue |
    ForEach-Object { Write-Host "  $($_.Name)  $($_.Length) bytes" -ForegroundColor Green }

if ($spirvVal) {
    Write-Host ""
    Write-Host "spirv-val:" -ForegroundColor Yellow
    foreach ($spv in @("fgui_image.vert.spv", "fgui_image.frag.spv")) {
        $path = Join-Path $scriptDir $spv
        if (Test-Path $path) {
            Write-Host "  --- $spv ---" -ForegroundColor DarkGray
            & $spirvVal "--target-env" "vulkan1.1" $path 2>&1 |
                ForEach-Object { Write-Host "    $_" -ForegroundColor DarkGray }
        }
    }
}

Write-Host ""

