# IntelNet Browser

<div align="center">

![Build Status](https://img.shields.io/github/actions/workflow/status/yourusername/intelnet/build.yml?branch=main)
![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Qt Version](https://img.shields.io/badge/Qt-6.8%2B-green.svg)
![Rust Version](https://img.shields.io/badge/rust-1.75%2B-orange.svg)

基于 Qt WebEngine（Chromium）的无障碍 AI 浏览器

[功能特性](#功能特性) • [快速开始](#快速开始) • [构建指南](#构建和运行) • [贡献指南](CONTRIBUTING.md)

</div>

---

## 简介

IntelNet Browser 是一款专为视障用户设计的智能浏览器，集成了本地 AI 理解和语音合成功能。当前主线客户端使用 Qt 6 + C++，Rust 核心负责 Qwen2-VL 页面/图像理解和 Piper 语音合成。

## 功能特性

- 🌐 完整的浏览器功能（导航、前进、后退、刷新）
- 🤖 基于 Qwen2-VL 的本地 AI 理解
- 📄 直接读取网页 DOM 结构生成页面摘要，不依赖 OCR
- 🖼️ 点击网页图片或上传本地图片进行分析
- 🔊 Piper 中文语音合成与朗读控制
- ♿ 键盘快捷键、焦点提示、大触控区域和高对比度设计
- 🧭 Chrome / Edge 风格的地址栏、导航栏和 AI 助手面板

## 技术栈

- **客户端**: Qt 6 + C++17
- **浏览器引擎**: Qt WebEngine（Chromium）
- **核心能力**: Rust `cdylib` + C FFI
- **视觉模型**: Qwen2-VL + llama.cpp server
- **语音合成**: Piper + 中文模型

## 快速开始

### 前置要求

确保你已安装以下工具：

| 工具 | 版本要求 | 用途 |
|------|---------|------|
| [Visual Studio 2022](https://visualstudio.microsoft.com/) | 2022+ | C++ 编译器和 CMake |
| [Rust](https://rustup.rs/) | 1.75+ (stable-msvc) | 核心库构建 |
| [Qt](https://www.qt.io/download) | 6.8+ | UI 框架和浏览器引擎 |
| Windows WebView2 Runtime | 最新 | 系统自带或自动安装 |

### 安装步骤

1. **克隆仓库**

   ```bash
   git clone https://github.com/yourusername/intelnet.git
   cd intelnet
   ```

2. **下载 AI 模型**

   ```powershell
   # 运行模型下载脚本（需要手动下载大文件）
   .\download_models.ps1
   ```

   或手动下载：
   - Qwen2-VL 模型 → 放入 `resources/models/`
   - Piper TTS 模型 → 放入 `resources/piper/`

3. **构建 Rust 核心**

   ```bash
   cd rust-core
   cargo build --release
   cd ..
   ```

4. **构建 Qt 客户端**

   ```bash
   cd qt-client
   build.bat C:\Qt\6.8.0\msvc2022_64
   ```

5. **运行**

   ```bash
   .\qt-client\build\Release\IntelNetBrowser.exe
   ```

详细构建说明请参见 [BUILD_GUIDE.md](BUILD_GUIDE.md)。

## 使用说明

### 浏览器功能

1. 在地址栏输入网址，按 Enter 或点击 Go 按钮
2. 使用导航按钮（后退、前进、刷新）
3. 正常浏览网页

### AI 页面与图像分析

1. 点击工具栏的“语音助手”打开 AI 面板。
2. “页面摘要”会直接读取当前页面的 URL、标题、正文、标题层级、图片和链接等 DOM 信息。
3. “图片分析”可在网页中点击图片；“上传图片”可选择本地文件。
4. AI 结果会显示在面板中，并自动通过 Piper 朗读。

## 项目结构

```
intelnet/
├── qt-client/           # 主客户端：Qt 原生 UI + Chromium
│   ├── src/              # 主窗口、浏览器、AI 面板、FFI 桥接
│   ├── resources/        # QSS 主题和 SVG 图标
│   ├── CMakeLists.txt
│   └── build.bat
├── rust-core/           # Rust AI/TTS 核心库
└── resources/            # llama.cpp、Qwen2-VL、Piper 运行资源
```

## 注意事项

1. 首次使用 AI 功能需要加载本地 Qwen2-VL 模型，可能需要较长时间。
2. CPU 推理速度取决于机器配置；建议至少 8GB RAM。
3. 模型和运行资源位于项目根目录 `resources`，Qt/Rust 核心会自动查找。

## 优化建议

如果想提升 AI 性能，可以替换为更合适的量化模型，或在 llama.cpp server 层启用 GPU offload。

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！
