#!/bin/bash

shopt -s nullglob
set -e

DIR=$(
  cd "$(dirname "$0")" || exit 1
  pwd
)

# 1. 获取位置参数，如果没有提供则默认为空字符串
# ${1:-} 的意思是：如果 $1 未设置，则返回空
ACTION=${1:-}

# 验证输入（可选）：只允许为空或 "clean"，防止误操作
if [[ -n "$ACTION" && "$ACTION" != "clean" ]]; then
  echo "Usage: $0 [clean]"
  exit 1
fi

arr=(dmt_{ingress,router,egress})

for dir in "${arr[@]}"; do
  echo "Processing $dir..."
  # 使用子 shell (...)
  # 这样 cd 只在括号内有效，结束后自动回到原位，且不会因为一次 cd 失败影响后续
  (
    cd "$dir"
    make ${ACTION}
  ) || {
    echo "Error: Make failed in $dir"
    exit 1
  }
done
