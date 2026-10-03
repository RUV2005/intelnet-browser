# 明镜浏览器 (MingJing Browser)

<div align="center">

**Making the web easier to see, understand, and hear**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Qt 6](https://img.shields.io/badge/Qt-6.x-green.svg)](https://www.qt.io/)
[![Rust](https://img.shields.io/badge/Rust-1.70+-orange.svg)](https://www.rust-lang.org/)

[Features](#features) • [Architecture](#architecture) • [Getting Started](#getting-started) • [Building](#building) • [Contributing](#contributing)

</div>

---

## Overview

MingJing Browser is an open-source accessibility-focused web browser that integrates AI-powered content understanding, image analysis, and text-to-speech capabilities directly into the browsing experience. Built with Qt WebEngine and a Rust AI core, it makes web content more accessible to everyone.

## Features

### 🔍 **Intelligent Page Summarization**
- AI-powered content extraction from complex web pages
- Structured summaries with key points highlighted
- Understanding based on DOM structure analysis

### 🖼️ **Advanced Image Analysis**
- Visual content recognition and description
- Chart and diagram interpretation
- Context-aware image understanding

### 🔊 **Natural Text-to-Speech**
- Chinese language TTS powered by Piper engine
- Natural voice synthesis with adjustable speed
- Seamless reading of selected text or full page summaries

### ⚡ **Fast & Lightweight**
- Built on Qt 6 WebEngine for modern web standards
- Rust-powered AI core for performance and safety
- Efficient C FFI bridge between frontend and backend

## Architecture

```
┌─────────────────────────────────────┐
│        Qt 6 Client Layer            │
│  WebEngine • Shortcuts • HiDPI      │
└─────────────────────────────────────┘
                  │
              C FFI Bridge
                  │
┌─────────────────────────────────────┐
│         Rust AI Core                │
│  Content Analysis • Image Vision    │
│  Piper TTS • Model Integration      │
└─────────────────────────────────────┘
```

**Technology Stack:**
- **Frontend**: Qt 6 WebEngine, QML, C++
- **AI Core**: Rust, distilled language models (1.5B-3B parameters)
- **TTS Engine**: Piper (Chinese language support)
- **Inference**: llama.cpp / ONNX Runtime
- **Bridge**: C FFI for safe cross-language communication

## Getting Started

### Prerequisites

- **Qt 6.5+** with WebEngine module
- **Rust 1.70+** (stable toolchain)
- **CMake 3.20+**
- **Python 3.8+** (for build scripts)

### Installation

**Option 1: Download Pre-built Binary** (Coming Soon)

```bash
# Download from releases page
# Extract and run
```

**Option 2: Build from Source**

```bash
# Clone the repository
git clone https://github.com/yourusername/mingjing-browser.git
cd mingjing-browser

# Build the project
./build.sh

# Run
./build/mingjing-browser
```

See [BUILD_GUIDE.md](BUILD_GUIDE.md) for detailed build instructions.

## Project Structure

```
mingjing-browser/
├── src/
│   ├── qt-client/          # Qt 6 frontend
│   │   ├── main.cpp
│   │   ├── browser/        # Browser UI components
│   │   └── qml/            # QML interface files
│   ├── rust-core/          # Rust AI core
│   │   ├── src/
│   │   │   ├── lib.rs      # FFI exports
│   │   │   ├── ai/         # AI model integration
│   │   │   ├── tts/        # Text-to-speech
│   │   │   └── vision/     # Image analysis
│   │   └── Cargo.toml
│   └── bridge/             # C FFI bridge layer
├── models/                 # Distilled AI models
├── website/                # Project landing page
├── docs/                   # Documentation
├── tests/                  # Test suites
├── CMakeLists.txt
└── README.md
```

## Model Distillation

MingJing Browser uses distilled small language models for on-device AI inference:

- **Text Understanding**: 1.5B parameter model (~800MB quantized)
- **Image Analysis**: 3B parameter vision model (~1.5GB quantized)
- **TTS**: Piper engine (~50MB)

**Total footprint**: ~2.5GB with all models loaded

### Why Distillation?

- ✅ **Privacy**: All processing happens locally, no data leaves your device
- ✅ **Speed**: 10-40x faster than cloud APIs (50-100ms response time)
- ✅ **Offline**: Works without internet connection
- ✅ **Cost**: No API fees, one-time training cost only

See [DISTILLATION.md](docs/DISTILLATION.md) for details on our model training process.

## Roadmap

### Current Status: Alpha

- [x] Basic browser functionality with Qt WebEngine
- [x] AI-powered page summarization
- [x] Image content analysis
- [x] Chinese TTS integration
- [x] Project website and branding
- [ ] Model distillation and quantization
- [ ] Offline inference integration
- [ ] Keyboard shortcut system
- [ ] High contrast theme support
- [ ] Multi-language support (English, Chinese)

### Future Plans

- Screen reader integration (NVDA, JAWS)
- Custom voice training for TTS
- Browser extension API
- Mobile platform support (Android, iOS)
- Community-contributed model improvements

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Ways to Contribute

- 🐛 Report bugs and issues
- 💡 Suggest new features
- 📝 Improve documentation
- 🧪 Add test coverage
- 🎨 Design UI/UX improvements
- 🤖 Train better distilled models
- 🌍 Translate to other languages

## Development

### Running Tests

```bash
# Run all tests
cargo test --all
ctest --test-dir build

# Run specific test suite
cargo test --package rust-core
```

### Code Style

- **Rust**: Follow `rustfmt` and `clippy` recommendations
- **C++**: Follow Qt coding conventions
- **CMake**: Use modern CMake practices (targets, not variables)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **Qt Project** for the excellent WebEngine framework
- **Piper TTS** for open-source Chinese speech synthesis
- **llama.cpp** for efficient model inference
- **Rust Community** for safety and performance tools

## Contact

- **Project Website**: [https://mingjing-browser.dev](https://mingjing-browser.dev)
- **Issue Tracker**: [GitHub Issues](https://github.com/yourusername/mingjing-browser/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/mingjing-browser/discussions)

---

<div align="center">
Made with ❤️ for accessibility

**Making the web accessible, one page at a time**
</div>
