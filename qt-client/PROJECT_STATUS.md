# IntelNet 无障碍浏览器 - Qt C++ + Rust 版本

## 项目状态：✅ 代码完成，待编译测试

所有源代码已经编写完成，Rust 核心库已成功编译。Qt C++ 客户端代码已完成，等待在有完整 Qt 和 C++ 编译环境的系统上编译。

---

## 已完成的工作

### ✅ 1. Rust 核心库 (100% 完成)

**位置**: `rust-core/`

**文件清单**:
- ✅ `Cargo.toml` - 依赖配置（已修复 reqwest json 特性）
- ✅ `src/lib.rs` - C FFI 接口层
- ✅ `src/ai.rs` - AI 图片分析模块
- ✅ `src/tts.rs` - TTS 语音合成模块
- ✅ `include/intelnet_core.h` - C 头文件

**编译状态**: ✅ 成功编译
```
Finished `release` profile [optimized] target(s) in 12.57s
```

**生成的库文件**:
- `target/release/intelnet_core.dll` (6.0 MB)
- `target/release/intelnet_core.dll.lib` (3.2 KB)
- `target/release/intelnet_core.lib` (52 MB)

**暴露的 C API**:
```c
int intelnet_init(void);                    // 初始化核心库
int intelnet_init_model(void);              // 初始化 AI 模型
char* intelnet_analyze_image(const char*);  // 分析图片
char* intelnet_summarize_text(const char*); // 总结网页文本
int intelnet_speak(const char*);            // TTS 朗读
void intelnet_stop_speaking(void);          // 停止朗读
void intelnet_free_string(char*);           // 释放字符串
void intelnet_shutdown(void);               // 清理资源
```

---

### ✅ 2. Qt C++ 客户端 (100% 代码完成)

**位置**: `qt-client/`

**文件清单**:

#### 核心文件
- ✅ `CMakeLists.txt` - 完整构建配置
- ✅ `src/main.cpp` - 程序入口
- ✅ `src/mainwindow.h/cpp` - 主窗口（原生 C++ UI）
- ✅ `src/browser_widget.h/cpp` - QWebEngineView 浏览器组件
- ✅ `src/voice_panel.h/cpp` - 语音助手面板（纯 C++ 控件）
- ✅ `src/rust_bridge.h/cpp` - Rust FFI 桥接层

#### 测试文件
- ✅ `src/test_main.cpp` - FFI 接口测试程序
- ✅ `CMakeLists_simple.txt` - 简化版构建配置（用于测试）

**功能特性**:
- 原生 C++ UI（不使用 HTML/CSS/JS）
- Qt WebEngine 浏览器（基于 Chromium）
- 工具栏：后退、前进、刷新、URL 输入
- 语音助手面板（浮动窗口）
- 页面摘要：直接读取页面结构化源码（URL/标题/描述/标题层级/正文/图片/链接）→ AI 生成中文摘要 → 自动朗读（不依赖截图 OCR）
- 网页图片分析：在页面中点击任意图片即可分析
- 本地图片分析
- TTS 语音朗读
- 完整的样式和布局

---

## 项目架构

```
┌──────────────────────────────────────┐
│   Qt C++ 前端 (原生 UI)              │
│   - QMainWindow                      │
│   - QWebEngineView (浏览器)         │
│   - VoicePanel (语音面板)           │
│   - 原生按钮、输入框等控件           │
└────────────┬─────────────────────────┘
             │ C FFI
┌────────────▼─────────────────────────┐
│   Rust 后端 (已编译成功)             │
│   - AI 图片分析 (qwen2vl)           │
│   - TTS 语音合成 (piper)            │
│   - 业务逻辑处理                     │
└──────────────────────────────────────┘
```

---

## 构建要求

### 必需工具

1. **Qt 6** (6.5+)
   - 下载：https://www.qt.io/download
   - 需要模块：Core, Widgets, WebEngineWidgets, Network
   - 建议路径：`C:\Qt\6.5.3\msvc2022_64`

2. **Visual Studio 2022** (或 2019)
   - 工作负载："使用 C++ 的桌面开发"
   - 包含：CMake 工具、MSVC 编译器

3. **Rust** (已安装)
   - 工具链：stable-msvc
   - 核心库已编译完成 ✅

---

## 构建步骤

### 方法 1: Visual Studio 2022（推荐）

1. **打开项目**
   ```
   Visual Studio 2022 → 文件 → 打开 → CMake...
   选择：qt-client/CMakeLists.txt
   ```

2. **配置 CMake**
   - 添加变量：`CMAKE_PREFIX_PATH = C:\Qt\6.5.3\msvc2022_64`
   - 或在 `CMakeSettings.json` 中配置

3. **生成并运行**
   ```
   生成 → 全部生成
   调试 → 开始执行（不调试）
   ```

### 方法 2: 命令行（需要配置环境）

```bash
cd qt-client
mkdir build && cd build

# 配置 CMake
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2022_64 -G "Visual Studio 17 2022"

# 构建
cmake --build . --config Release

# 运行
Release\IntelNetBrowser.exe
```

### 方法 3: 测试 FFI 接口（不需要 Qt）

```bash
cd qt-client
mkdir build && cd build

# 使用简化版 CMake（只测试 Rust FFI）
cmake .. -DCMAKE_TOOLCHAIN_FILE=../CMakeLists_simple.txt

# 编译测试程序
cmake --build . --config Release

# 运行测试
Release\IntelNetBrowser.exe
```

---

## 代码修复记录

### ✅ 已修复的问题

1. **Rust 编译错误**
   - ❌ 问题：`reqwest::blocking::RequestBuilder` 找不到 `json` 方法
   - ✅ 修复：在 `Cargo.toml` 中添加 `features = ["blocking", "json"]`

