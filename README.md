# IntelNet Browser

<div align="center">

**Make the web easier to see, understand, and hear**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Qt 6](https://img.shields.io/badge/Qt-6.5-green.svg)](https://www.qt.io/)
[![Rust](https://img.shields.io/badge/Rust-stable-orange.svg)](https://www.rust-lang.org/)

[Features](#features) • [Architecture](#architecture) • [Download](#download) • [Build from source](#build-from-source) • [Contributing](#contributing)

</div>

---

## About

IntelNet Browser (明镜浏览器) is an open-source accessibility browser that bakes AI content understanding, image analysis, and speech synthesis directly into the browsing experience. Qt WebEngine on the front, a Rust AI core underneath, and **all inference runs locally — your data never leaves the device**.

## Features

### 📄 Page Summaries
Reads the page DOM (title, headings, body text, images, links) and generates a Chinese summary with a local vision-language model. No screenshot OCR involved.

### 🖼️ Image Analysis
Click any image on a web page and the local vision model describes it in Chinese, including charts and diagrams.

### 🔘 Button Function Recognition
For icon-only buttons with unclear meaning, the AI looks at the icon style and tells you what it does in one sentence.

### 🔢 CAPTCHA Reading
PaddleOCR-VL recognizes CAPTCHAs locally and reads them aloud character by character. Failed recognitions are reported honestly instead of reading out a wrong guess.

### 🔊 Chinese Text-to-Speech
Piper-based Chinese speech synthesis with adjustable speed. Reads selected text or full-page summaries.

### ⌨️ Accessible Operation
Keyboard shortcuts, focus indicators, high contrast; the install and model-download flows are screen-reader operable.

## Architecture

```
┌─────────────────────────────────────┐
│           Qt 6 Client                │
│   WebEngine • Voice Panel • Hotkeys  │
└─────────────────────────────────────┘
                  │ C FFI
┌─────────────────────────────────────┐
│           Rust AI Core               │
│  llama-server ×2 • Piper TTS        │
└─────────────────────────────────────┘
```

- **Frontend**: Qt 6.5 WebEngine (C++)
- **AI inference**: llama.cpp, with Qwen2-VL-2B-Instruct-GGUF (language + vision projector) and PaddleOCR-VL each running as a local inference server
- **Speech synthesis**: Piper (zh_CN-chaowei-medium) + g2pw polyphone correction, portable Python bundled, no system Python required
- **Bridge**: C FFI

## Download

### Requirements

- Windows 10/11 64-bit
- 8GB+ RAM recommended (each AI server holds ~1.4GB resident; 4GB machines work but are tight)
- ~3.5GB of model files downloaded on first launch (in-app downloader with cancel/retry/skip-existing)

### Install

1. Download `intelnet-browser-setup-*.exe` (~300MB) from [Releases](https://github.com/RUV2005/intelnet-browser/releases)
2. Run the installer
3. On first launch, follow the prompt to download the AI models

Model files are distributed via ModelScope; URLs are listed in `download_models.ps1`.

## Build from source

### Requirements

- Visual Studio 2022 (with "Desktop development with C++")
- Qt 6.5.3 (msvc2019_64, with WebEngine)
- Rust stable-msvc toolchain
- CMake 3.20+
- Inno Setup 6+ (only for building the installer)

Note: MSVC 14.4+ removed `stdext::checked_array_iterator`, which Qt 6.5.3 still references (`error C3861`). Either use Qt 6.8+ or patch the Qt headers as described in `BUILD_GUIDE.md`.

### Steps

```bash
# 1. Build the Rust core
cd rust-core
cargo build --release

# 2. Build the Qt client
cd ../qt-client
build.bat C:\Qt\6.5.3\msvc2019_64

# 3. Package the installer (optional)
cd ..
iscc installer\intelnet-browser.iss
```

See [BUILD_GUIDE.md](BUILD_GUIDE.md) for details.

## Project layout

```
intelnet-browser/
├── qt-client/                # Qt 6 frontend
├── rust-core/                # Rust AI/TTS core
├── installer/                # Inno Setup packaging
├── resources/                # Runtime assets (llama.cpp, g2pW tables, ...)
├── website/                  # Project website
├── download_models.ps1       # Model download script (fallback/debug)
└── setup_portable_python.ps1 # Portable Python packager
```

## Roadmap

- [x] Page summaries / image analysis / Chinese TTS
- [x] Icon-button function recognition
- [x] Local CAPTCHA recognition and reading
- [ ] Complex form explanation and required-field checks (v1.0.0)
- [ ] Real-time webpage captioning for hearing-impaired users (V2)

## Contributing

Issues and pull requests are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md) first.

## License

GPLv3 — see [LICENSE](LICENSE).

## Links

- **Website**: https://guangji.online
- **Issues**: [GitHub Issues](https://github.com/RUV2005/intelnet-browser/issues)

---

<div align="center">
Built with ❤️ for accessibility
</div>
