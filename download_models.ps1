# IntelNet Browser - Model Download Script
# This script helps you download the required AI models

param(
    [switch]$SkipQwen,
    [switch]$SkipPiper
)

$ErrorActionPreference = "Stop"

# Model URLs (replace with actual URLs when models are hosted)
$QWEN_MODEL_URL = "https://huggingface.co/Qwen/Qwen2-VL-7B-Instruct-GGUF/resolve/main/qwen2-vl-7b-instruct-q4_0.gguf"
$PIPER_MODEL_URL = "https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/models/vits-medium.onnx"

$RESOURCES_DIR = Join-Path $PSScriptRoot "resources"
$MODELS_DIR = Join-Path $RESOURCES_DIR "models"
$PIPER_DIR = Join-Path $RESOURCES_DIR "piper"

Write-Host "=== IntelNet Model Downloader ===" -ForegroundColor Cyan
Write-Host ""

# Create directories
if (-not (Test-Path $MODELS_DIR)) {
    New-Item -ItemType Directory -Path $MODELS_DIR | Out-Null
    Write-Host "[+] Created directory: $MODELS_DIR" -ForegroundColor Green
}

if (-not (Test-Path $PIPER_DIR)) {
    New-Item -ItemType Directory -Path $PIPER_DIR | Out-Null
    Write-Host "[+] Created directory: $PIPER_DIR" -ForegroundColor Green
}

# Download Qwen2-VL model
if (-not $SkipQwen) {
    Write-Host ""
    Write-Host "[*] Downloading Qwen2-VL model..." -ForegroundColor Yellow
    Write-Host "    URL: $QWEN_MODEL_URL" -ForegroundColor Gray
    Write-Host "    This may take a while (model size: ~4GB)" -ForegroundColor Gray

    $qwenPath = Join-Path $MODELS_DIR "qwen2-vl-7b-instruct-q4_0.gguf"

    if (Test-Path $qwenPath) {
        Write-Host "[!] Model already exists: $qwenPath" -ForegroundColor Yellow
        $answer = Read-Host "    Overwrite? (y/N)"
        if ($answer -ne "y") {
            Write-Host "[*] Skipping Qwen2-VL download" -ForegroundColor Gray
        } else {
            # Invoke-WebRequest -Uri $QWEN_MODEL_URL -OutFile $qwenPath
            Write-Host "[!] Please download manually from: $QWEN_MODEL_URL" -ForegroundColor Yellow
            Write-Host "    Save to: $qwenPath" -ForegroundColor Gray
        }
    } else {
        # Invoke-WebRequest -Uri $QWEN_MODEL_URL -OutFile $qwenPath
        Write-Host "[!] Please download manually from: $QWEN_MODEL_URL" -ForegroundColor Yellow
        Write-Host "    Save to: $qwenPath" -ForegroundColor Gray
    }
}

# Download Piper TTS model
if (-not $SkipPiper) {
    Write-Host ""
    Write-Host "[*] Downloading Piper TTS model..." -ForegroundColor Yellow
    Write-Host "    URL: $PIPER_MODEL_URL" -ForegroundColor Gray

    $piperPath = Join-Path $PIPER_DIR "vits-medium.onnx"

    if (Test-Path $piperPath) {
        Write-Host "[!] Model already exists: $piperPath" -ForegroundColor Yellow
        $answer = Read-Host "    Overwrite? (y/N)"
        if ($answer -ne "y") {
            Write-Host "[*] Skipping Piper download" -ForegroundColor Gray
        } else {
            # Invoke-WebRequest -Uri $PIPER_MODEL_URL -OutFile $piperPath
            Write-Host "[!] Please download manually from: $PIPER_MODEL_URL" -ForegroundColor Yellow
            Write-Host "    Save to: $piperPath" -ForegroundColor Gray
        }
    } else {
        # Invoke-WebRequest -Uri $PIPER_MODEL_URL -OutFile $piperPath
        Write-Host "[!] Please download manually from: $PIPER_MODEL_URL" -ForegroundColor Yellow
        Write-Host "    Save to: $piperPath" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "=== Download Instructions ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Models are too large for automatic download." -ForegroundColor Yellow
Write-Host "Please download them manually and place in the directories above." -ForegroundColor Yellow
Write-Host ""
Write-Host "Required files:" -ForegroundColor White
Write-Host "  1. Qwen2-VL GGUF model -> $MODELS_DIR" -ForegroundColor Gray
Write-Host "  2. Piper TTS model -> $PIPER_DIR" -ForegroundColor Gray
Write-Host ""
Write-Host "See README.md for download links and instructions." -ForegroundColor White
Write-Host ""
