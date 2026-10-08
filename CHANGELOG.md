# IntelNet Browser - Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Windows 安装包（Inno Setup，一键安装，约 300MB）
- 应用内首次启动模型下载器（Qt 进度弹窗，约 3.5GB，支持取消/重试/跳过已存在文件）
- PaddleOCR-VL 验证码识别（独立 llama-server 推理槽位）
- 语义不明按钮功能识别
- llama.cpp b11193 内置，随安装包分发（R2 直链，CI 自动下载）
- 便携 Python：TTS 链路不再依赖系统 Python
- CI 自动构建安装包、打 GitHub Release 全流程

### Fixed
- 安装包写入 Program Files 下模型目录的权限问题（models/、piper/ 加 users-modify）
- llama-server.exe 未打进安装包
- TTS 多音字校正（g2pw）按进程工作目录解析字表导致无声
- 模型下载脚本与 tts.rs 的 Piper 语音文件名不一致（chaowei.onnx vs zh_CN-chaowei-medium.onnx）
- 双 llama-server 日志分离、随机端口分配

### Changed
- 开源许可证由 MIT 变更为 GPLv3

## [0.1.0] - 2024-09-28

### Added
- Initial release
- Qt 6 WebEngine browser core
- Rust AI/TTS core library with C FFI
- Qwen2-VL image analysis integration
- Piper Chinese TTS integration
- Page summary feature (DOM-based, non-OCR)
- Web image selection and analysis
- Accessibility features (keyboard shortcuts, focus indicators, high contrast)
- Complete build system (CMake + Cargo)
- Comprehensive build documentation

[Unreleased]: https://github.com/RUV2005/intelnet-browser/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/RUV2005/intelnet-browser/releases/tag/v0.1.0
