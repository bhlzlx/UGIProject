# Android PBR APK 一键构建
# 前置条件: 1) Android SDK + NDK 已安装  2) UGI + LightWeightCommon 已编译为 .a
$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$proj = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $root))  # UGIProject root

Write-Host "=== Step 1: 编译 shader → SPIR-V ===" -ForegroundColor Cyan
Push-Location "$proj/bin/shaders/pbr"
# 用 ShaderCompiler 编译 .slang
& "$proj/../UGIShaderCompiler/build/ShaderCompiler.exe" "$proj/bin/shaders/pbr"
Pop-Location

Write-Host "=== Step 2: 复制 assets ===" -ForegroundColor Cyan
$assets = "$root/app/src/main/assets"
New-Item -Force -ItemType Directory "$assets/shaders/pbr" | Out-Null
New-Item -Force -ItemType Directory "$assets/image" | Out-Null
Copy-Item -Force "$proj/bin/shaders/pbr/*.spv" "$assets/shaders/pbr/"
Copy-Item -Force "$proj/bin/shaders/pbr/pipeline.bin" "$assets/shaders/pbr/"
Copy-Item -Force "$proj/bin/image/island.png" "$assets/image/"

Write-Host "=== Step 3: Gradle build ===" -ForegroundColor Cyan
# 需要先复制 local.properties.example → local.properties 并填写 sdk.dir
Push-Location $root
./gradlew assembleRelease
Pop-Location

Write-Host "=== Done ===" -ForegroundColor Green
Write-Host "APK: $root/app/build/outputs/apk/release/app-release.apk"
