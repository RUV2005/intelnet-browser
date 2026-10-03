# 明镜浏览器

<div align="center">

**让网页更容易被看见、理解和听见**

[![许可证: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Qt 6](https://img.shields.io/badge/Qt-6.x-green.svg)](https://www.qt.io/)
[![Rust](https://img.shields.io/badge/Rust-1.70+-orange.svg)](https://www.rust-lang.org/)

[功能特性](#功能特性) • [技术架构](#技术架构) • [快速开始](#快速开始) • [构建指南](#构建指南) • [参与贡献](#参与贡献)

</div>

---

## 项目简介

明镜浏览器是一款开源的无障碍访问浏览器，将 AI 驱动的内容理解、图像分析和语音朗读功能直接集成到浏览体验中。采用 Qt WebEngine 前端和 Rust AI 核心构建，让 Web 内容对所有人更加友好。

## 功能特性

### 🔍 **智能页面摘要**
- AI 驱动的复杂网页内容提取
- 结构化摘要，突出关键要点
- 基于 DOM 结构的深度理解

### 🖼️ **高级图像分析**
- 视觉内容识别与描述
- 图表和图示解读
- 上下文感知的图像理解

### 🔊 **自然语音朗读**
- 基于 Piper 引擎的中文 TTS
- 自然语音合成，可调节语速
- 无缝朗读选中文本或完整页面摘要

### ⚡ **快速轻量**
- 基于 Qt 6 WebEngine，支持现代 Web 标准
- Rust 驱动的 AI 核心，兼顾性能与安全
- 高效的 C FFI 桥接前后端

## 技术架构

```
┌─────────────────────────────────────┐
│        Qt 6 客户端层                 │
│  WebEngine • 快捷键 • 高对比度       │
└─────────────────────────────────────┘
                  │
              C FFI 桥接
                  │
┌─────────────────────────────────────┐
│         Rust AI 核心                │
│  内容分析 • 图像视觉 • Piper TTS    │
└─────────────────────────────────────┘
```

**技术栈：**
- **前端**: Qt 6 WebEngine, QML, C++
- **AI 核心**: Rust, 蒸馏语言模型 (1.5B-3B 参数)
- **TTS 引擎**: Piper（中文语音支持）
- **推理引擎**: llama.cpp / ONNX Runtime
- **桥接层**: C FFI 实现安全的跨语言通信

## 快速开始

### 环境要求

- **Qt 6.5+** 包含 WebEngine 模块
- **Rust 1.70+** (stable 工具链)
- **CMake 3.20+**
- **Python 3.8+** (用于构建脚本)

### 安装方式

**方式 1: 下载预编译版本**（即将发布）

```bash
# 从发布页面下载
# 解压并运行
```

**方式 2: 从源码构建**

```bash
# 克隆仓库
git clone https://github.com/yourusername/mingjing-browser.git
cd mingjing-browser

# 构建项目
./build.sh

# 运行
./build/mingjing-browser
```

详细构建说明请查看 [BUILD_GUIDE.md](BUILD_GUIDE.md)

## 项目结构

```
mingjing-browser/
├── src/
│   ├── qt-client/          # Qt 6 前端
│   │   ├── main.cpp
│   │   ├── browser/        # 浏览器 UI 组件
│   │   └── qml/            # QML 界面文件
│   ├── rust-core/          # Rust AI 核心
│   │   ├── src/
│   │   │   ├── lib.rs      # FFI 导出
│   │   │   ├── ai/         # AI 模型集成
│   │   │   ├── tts/        # 语音合成
│   │   │   └── vision/     # 图像分析
│   │   └── Cargo.toml
│   └── bridge/             # C FFI 桥接层
├── models/                 # 蒸馏 AI 模型
├── website/                # 项目官网
├── docs/                   # 文档
├── tests/                  # 测试套件
├── CMakeLists.txt
└── README.md
```

## 模型蒸馏

明镜浏览器使用蒸馏的小型语言模型进行设备端 AI 推理：

- **文本理解**: 1.5B 参数模型（量化后约 800MB）
- **图像分析**: 3B 参数视觉模型（量化后约 1.5GB）
- **语音合成**: Piper 引擎（约 50MB）

**总内存占用**: 加载所有模型约 2.5GB

### 为什么选择蒸馏？

- ✅ **隐私保护**: 所有处理都在本地进行，数据不离开设备
- ✅ **速度快**: 比云端 API 快 10-40 倍（响应时间 50-100ms）
- ✅ **离线可用**: 无需网络连接即可工作
- ✅ **成本低**: 无 API 费用，仅一次性训练成本

详细的模型训练流程请查看 [DISTILLATION.md](docs/DISTILLATION.md)

## 开发路线图

### 当前状态: Alpha 版本

- [x] 基于 Qt WebEngine 的基础浏览器功能
- [x] AI 驱动的页面摘要
- [x] 图像内容分析
- [x] 中文 TTS 集成
- [x] 项目官网和品牌设计
- [ ] 模型蒸馏和量化
- [ ] 离线推理集成
- [ ] 键盘快捷键系统
- [ ] 高对比度主题支持
- [ ] 多语言支持（英文、中文）

### 未来计划

- 屏幕阅读器集成（NVDA、JAWS）
- TTS 自定义语音训练
- 浏览器扩展 API
- 移动平台支持（Android、iOS）
- 社区贡献的模型改进

## 参与贡献

我们欢迎贡献！请查看 [CONTRIBUTING.md](CONTRIBUTING.md) 了解贡献指南。

### 贡献方式

- 🐛 报告 bug 和问题
- 💡 提出新功能建议
- 📝 改进文档
- 🧪 增加测试覆盖率
- 🎨 设计 UI/UX 改进
- 🤖 训练更好的蒸馏模型
- 🌍 翻译成其他语言

## 开发指南

### 运行测试

```bash
# 运行所有测试
cargo test --all
ctest --test-dir build

# 运行特定测试套件
cargo test --package rust-core
```

### 代码风格

- **Rust**: 遵循 `rustfmt` 和 `clippy` 建议
- **C++**: 遵循 Qt 编码规范
- **CMake**: 使用现代 CMake 实践（targets，而非 variables）

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 致谢

- **Qt Project** 提供优秀的 WebEngine 框架
- **Piper TTS** 开源中文语音合成
- **llama.cpp** 高效模型推理
- **Rust 社区** 安全和性能工具支持

## 联系方式

- **项目官网**: [https://mingjing-browser.dev](https://mingjing-browser.dev)
- **问题追踪**: [GitHub Issues](https://github.com/yourusername/mingjing-browser/issues)
- **讨论区**: [GitHub Discussions](https://github.com/yourusername/mingjing-browser/discussions)

## 演示网站

访问我们的在线演示网站体验 AI 功能：

- **在线试用**: [https://demo.mingjing-browser.dev](http://localhost:5173)
- **页面摘要**: 粘贴文章内容，AI 即时生成结构化摘要
- **图像分析**: 上传图片，AI 描述视觉内容
- **语音朗读**: 输入文字，体验自然 TTS 合成

---

<div align="center">
用 ❤️ 为无障碍而生

**让无障碍成为浏览器的核心能力**
</div>
