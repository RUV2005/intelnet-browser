#!/bin/bash
# CI/CD 状态监控脚本

echo "=== IntelNet Browser CI/CD 监控 ==="
echo ""

# 检查是否安装了 gh
if ! command -v gh &> /dev/null; then
    echo "错误: 未安装 GitHub CLI (gh)"
    echo "请访问: https://cli.github.com/"
    exit 1
fi

# 检查认证状态
if ! gh auth status &> /dev/null; then
    echo "请先登录 GitHub CLI:"
    echo "  gh auth login"
    exit 1
fi

echo "监控仓库: RUV2005/intelnet-browser"
echo "按 Ctrl+C 退出监控"
echo ""

while true; do
    clear
    echo "=== CI/CD 状态 ($(date '+%Y-%m-%d %H:%M:%S')) ==="
    echo ""

    # 获取最近的 workflow runs
    gh run list --repo RUV2005/intelnet-browser --limit 5

    echo ""
    echo "---"
    echo "每 30 秒刷新一次..."
    sleep 30
done
