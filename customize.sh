SKIPUNZIP=1

# --- 基础文件 ---
unzip -o "$ZIPFILE" 'module.prop' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'service.sh' -d "$MODPATH" >&2

# --- 根据设备架构选择对应的可执行文件 ---
case "$ARCH" in
  arm64) BIN=main-arm64 ;;
  arm)   BIN=main-arm ;;
  x64)   BIN=main-x64 ;;
  x86)   BIN=main-x86 ;;
  *)     abort "! RootSnitch: 不支持的架构: $ARCH" ;;
esac

ui_print "- 检测到架构: $ARCH，安装 $BIN"
unzip -o "$ZIPFILE" "$BIN" -d "$MODPATH" >&2 || abort "! RootSnitch: 无法解压 $BIN"
mv -f "$MODPATH/$BIN" "$MODPATH/main"
chmod 0700 "$MODPATH/main"

# --- 授予通知助手权限（可选，失败不影响使用） ---
pm list package 2>/dev/null | grep -q com.google.android.ext.services && \
  cmd notification allow_assistant com.google.android.ext.services/android.ext.services.notification.Assistant