2. **TTS 初始化错误**
   - ❌ 问题：`TtsPlayer::new()` 不存在
   - ✅ 修复：改用 `TtsPlayer::spawn()`

3. **Qt 代码缺少函数声明**
   - ❌ 问题：`voice_panel.cpp` 中调用了未声明的 `createActionButton`
   - ✅ 修复：在 `voice_panel.h` 中添加函数声明

4. **依赖缺失**
   - ❌ 问题：缺少 `which` 和 `rodio` 依赖
   - ✅ 修复：在 `Cargo.toml` 中添加依赖

---

## 已知限制

### 当前环境（已验证可编译运行）
- ✅ Visual Studio 2026 + MSVC 14.51
- ✅ CMake 4.4
- ✅ Rust 核心库已编译（`intelnet_core.dll`）
- ✅ Qt 6.5.3 (msvc2019_64, 含 WebEngine) 安装于 `C:\Qt\6.5.3\msvc2019_64`
- ✅ Qt 客户端编译成功并通过启动测试：`qt-client/build/Release/IntelNetBrowser.exe`

### MSVC 14.4+ 兼容性说明（重要）
MSVC 14.4 起（VS 2022 17.8+，含 VS 2026 的 14.51）STL 移除了
`stdext::checked_array_iterator`，而 Qt 6.5.3 仍无条件引用它，会导致
`error C3861: "stdext": 找不到标识符`。

Qt 6.8+ 已修复（仅在 `_MSC_VER < 1938` 时使用 `stdext`）。本机对 Qt 6.5.3
的头文件打了同样的补丁：

`C:\Qt\6.5.3\msvc2019_64\include\QtCore\qcompilerdetection.h`
把两行 `stdext::...` 定义包进 `#if _MSC_VER < 1938 ... #endif`。
若重装/升级 Qt，请改用 Qt 6.8+ 以避免该补丁。

### 功能限制
- ⚠️ AI 模型和 TTS 模型文件位于项目根目录 `resources`，rust-core 会自动查找
- ⚠️ TTS 需要系统安装 `python`（piper 通过 python 调用）

---

## 下一步操作

### 安装 Qt 后即可编译：

1. **安装 Qt 6**
   - 下载 Qt 在线安装器
   - 选择 Qt 6.5.3 for MSVC 2022（需勾选 WebEngine 模块）

2. **一键构建（推荐）**
   ```bat
   cd qt-client
   build.bat C:\Qt\6.5.3\msvc2022_64
   ```
   或先设置 `QTDIR` 环境变量后直接运行 `build.bat`。

3. **用 Visual Studio 打开**
   - 用 Visual Studio 打开 `qt-client/CMakeLists.txt`
   - 在 `CMakeSettings.json` 中把 `CMAKE_PREFIX_PATH` 改成你的 Qt 路径

4. **测试功能**
   - 浏览器导航
   - 语音助手面板
   - 页面摘要（需要模型文件）
   - 网页图片分析（需要模型文件）
   - 本地图片分析（需要模型文件）
   - TTS 朗读（需要模型文件）

---

## 项目文件结构

```
intelnet/
├── rust-core/                    # Rust 核心库 ✅
│   ├── Cargo.toml               # 已修复 ✅
│   ├── src/
│   │   ├── lib.rs               # C FFI 接口 ✅
│   │   ├── ai.rs                # AI 模块 ✅
│   │   └── tts.rs               # TTS 模块 ✅
│   ├── include/
│   │   └── intelnet_core.h      # C 头文件 ✅
│   └── target/release/           # 编译产物 ✅
│       ├── intelnet_core.dll
│       └── intelnet_core.dll.lib
│
├── qt-client/                    # Qt C++ 客户端 ✅
│   ├── CMakeLists.txt           # 完整构建配置 ✅
│   ├── CMakeLists_simple.txt    # 简化测试配置 ✅
│   └── src/
│       ├── main.cpp             # 程序入口 ✅
│       ├── test_main.cpp        # FFI 测试 ✅
│       ├── mainwindow.h/cpp     # 主窗口 ✅
│       ├── browser_widget.h/cpp # 浏览器 ✅
│       ├── voice_panel.h/cpp    # 语音面板 ✅
│       └── rust_bridge.h/cpp    # FFI 桥接 ✅
│
├── resources/                    # llama.cpp、Qwen2-VL、Piper 运行资源
├── BUILD_GUIDE.md                # 构建指南 ✅
└── PROJECT_STATUS.md             # 本文件 ✅
```

---

## 技术亮点

### ✨ 架构优势
- **纯 C++ 原生 UI**：不依赖 HTML/CSS/JS，性能更好
- **现代浏览器内核**：Qt WebEngine (Chromium)
- **强大的后端能力**：保留 Rust AI 和 TTS 功能
- **清晰的分层**：UI 层与业务逻辑完全分离

### ✨ 开发体验
- **Visual Studio 原生支持**：可用 VS 2022 直接打开和调试
- **CMake 构建系统**：跨平台、标准化
- **C FFI 接口**：稳定、高效、易于维护

---

## 总结

✅ **项目代码 100% 完成**
✅ **Rust 核心库编译成功**
✅ **Qt C++ 代码结构完整**
⏳ **等待在完整环境中编译测试**

所有代码已经过仔细检查和修复，理论上应该可以直接编译运行。如果在编译过程中遇到问题，请参考 `BUILD_GUIDE.md` 中的故障排除部分。

---

## 联系信息

- 项目名称：IntelNet 无障碍浏览器
- 版本：1.0.0 (Qt + Rust 版)
- 技术栈：Qt 6, C++17, Rust, WebEngine
- 构建系统：CMake
- 许可证：MIT

---

*最后更新：2024-09-28*
