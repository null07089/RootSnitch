# RootSnitch

当 root 进程联网时弹通知的 Magisk 模块。

[![release](https://img.shields.io/github/v/release/null07089/RootSnitch?label=release)](https://github.com/null07089/RootSnitch/releases)
[![license](https://img.shields.io/github/license/null07089/RootSnitch?label=license)](LICENSE)

拿到 root 之后，任何进程都能悄悄联网。RootSnitch 常驻后台，盯住处于 `su` / `ksu` / `magisk` 域的 root 进程：只要它打开了 socket，就弹一条通知，附上 PID 和命令行。

## 功能

- 按 SELinux 域识别 root 进程，而不是靠进程名猜
- 读取 `/proc/<pid>/fd`，有 `socket:[...]` 才算联网
- 通知里带 PID 和完整命令行
- 同一进程只提醒一次，进程退出后可再次提醒
- 支持 `arm64` / `arm` / `x86_64` / `x86`
- 支持 Magisk 在线更新

## 原理

每秒扫一遍 `/proc`：

1. 只看 `uid=0` 的进程
2. 读 `attr/current`，命中 `su` / `ksu` / `magisk` 域
3. 遍历 `fd`，存在 `socket:[...]` 即判定联网
4. fork 一个降到 `shell`(2000) 的子进程调用 `cmd notification post` 发通知

轮询间隔由 `main.cpp` 里的 `LOOP_SLEEP_SEC` 控制，默认 1 秒。

## 安装

到 [Releases](https://github.com/null07089/RootSnitch/releases) 下载 `RootSnitch-vX.Y.zip`，在 Magisk / KernelSU 里本地安装后重启。Android 13+ 记得允许通知。

## 暂停 / 恢复

最简单的办法是**直接关掉 Magisk 里 RootSnitch 的模块开关**：开关会写一个 `disable` 文件，正在运行的 RootSnitch 检测到后立即停止监控；重新打开开关它又自动恢复，全程不用重启。

也可以手动操作：

```sh
touch /data/adb/modules/RootSnitch/disable   # 暂停
rm /data/adb/modules/RootSnitch/disable      # 恢复
```

## 自己编译

需要 Android NDK（示例 r26b）。

```sh
export NDK=/path/to/android-ndk-r26b
bash scripts/build.sh
```

产物是 `build/main-<abi>`。打包为可刷入 zip 的完整流程见 `.github/workflows/release.yml`。

## 目录

```
RootSnitch/
├── main.cpp          # 监控程序源码
├── service.sh        # 开机启动，按 ABI 选择并运行二进制
├── customize.sh      # 安装时选架构、授权通知助手
├── module.prop       # 模块信息（含 updateJson）
├── update.json       # Magisk 在线更新清单
├── scripts/build.sh  # 多架构编译脚本
├── META-INF/         # Magisk 安装引导
└── .github/          # CI 与发布工作流
```

## 在线更新

`module.prop` 的 `updateJson` 指向仓库里的 `update.json`。发新版时改 `module.prop` 的 `version`/`versionCode`，提交后打一个同名 tag（如 `v1.1`）推送，GitHub Actions 会自动编译所有架构、发布 Release，并把 `update.json` 更新回 `main`。

## 许可

[GPL-3.0](LICENSE)。仅用于安全审计与学习，请勿用于非法用途。
