#!/system/bin/sh
# RootSnitch 启动脚本

MODDIR=${0%/*}
[ -d "$MODDIR" ] || MODDIR=/data/adb/modules/RootSnitch

case "$(getprop ro.product.cpu.abi)" in
  arm64-v8a)           ABI=arm64 ;;
  armeabi-v7a|armeabi) ABI=arm ;;
  x86_64)              ABI=x64 ;;
  x86)                 ABI=x86 ;;
esac

BIN="$MODDIR/main-$ABI"
[ -x "$BIN" ] || BIN="$MODDIR/main"
[ -x "$BIN" ] || { echo "RootSnitch: 找不到可执行文件" >&2; exit 1; }

exec "$BIN"
