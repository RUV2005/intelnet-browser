# IntelNet Browser - Model Download Script
# This script helps you download the required AI models

param(
    [switch]$SkipQwen,
    [switch]$SkipPiper,
    [switch]$SkipOcr
)

$ErrorActionPreference = "Stop"

# Qwen2-VL-2B model files, mirrored on ModelScope for fast download in China.
# Source: qwen/Qwen2-VL-2B-Instruct-GGUF (Apache License 2.0)
$QWEN_REPO = "danmo6321/intelnet-Qwen2-VL-2B-GGUF"
$QWEN_REVISION = "master"
$QWEN_FILES = @("model.gguf", "mmproj.gguf")
$PIPER_MODEL_URL = "https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/chaowen/medium/zh_CN-chaowen-medium.onnx"
$PIPER_CONFIG_URL = "https://huggingface.co/rhasspy/piper-voices/resolve/main/zh/zh_CN/chaowen/medium/zh_CN-chaowen-medium.onnx.json"

$RESOURCES_DIR = Join-Path $PSScriptRoot "resources"
$MODELS_DIR = Join-Path $RESOURCES_DIR "models"
$QWEN_DIR = Join-Path $MODELS_DIR "qwen2vl"
$PIPER_DIR = Join-Path $RESOURCES_DIR "piper"
$OCR_DIR = Join-Path $MODELS_DIR "paddleocr-vl"
$OCR_REPO = "PaddlePaddle/PaddleOCR-VL-1.6-GGUF"
$OCR_FILES = @("PaddleOCR-VL-1.6-GGUF.gguf", "PaddleOCR-VL-1.6-GGUF-mmproj.gguf")

Write-Host "=== IntelNet Model Downloader ===" -ForegroundColor Cyan
Write-Host ""

# Create directories
foreach ($d in @($MODELS_DIR, $QWEN_DIR, $PIPER_DIR, $OCR_DIR)) {
    if (-not (Test-Path $d)) {
        New-Item -ItemType Directory -Path $d | Out-Null
        Write-Host "[+] Created directory: $d" -ForegroundColor Green
    }
}

if (-not $SkipOcr) {
    Write-Host ""
    Write-Host "[*] Downloading PaddleOCR-VL-1.6 model files..." -ForegroundColor Yellow
    foreach ($f in $OCR_FILES) {
        $url = "https://www.modelscope.cn/models/$OCR_REPO/resolve/master/$f"
        $dest = Join-Path $OCR_DIR $f
        if (Test-Path $dest) {
            Write-Host "    [!] Already exists, skipping: $f" -ForegroundColor Yellow
            continue
        }
        Write-Host "    Downloading $f ..." -ForegroundColor Gray
        try {
            Invoke-WebRequest -Uri $url -OutFile $dest
            Write-Host "    [+] Done: $f" -ForegroundColor Green
        } catch {
            Write-Host "    [!] Download failed: $f" -ForegroundColor Red
            Write-Host "        Manual link: $url" -ForegroundColor Gray
        }
    }
}

# Download Qwen2-VL model from ModelScope mirror
if (-not $SkipQwen) {
    Write-Host ""
    Write-Host "[*] Downloading Qwen2-VL-2B model files..." -ForegroundColor Yellow
    Write-Host "    Repo: $QWEN_REPO" -ForegroundColor Gray
    Write-Host "    This may take a while (total ~1.6GB)" -ForegroundColor Gray

    foreach ($f in $QWEN_FILES) {
        $url = "https://www.modelscope.cn/models/$QWEN_REPO/resolve/$QWEN_REVISION/$f"
        $dest = Join-Path $QWEN_DIR $f

        if (Test-Path $dest) {
            Write-Host "    [!] Already exists, skipping: $f" -ForegroundColor Yellow
            continue
        }

        Write-Host "    Downloading $f ..." -ForegroundColor Gray
        try {
            Invoke-WebRequest -Uri $url -OutFile $dest
            Write-Host "    [+] Done: $f" -ForegroundColor Green
        } catch {
            Write-Host "    [!] Download failed: $f" -ForegroundColor Red
            Write-Host "        Manual link: $url" -ForegroundColor Gray
            Write-Host "        Save to: $dest" -ForegroundColor Gray
        }
    }
}

# Download Piper TTS model
if (-not $SkipPiper) {
    Write-Host ""
    Write-Host "[*] Downloading Piper TTS model..." -ForegroundColor Yellow
    Write-Host "    URL: $PIPER_MODEL_URL" -ForegroundColor Gray

    $piperPath = Join-Path $PIPER_DIR "chaowen.onnx"
    $piperConfigPath = Join-Path $PIPER_DIR "chaowen.onnx.json"

    if (Test-Path $piperPath) {
        Write-Host "[!] Model already exists: $piperPath" -ForegroundColor Yellow
        $answer = Read-Host "    Overwrite? (y/N)"
        if ($answer -ne "y") {
            Write-Host "[*] Skipping Piper download" -ForegroundColor Gray
        } else {
            Invoke-WebRequest -Uri $PIPER_MODEL_URL -OutFile $piperPath
            Write-Host "[+] Piper model downloaded" -ForegroundColor Green
        }
    } else {
        Invoke-WebRequest -Uri $PIPER_MODEL_URL -OutFile $piperPath
        Write-Host "[+] Piper model downloaded" -ForegroundColor Green
    }

    if (Test-Path $piperConfigPath) {
        Write-Host "[!] Piper config already exists: $piperConfigPath" -ForegroundColor Yellow
    } else {
        Invoke-WebRequest -Uri $PIPER_CONFIG_URL -OutFile $piperConfigPath
        Write-Host "[+] Piper config downloaded" -ForegroundColor Green
    }
}

Write-Host ""
Write-Host "=== Done ===" -ForegroundColor Cyan
Write-Host ""
Write-Host "Required files:" -ForegroundColor White
Write-Host "  1. Qwen2-VL-2B GGUF (model.gguf, mmproj.gguf) -> $QWEN_DIR" -ForegroundColor Gray
Write-Host "  2. Piper TTS model -> $PIPER_DIR" -ForegroundColor Gray
Write-Host "  3. PaddleOCR-VL-1.6 model files -> $OCR_DIR" -ForegroundColor Gray
Write-Host ""
