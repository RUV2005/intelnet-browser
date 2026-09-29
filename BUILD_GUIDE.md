# IntelNet 无障碍浏览器 - Qt + Rust 版本

## 项目架构

```
IntelNet Browser
├── qt-client/          # Qt C++ 前端（原生 UI）
│   ├── src/
│   │   ├── main.cpp              # 程序入口
│   │   ├── mainwindow.h/cpp      # 主窗口
│   │   ├── browser_widget.h/cpp  # 浏览器组件
│   │   ├── voice_panel.h/cpp     # 语音助手面板
│   │   └── rust_bridge.h/cpp     # Rust FFI 桥接
│   └── CMakeLists.txt
│
└── rust-core/          # Rust 核心库（AI + TTS）
    ├── src/
    │   ├── lib.rs      # C FFI 接口
    │   ├── ai.rs       # AI 图片分析
    │   └── tts.rs      # TTS 语音合成
    └── Cargo.toml
```

## 技术栈

- **前端**: Qt 6 (C++) - QWebEngineView
- **后端**: Rust - AI 模型 + TTS
- **通信**: C FFI (Foreign Function Interface)
- **构建**: CMake + Cargo

## 环境要求

### Windows 开发环境

1. **Visual Studio 2022** (或 2019)
   - 安装 "使用 C++ 的桌面开发" 工作负载
   - 包含 CMake 工具

2. **Qt 6**
   - 下载: https://www.qt.io/download
   - 选择 Qt 6.5+ (包含 WebEngine 模块)
   - 安装路径: `C:\Qt\6.5.3\msvc2022_64`

3. **Rust**
   - 下载: https://rustup.rs/
   - 安装 MSVC 工具链: `rustup default stable-msvc`

4. **CMake** (通常 Visual Studio 已包含)
   - 版本: 3.21+

## 构建步骤

### 1. 构建 Rust 核心库

```bash
cd rust-core
cargo build --release
```

这会生成：
- Windows: `target/release/intelnet_core.dll` 和 `intelnet_core.dll.lib`
- Linux: `target/release/libintelnet_core.so`
- macOS: `target/release/libintelnet_core.dylib`

### 2. 构建 Qt 客户端

#### 方式 A: 使用 Visual Studio

1. 打开 Visual Studio 2022
2. 菜单: `文件` → `打开` → `CMake...`
3. 选择 `qt-client/CMakeLists.txt`
4. 等待 CMake 配置完成
5. 设置 Qt 路径（如果需要）:
   - CMake 设置 → 添加变量: `CMAKE_PREFIX_PATH = C:\Qt\6.5.3\msvc2022_64`
6. 生成 → 全部生成
7. 调试 → 开始执行（不调试）

#### 方式 B: 使用命令行

```bash
cd qt-client
mkdir build
cd build

# 配置 CMake（设置 Qt 路径）
cmake .. -DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2022_64

# 构建
cmake --build . --config Release

# 运行
Release/IntelNetBrowser.exe
```

## Visual Studio 项目设置

### 打开项目

1. 启动 Visual Studio 2022
2. 选择 "打开本地文件夹"
3. 选择 `qt-client` 目录
4. VS 会自动识别 CMakeLists.txt

### 配置 CMake 设置

在 `CMakeSettings.json` 中添加：

```json
{
  "configurations": [
    {
      "name": "x64-Release",
      "generator": "Ninja",
      "configurationType": "Release",
      "buildRoot": "${projectDir}\\out\\build\\${name}",
      "installRoot": "${projectDir}\\out\\install\\${name}",
      "cmakeCommandArgs": "-DCMAKE_PREFIX_PATH=C:/Qt/6.5.3/msvc2022_64",
      "buildCommandArgs": "",
      "ctestCommandArgs": "",
      "inheritEnvironments": [ "msvc_x64_x64" ]
    }
  ]
}
```

### 调试设置

在 `launch.vs.json` 中添加：

```json
{
  "version": "0.2.1",
  "defaults": {},
  "configurations": [
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "IntelNetBrowser.exe",
      "name": "IntelNetBrowser.exe"
    }
  ]
}
```

## 功能特性

### 已实现

✅ 原生 C++ UI（不依赖 HTML/CSS/JS）
✅ Qt WebEngine 浏览器内核
✅ 工具栏（后退、前进、刷新、URL输入）
✅ 语音助手面板（浮动窗口）
✅ Rust FFI 桥接层
✅ 图片分析功能（调用 Rust AI）
✅ TTS 语音合成（调用 Rust TTS）

### 待完成

✅ 页面摘要分析（已实现：直接读取页面结构化源码 → AI 摘要 → 朗读）
✅ 网页图片选择功能（已实现：点击页面图片进行分析）
⬜ 键盘快捷键优化
⬜ 无障碍功能增强
⬜ 设置面板

## 常见问题

### Q: 找不到 Qt

**A:** 确保设置了 `CMAKE_PREFIX_PATH`:

```bash
set CMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2022_64
```

### Q: Rust 库链接失败

**A:** 检查 Rust 库是否编译成功:

```bash
cd rust-core
cargo build --release
dir target\release\intelnet_core.dll
```

### Q: Visual Studio 无法识别 CMake 项目

**A:** 确保安装了 "使用 C++ 的桌面开发" 工作负载，包含 CMake 工具。

### Q: 运行时找不到 DLL

**A:** CMake 会自动复制 DLL，但如果手动运行需要：

```bash
copy rust-core\target\release\intelnet_core.dll qt-client\build\Release\
```

### Q: 编译时报 `error C3861: "stdext": 找不到标识符`（qvarlengtharray.h）

**A:** MSVC 14.4+（VS 2022 17.8 及更高版本，含 VS 2026 的 14.51）的 STL
移除了 `stdext::checked_array_iterator`，而 Qt 6.5.3 仍引用它。两种解决办法：

1. **推荐**：改用 Qt 6.8 或更高版本（已修复该问题）。
2. 或对 Qt 6.5.3 的头文件打补丁：
   `C:\Qt\6.5.3\msvc2019_64\include\QtCore\qcompilerdetection.h`
   把 `stdext::make_unchecked_array_iterator` /
   `stdext::make_checked_array_iterator` 两行定义包进
   `#if _MSC_VER < 1938 ... #endif`（与 Qt 6.8 的做法一致）。

## 下一步开发

1. **完善 TTS 模块** - 实现句子分割和流式朗读
2. **页面分析** - 提取网页文本并生成摘要
3. **图片选择** - 在网页中点击图片进行分析
4. **设置界面** - 允许用户自定义选项
5. **快捷键系统** - 完整的键盘导航支持

## 许可证

MIT License

## 联系方式

- 项目: IntelNet 无障碍浏览器
- 版本: 1.0.0
