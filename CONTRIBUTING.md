# Contributing to IntelNet Browser

感谢你考虑为 IntelNet Browser 做贡献！

## 如何贡献

### 报告 Bug

如果你发现了 bug，请创建一个 issue 并包含以下信息：

1. **环境信息**
   - 操作系统版本
   - Qt 版本
   - Rust 版本
   - Visual Studio 版本

2. **复现步骤**
   - 详细的操作步骤
   - 期望的行为
   - 实际发生的行为

3. **日志和截图**
   - 相关的错误日志
   - 截图（如果适用）

### 提交功能请求

如果你有新功能的想法：

1. 先检查是否已有相关的 issue
2. 创建新 issue 并描述：
   - 功能的用途和场景
   - 预期的实现方式
   - 对用户的价值

### Pull Request 流程

1. **Fork 仓库**
   ```bash
   git clone https://github.com/RUV2005/intelnet-browser.git
   cd intelnet-browser
   ```

2. **创建分支**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **编写代码**
   - 遵循现有的代码风格
   - 添加必要的测试
   - 更新相关文档

4. **运行测试**
   ```bash
   # Rust 测试
   cd rust-core
   cargo test
   cargo clippy
   cargo fmt --check
   
   # Qt 构建测试
   cd ../qt-client
   ./build.bat
   ```

5. **提交代码**
   ```bash
   git add .
   git commit -m "feat: add your feature description"
   ```

6. **推送并创建 PR**
   ```bash
   git push origin feature/your-feature-name
   ```

## 代码规范

### Rust 代码

- 使用 `cargo fmt` 格式化代码
- 运行 `cargo clippy` 检查代码质量
- 为公共 API 添加文档注释
- 编写单元测试

### C++ 代码

- 遵循 Qt 编码规范
- 使用智能指针管理内存
- 添加必要的注释
- 保持函数简短清晰

### 提交信息规范

使用约定式提交（Conventional Commits）：

- `feat:` 新功能
- `fix:` Bug 修复
- `docs:` 文档更新
- `style:` 代码格式调整
- `refactor:` 重构
- `test:` 测试相关
- `chore:` 构建/工具链相关

示例：
```
feat: add page summary feature
fix: memory leak in image analysis
docs: update build instructions
```

## 开发环境设置

### 必需工具

1. **Rust**
   ```bash
   curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
   rustup default stable-msvc
   ```

2. **Qt 6**
   - 下载：https://www.qt.io/download
   - 需要组件：Core, Widgets, WebEngineWidgets, Network

3. **Visual Studio 2022**
   - 需要工作负载：桌面 C++ 开发
   - 需要组件：MSVC, CMake

### 构建步骤

参见 [BUILD_GUIDE.md](BUILD_GUIDE.md)

## 获取帮助

- 创建 issue 提问
- 查看现有文档
- 参与讨论

## 行为准则

- 尊重所有贡献者
- 提供建设性反馈
- 专注于问题本身，而非个人
- 保持友善和专业

## 许可证

提交代码即表示你同意将代码以 GPLv3 许可证授权。
