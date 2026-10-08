# 明镜浏览器

<div align="center">

**让网页更容易被看见、理解和听见**

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Qt 6](https://img.shields.io/badge/Qt-6.5-green.svg)](https://www.qt.io/)
[![Rust](https://img.shields.io/badge/Rust-stable-orange.svg)](https://www.rust-lang.org/)

[功能特性](#功能特性) • [技术架构](#技术架构) • [下载安装](#下载安装) • [从源码构建](#从源码构建) • [参与贡献](#参与贡献)

</div>

---

## 项目简介

明镜浏览器（IntelNet Browser）是一款开源的无障碍浏览器，把 AI 内容理解、图像分析和语音朗读直接做进浏览体验。Qt WebEngine 做前端，Rust 写 AI 核心，**所有推理都在本地完成，数据不出设备**。

## 功能特性

### 📄 页面摘要
读取页面 DOM 结构（标题、标题层级、正文、图片、链接），本地大模型生成中文摘要，可一键朗读。不依赖截图 OCR。

### 🖼️ 图片分析
点击网页上的任意图片，本地视觉模型用中文描述图片内容，图表图示也能解读。

### 🔘 按钮功能识别
语义不明的图标按钮，AI 只看图标样式，用一句话说出它的功能。

### 🔢 验证码朗读
PaddleOCR-VL 本地识别验证码并逐字符朗读；识别失败会明确提示，不会把猜错的结果念出来。

### 🔊 中文语音朗读
Piper 中文语音合成，可调语速；选中的文本或整页摘要都能读。

### ⌨️ 无障碍操作
键盘快捷键、焦点指示、高对比度；安装与模型下载全程可读屏操作。

## 技术架构

```
┌─────────────────────────────────────┐
│            Qt 6 客户端               │
│   WebEngine • 语音面板 • 快捷键      │
└─────────────────────────────────────┘
                  │ C FFI
┌─────────────────────────────────────┐
│            Rust AI 核心              │
│  llama-server ×2 • Piper TTS        │
└─────────────────────────────────────┘
```

- **前端**：Qt 6.5 WebEngine（C++）
- **AI 推理**：llama.cpp，Qwen2-VL-2B-Instruct-GGUF（文本+视觉两件套）与 PaddleOCR-VL 各跑一个本地推理服务
- **语音合成**：Piper（zh_CN-chaowei-medium）+ g2pw 多音字校正，便携 Python 内置，不依赖系统 Python
- **桥接**：C FFI

## 下载安装

### 环境要求

- Windows 10/11 64 位
- 内存建议 8GB 以上（单个 AI 服务常驻约 1.4GB；4GB 机器可运行但偏紧）
- 首次启动需下载约 3.5GB 模型文件（应用内自动下载，支持取消/重试/跳过已存在文件）

### 安装步骤

1. 从 [Releases](https://github.com/RUV2005/intelnet-browser/releases) 下载 `intelnet-browser-setup-*.exe`（约 300MB）
2. 运行安装向导
3. 首次启动时按提示下载 AI 模型，完成后即可使用

模型文件经 ModelScope 分发，下载地址可在 `download_models.ps1` 中查看。

## 从源码构建

### 环境要求

- Visual Studio 2022（含"使用 C++ 的桌面开发"工作负载）
- Qt 6.5.3（msvc2019_64，含 WebEngine 模块）
- Rust stable-msvc 工具链
- CMake 3.20+
- Inno Setup 6+（仅打包安装包时需要）

注意：MSVC 14.4+ 移除了 `stdext::checked_array_iterator`，而 Qt 6.5.3 仍引用它，编译会报 `error C3861`。两种解法：改用 Qt 6.8+，或按 `BUILD_GUIDE.md` 中的说明给 Qt 头文件打补丁。

### 构建步骤

```bash
# 1. 构建 Rust 核心
cd rust-core
cargo build --release

# 2. 构建 Qt 客户端
cd ../qt-client
build.bat C:\Qt\6.5.3\msvc2019_64

# 3. 打包安装包（可选）
cd ..
iscc installer\intelnet-browser.iss
```

详细说明见 [BUILD_GUIDE.md](BUILD_GUIDE.md)。

## 项目结构

```
intelnet-browser/
├── qt-client/                # Qt 6 前端
├── rust-core/                # Rust AI/TTS 核心
├── installer/                # Inno Setup 打包脚本
├── resources/                # 运行资源（llama.cpp、g2pW 字表等）
├── website/                  # 官网
├── download_models.ps1       # 模型下载脚本（备用/调试）
└── setup_portable_python.ps1 # 便携 Python 打包脚本
```

## 开发路线图

- [x] 页面摘要 / 图片分析 / 中文 TTS 朗读
- [x] 语义不明按钮功能识别
- [x] 验证码本地识别与朗读
- [ ] 复杂表单 AI 解释与必填检查（v1.0.0）
- [ ] 网页声音实时字幕（听障场景，V2）

## 参与贡献

欢迎提交 Issue 和 Pull Request，请先阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 许可证

本项目采用 **GPLv3** 许可证，详见 [LICENSE](LICENSE)。

## 联系方式

- **项目官网**：https://guangji.online
- **问题追踪**：[GitHub Issues](https://github.com/RUV2005/intelnet-browser/issues)

---

<div align="center">
用 ❤️ 为无障碍而生
</div>
