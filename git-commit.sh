#!/bin/bash
# Git 提交辅助脚本

set -e

echo "=== IntelNet Git Commit Helper ==="
echo ""

# 检查是否有未暂存的更改
if [[ -z $(git status -s) ]]; then
    echo "✓ 没有需要提交的更改"
    exit 0
fi

echo "当前更改："
git status -s
echo ""

# 提示用户选择提交类型
echo "请选择提交类型："
echo "1) feat     - 新功能"
echo "2) fix      - Bug 修复"
echo "3) docs     - 文档更新"
echo "4) style    - 代码格式"
echo "5) refactor - 重构"
echo "6) test     - 测试"
echo "7) chore    - 构建/工具"
echo ""
read -p "选择 (1-7): " choice

case $choice in
    1) type="feat" ;;
    2) type="fix" ;;
    3) type="docs" ;;
    4) type="style" ;;
    5) type="refactor" ;;
    6) type="test" ;;
    7) type="chore" ;;
    *) echo "无效选择"; exit 1 ;;
esac

read -p "提交信息: " message

if [[ -z "$message" ]]; then
    echo "错误：提交信息不能为空"
    exit 1
fi

# 暂存所有更改
git add .

# 提交
commit_msg="$type: $message"
git commit -m "$commit_msg"

echo ""
echo "✓ 提交成功: $commit_msg"
echo ""
read -p "是否推送到远程仓库? (y/N): " push

if [[ "$push" == "y" || "$push" == "Y" ]]; then
    git push
    echo "✓ 已推送到远程仓库"
fi
