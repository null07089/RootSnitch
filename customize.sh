SKIPUNZIP=1

unzip -o "$ZIPFILE" 'module.prop' -d "$MODPATH" >&2
unzip -o "$ZIPFILE" 'service.sh' -d "$MODPATH" >&2

case "$ARCH" in
  arm64) BIN=main-arm64 ;;
  arm)   BIN=main-arm ;;
  x64)   BIN=main-x64 ;;
  x86)   BIN=main-x86 ;;
  *)     abort "! RootSnitch: 不支持的架构 $ARCH" ;;
esac

ui_print "- 架构 $ARCH，安装 $BIN"
unzip -o "$ZIPFILE" "$BIN" -d "$MODPATH" >&2 || abort "! RootSnitch: 解压 $BIN 失败"
chmod 0700 "$MODPATH/$BIN"

# 另建通用名 main，供 service.sh 在 ABI 识别异常时回退
ln -sf "$BIN" "$MODPATH/main" 2>/dev/null || cp -f "$MODPATH/$BIN" "$MODPATH/main"

# 允许系统的通知助手（可选，失败不影响使用）
pm list package 2>/dev/null | grep -q com.google.android.ext.services && \
  cmd notification allow_assistant com.google.android.ext.services/android.ext.services.notification.Assistant
