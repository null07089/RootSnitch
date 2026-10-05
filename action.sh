#!/system/bin/sh
# RootSnitch - 模块“操作”按钮脚本（KernelSU / APatch）
# 1) 解除用于隐藏进程的 /proc/<pid> 挂载
# 2) 列出处于 su / ksu / magisk 域且持有 socket（已联网）的进程

echo "== 解除隐藏进程挂载 =="
awk '/\/proc\/[0-9]+/ {print $2}' /proc/mounts | while read -r mnt; do
    [ -n "$mnt" ] || continue
    echo "umount -l $mnt"
    umount -l "$mnt"
done

echo
echo "== 持有 socket 的 root 进程 =="
found=0
for d in /proc/[0-9]*; do
    pid=${d#/proc/}
    label=$(cat "$d/attr/current" 2>/dev/null)
    case "$label" in
        u:r:su:s0*|u:r:ksu:s0*|u:r:magisk:s0*) ;;
        *) continue ;;
    esac

    net=0
    for f in "$d"/fd/*; do
        link=$(readlink "$f" 2>/dev/null)
        case "$link" in
            socket:\[*\]) net=1; break ;;
        esac
    done
    [ "$net" = 1 ] || continue

    args=$(tr '\0' ' ' < "$d/cmdline" 2>/dev/null)
    echo "$pid  ${args% }"
    found=1
done
[ "$found" = 1 ] || echo "（未发现联网的 root 进程）"
