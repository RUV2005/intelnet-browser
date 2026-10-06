# setup_portable_python.ps1
# 一键搭建安装包自带的 portable Python（含 piper-tts 中文语音链路）
# 在项目根目录运行：.\setup_portable_python.ps1
# 可选参数：
#   -VocabUrl     HF 缓存找不到 vocab.txt 时的下载地址
#   -G2pwOnnxUrl  g2pW/g2pw.onnx 的下载地址（151MB，未进 git）
#   -PythonMirror embed 包镜像（默认 npmmirror，404 就换官方）
#   -PipIndex     pip 镜像源（默认清华）
param(
    [string]$PythonVersion = "3.13.7",
    [string]$PythonMirror = "https://npmmirror.com/mirrors/python",
    [string]$PipIndex = "https://pypi.tuna.tsinghua.edu.cn/simple",
    [string]$GetPipUrl = "https://bootstrap.pypa.io/get-pip.py",
    [string]$VocabUrl = "",
    [string]$G2pwOnnxUrl = ""
)

$ErrorActionPreference = "Stop"
$ResDir = Join-Path $PSScriptRoot "resources"
$PyDir  = Join-Path $ResDir "python"
$PyExe  = Join-Path $PyDir "python.exe"

function Step($msg) { Write-Host "`n==> $msg" -ForegroundColor Cyan }

# ── 1. 下 embed 版 Python ──────────────────────────────
Step "1/7 检查 portable Python"
if (Test-Path $PyExe) {
    Write-Host "已存在，跳过下载：$PyExe"
} else {
    $zipUrl = "$PythonMirror/$PythonVersion/python-$PythonVersion-embed-amd64.zip"
    $zipTmp = Join-Path $env:TEMP "python-embed.zip"
    Write-Host "下载 $zipUrl"
    try {
        Invoke-WebRequest -Uri $zipUrl -OutFile $zipTmp
    } catch {
        throw "镜像下载失败（$_），可重跑并指定 -PythonMirror 'https://www.python.org/ftp/python'"
    }
    New-Item -ItemType Directory -Force -Path $PyDir | Out-Null
    Expand-Archive -Path $zipTmp -DestinationPath $PyDir -Force
    Remove-Item $zipTmp
    Write-Host "解压到 $PyDir"
}

# ── 2. 开 import site ──────────────────────────────────
Step "2/7 启用 site-packages"
$pth = Get-ChildItem -Path $PyDir -Filter "python*._pth" | Select-Object -First 1
if (-not $pth) { throw "找不到 python*._pth" }
$c = Get-Content $pth.FullName -Raw
if ($c -match '#\s*import site') {
    $c = $c -replace '#\s*import site', 'import site'
    Set-Content -Path $pth.FullName -Value $c -NoNewline
    Write-Host "已取消 import site 的注释"
} else {
    Write-Host "import site 已启用，跳过"
}

# ── 3. 装 pip ──────────────────────────────────────────
Step "3/7 安装 pip"
$pipTest = & $PyExe -m pip --version 2>$null
if ($pipTest) {
    Write-Host "pip 已存在：$pipTest"
} else {
    $getPip = Join-Path $PyDir "get-pip.py"
    Invoke-WebRequest -Uri $GetPipUrl -OutFile $getPip
    & $PyExe $getPip
    Remove-Item $getPip
}

# ── 4. 装 piper-tts[zh]（走国内镜像）───────────────────
Step "4/7 安装 piper-tts[zh]"
& $PyExe -m pip install -i $PipIndex "piper-tts[zh]" unicode-rbnf
& $PyExe -c "from piper.voice import PiperVoice; print('piper ok')"

# ── 5. g2pW 模型文件 ───────────────────────────────────
Step "5/7 检查 g2pW"
$g2pDir = Join-Path $ResDir "g2pW"
$need = @("config.py", "POLYPHONIC_CHARS.txt", "MONOPHONIC_CHARS.txt")
$missing = $need | Where-Object { -not (Test-Path (Join-Path $g2pDir $_)) }
if ($missing) {
    Write-Warning "resources\g2pW 缺文件：$($missing -join ', ')，请从备份拷入"
} else {
    Write-Host "g2pW 文本文件齐全"
}
$onnxDst = Join-Path $g2pDir "g2pw.onnx"
if (Test-Path $onnxDst) {
    Write-Host "g2pw.onnx 已存在，跳过"
} elseif ($G2pwOnnxUrl) {
    Write-Host "下载 g2pw.onnx（151MB）..."
    Invoke-WebRequest -Uri $G2pwOnnxUrl -OutFile $onnxDst
} else {
    Write-Warning "缺 resources\g2pW\g2pw.onnx（151MB，未进 git）"
    Write-Warning "传到 ModelScope 后重跑：.\setup_portable_python.ps1 -G2pwOnnxUrl <直链地址>"
}

# ── 6. vocab.txt（bert tokenizer）──────────────────────
Step "6/7 准备 bert vocab.txt"
$bertDir = Join-Path $g2pDir "bert"
$vocabDst = Join-Path $bertDir "vocab.txt"
if ((Test-Path $vocabDst) -and ((Get-Item $vocabDst).Length -gt 1000)) {
    Write-Host "vocab.txt 已存在，跳过"
} else {
    New-Item -ItemType Directory -Force -Path $bertDir | Out-Null
    # 先翻 HF 本地缓存
    $hfCache = Join-Path $env:USERPROFILE ".cache\huggingface\hub\models--bert-base-chinese"
    $found = Get-ChildItem -Path $hfCache -Recurse -Filter "vocab.txt" -ErrorAction SilentlyContinue |
        Where-Object { $_.Length -gt 1000 } | Select-Object -First 1
    if ($found) {
        Copy-Item $found.FullName $vocabDst -Force
        Write-Host "从 HF 缓存拷入：$($found.FullName)"
    } elseif ($VocabUrl) {
        Write-Host "下载 $VocabUrl"
        Invoke-WebRequest -Uri $VocabUrl -OutFile $vocabDst
    } else {
        Write-Warning "找不到可用的 vocab.txt！"
        Write-Warning "手动把 bert-base-chinese 的 vocab.txt 放到：$vocabDst"
        Write-Warning "或重跑：.\setup_portable_python.ps1 -VocabUrl <地址>"
    }
}

# ── 7. config.py 指本地 ────────────────────────────────
Step "7/7 config.py 指向本地 tokenizer"
$configPy = Join-Path $g2pDir "config.py"
if (Test-Path $configPy) {
    $c = Get-Content $configPy -Raw
    if ($c -match "model_source\s*=\s*'bert-base-chinese'") {
        $c = $c -replace "model_source\s*=\s*'bert-base-chinese'", "model_source = 'g2pW/bert'"
        Set-Content -Path $configPy -Value $c -NoNewline
        Write-Host "model_source 已改为本地 g2pW/bert"
    } else {
        Write-Host "model_source 已是本地路径，跳过"
    }
}

Write-Host "`n完成。验证命令：" -ForegroundColor Green
Write-Host "  .\resources\python\python.exe -c `"from piper.voice import PiperVoice; print('ok')`""