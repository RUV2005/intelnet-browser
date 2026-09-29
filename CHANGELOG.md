# IntelNet Browser - Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Git version control initialization
- Comprehensive `.gitignore` for build artifacts
- GitHub Actions CI/CD pipeline
  - Automated Rust core builds
  - Automated Qt client builds
  - Rust formatting and linting checks
- Issue templates (bug report, feature request)
- Contributing guidelines (CONTRIBUTING.md)
- MIT License
- Model download script (download_models.ps1)
- Enhanced error handling structures
- Rust unit tests for FFI layer
- Performance benchmarks with criterion
- Automated release workflow
- Enhanced README with badges and quick start guide

### Changed
- Improved README structure and documentation
- Enhanced project organization

### Fixed
- N/A

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

[Unreleased]: https://github.com/yourusername/intelnet/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/yourusername/intelnet/releases/tag/v0.1.0
