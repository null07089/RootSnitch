#!/usr/bin/env bash
#
# Build RootSnitch binaries for every Android ABI supported by the module.
#
# Usage:
#   NDK=/path/to/android-ndk bash scripts/build.sh
#
# Environment:
#   NDK   Android NDK root directory (required)
#   API   Minimum Android API level (default: 21)
#   SRC   Source file to compile (default: main.cpp)
#   OUT   Output directory (default: build)
#
set -euo pipefail

NDK="${NDK:?请设置 NDK 环境变量指向 Android NDK 目录}"
API="${API:-21}"
SRC="${SRC:-main.cpp}"
OUT="${OUT:-build}"
HOST_TAG="linux-x86_64"
BIN_DIR="$NDK/toolchains/llvm/prebuilt/$HOST_TAG/bin"

if [ ! -d "$BIN_DIR" ]; then
  echo "错误: 找不到 NDK 工具链目录 $BIN_DIR" >&2
  exit 1
fi

# ABI  ->  编译器前缀
declare -A COMPILER=(
  [arm64]="aarch64-linux-android${API}-clang++"
  [arm]="armv7a-linux-androideabi${API}-clang++"
  [x86_64]="x86_64-linux-android${API}-clang++"
  [x86]="i686-linux-android${API}-clang++"
)

# ABI  ->  产物文件名（与 customize.sh 中的选择逻辑保持一致）
declare -A OUTPUT=(
  [arm64]="main-arm64"
  [arm]="main-arm"
  [x86_64]="main-x64"
  [x86]="main-x86"
)

# 固定迭代顺序，保证输出可复现
ABIS=(arm64 arm x86_64 x86)

mkdir -p "$OUT"

for abi in "${ABIS[@]}"; do
  echo ">> 正在编译 [$abi] ${OUTPUT[$abi]}"
  "$BIN_DIR/${COMPILER[$abi]}" \
    -O2 \
    -s \
    -static-libstdc++ \
    -o "$OUT/${OUTPUT[$abi]}" \
    "$SRC"
done

echo "完成，产物位于 $OUT/"
