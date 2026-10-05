# RootSnitch

> 根进程联网告警 —— 当 root 进程悄悄发起网络连接时，第一时间通知你。

[![Release](https://img.shields.io/github/v/release/null07089/RootSnitch?color=blue&label=release)](https://github.com/null07089/RootSnitch/releases)
[![Build](https://github.com/null07089/RootSnitch/actions/workflows/build.yml/badge.svg)](https://github.com/null07089/RootSnitch/actions/workflows/build.yml)
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-orange)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Magisk-brightgreen)](https://github.com/topjohnwu/Magisk)

RootSnitch 是一个轻量级 Magisk 模块。它常驻后台，持续扫描系统中的 root 进程（SELinux 域为 `su` / `ksu` / `magisk`），一旦发现某个 root 进程持有 **socket 文件描述符**（即进行网络通信），就立即通过系统通知发出告警，并附上进程 PID 与命令行，方便你审计「谁在你不知情的时候联网了」。

---

## 目录

- [功能特性](#功能特性)
- [工作原理](#工作原理)
- [系统要求](#系统要求)
- [安装](#安装)
- [使用与配置](#使用与配置)
- [从源码构建](#从源码构建)
- [项目结构](#项目结构)
- [在线更新](#在线更新)
- [常见问题](#常见问题)
- [贡献](#贡献)
- [免责声明](#免责声明)
- [许可证](#许可证)

---

## 功能特性

- **精准识别 root 进程**：通过 `/proc/<pid>/attr/current` 读取 SELinux 上下文，匹配 `u:r:su:s0`、`u:r:ksu:s0`、`u:r:magisk:s0` 三个域。
- **判断真实联网行为**：遍历 `/proc/<pid>/fd`，检测是否存在指向 `socket:[...]` 的文件描述符，而非仅凭进程名猜测。
- **系统通知告警**：以 `shell` 身份调用 `cmd notification post`，弹出版本化的通知，内容包含 PID 与完整命令行。
- **去重与队列控制**：以 `PID + inode` 作为唯一键避免重复告警；维护一个先进先出的队列（默认上限 1024），防止内存无限膨胀。
- **可开关**：在模块目录放置 `disable` 文件即可暂停监控，删除后自动恢复，无需重启模块。
- **多架构支持**：为 `arm64-v8a`、`armeabi-v7a`、`x86_64`、`x86` 分别编译，安装时自动选择。
- **支持在线更新**：通过 `module.prop` 中的 `updateJson` 从 GitHub 拉取新版本。

## 工作原理

核心逻辑位于 [`main.cpp`](main.cpp)，循环流程如下（默认每 1 秒一次）：

1. **枚举进程**：扫描 `/proc` 下所有数字目录。
2. **过滤 root 进程**：`stat("/proc/<pid>")`，只保留 `st_uid == 0` 的进程。
3. **校验 SELinux 上下文**：读取 `attr/current`，命中 `su` / `ksu` / `magisk` 域才继续。
4. **检测 socket**：遍历 `/proc/<pid>/fd`，若存在指向 `socket:[...]` 的软链接，说明该进程持有网络套接字。
5. **去重后告警**：若该 `PID + inode` 组合此前未告警过，则 fork 一个子进程，降权为 `shell`（uid/gid 2000）后执行通知命令：

   ```
   /system/bin/cmd notification post \
     -t "进程联网告警" -S bigtext <tag> "<pid> | <cmdline>"
   ```

6. **休眠**：`sleep(1)` 后进入下一轮；子进程由 `SIGCHLD` 处理函数回收，避免僵尸进程。

> 进程退出后其 `PID + inode` 会从去重表中淘汰，若同一 PID 复用给新进程会被视为新对象。

## 系统要求

| 项目 | 要求 |
| --- | --- |
| Root 方案 | Magisk v20.4+（KernelSU / APatch 等提供 Magisk 兼容模块的也通常可用） |
| Android 版本 | Android 8.0+（依赖 `cmd notification`），API 21+ 编译 |
| 架构 | `arm64-v8a` / `armeabi-v7a` / `x86_64` / `x86` |
| 构建（可选） | Android NDK r26b（其他较新版本亦可） |

> 注意：Android 13 起通知需要用户授权。首次安装后请确认系统未屏蔽模块渠道的通知；模块会在安装时尝试为系统通知助手授权。

## 安装

### 方式一：使用已发布的 zip（推荐）

1. 前往 [Releases](https://github.com/null07089/RootSnitch/releases) 下载最新的 `RootSnitch-vX.Y.zip`。
2. 打开 Magisk / KernelSU 管理器 → 模块 → 从本地安装。
3. 选择刚下载的 zip，刷入后重启（或直接生效）。
4. 首次运行时授予通知权限。

### 方式二：自行构建

见[从源码构建](#从源码构建)。

## 使用与配置

- **查看告警**：root 进程联网时，通知栏会出现标题为「进程联网告警」的通知，正文形如 `12345 | busybox wget http://example.com`。
- **暂停监控**：在模块目录创建 `disable` 文件（例如 `touch /data/adb/modules/RootSnitch/disable`），监控循环会暂停；删除该文件后自动恢复。
- **卸载**：在 Magisk 管理器中卸载模块并重启。

### 可选：内置过滤规则

源码中的 `MTIO_FILTER` 常量用于忽略 MT 管理器的 `mtio` 进程，避免其代理流量造成误报：

```cpp
static const char *MTIO_FILTER = "/data/user/0/bin.mt.plus/files/mtio ";
```

如需自定义过滤或在编译前修改常量，请编辑 [`main.cpp`](main.cpp) 后重新构建。

## 从源码构建

### 前置条件

- 已安装 Android NDK（示例使用 r26b）。
- 具备 `bash`、`zip`（打包时需要）。

### 步骤

```bash
# 1. 克隆仓库
git clone https://github.com/null07089/RootSnitch.git
cd RootSnitch

# 2. 设置 NDK 路径（按实际安装位置修改）
export NDK=/path/to/android-ndk-r26b

# 3. 编译所有架构
bash scripts/build.sh
# 产物：build/main-arm64、build/main-arm、build/main-x64、build/main-x86

# 4. 打包为可刷入的模块 zip
STAGE=stage/module
rm -rf stage && mkdir -p "$STAGE"
cp -r META-INF "$STAGE/"
cp customize.sh module.prop service.sh "$STAGE/"
cp build/main-* "$STAGE/"
( cd "$STAGE" && zip -r9 ../../RootSnitch-local.zip . )
```

可通过环境变量定制构建：`API`（最低 API，默认 21）、`SRC`（源文件，默认 `main.cpp`）、`OUT`（输出目录，默认 `build`）。

## 项目结构

```
RootSnitch/
├── .github/
│   └── workflows/
│       ├── build.yml          # 推送 / PR 时编译验证并上传产物
│       └── release.yml        # 打 tag 时构建、发布 Release 并更新 update.json
├── META-INF/
│   └── com/google/android/
│       ├── update-binary      # Magisk 安装引导脚本
│       └── updater-script     # 固定内容 "#MAGISK"
├── scripts/
│   └── build.sh               # 多架构编译脚本
├── customize.sh               # 安装时按架构选择二进制、授权通知助手
├── main.cpp                   # 监控程序源码
├── service.sh                 # 开机启动，exec 运行 main
├── module.prop                # 模块元数据（含 updateJson）
├── update.json                # Magisk 在线更新清单
├── CHANGELOG.md               # 更新日志
├── CONTRIBUTING.md            # 贡献指南
├── LICENSE                    # GPL-3.0
└── README.md
```

## 在线更新

模块通过 [`module.prop`](module.prop) 的 `updateJson` 字段接入 Magisk 的在线更新：

```properties
updateJson=https://raw.githubusercontent.com/null07089/RootSnitch/main/update.json
```

[`update.json`](update.json) 的格式：

```json
{
  "version": "1.0",
  "versionCode": 1,
  "zipUrl": "https://github.com/null07089/RootSnitch/releases/download/v1.0/RootSnitch-v1.0.zip",
  "changelog": "https://raw.githubusercontent.com/null07089/RootSnitch/main/CHANGELOG.md"
}
```

**发布流程**：修改 `module.prop` 中的 `version` 与 `versionCode` → 提交并推送 → 打一个与 `version` 匹配的 tag（如 `v1.0`）并推送。GitHub Actions 会自动构建所有架构、打包、发布 Release，并把新的 `update.json` 提交回 `main` 分支。

```bash
git tag v1.0
git push origin v1.0
```

> `versionCode` 必须是递增的整数；`tag` 去掉前缀 `v` 后必须与 `module.prop` 的 `version` 完全一致，否则发布流程会主动失败。

## 常见问题

<details>
<summary>为什么 root 进程联网却没收到通知？</summary>

- 确认系统已授予通知权限，且未拦截模块通知渠道。
- 部分 Android 13+ 设备需要为「系统通知助手」开启助手权限（模块安装时会尝试自动授权）。
- 该进程可能未处于 `su` / `ksu` / `magisk` SELinux 域（例如某些自定义 ROM 使用不同的域名）。
- 若进程尚未持有 socket（只在极短时间内联网），可能错过采样窗口。

</details>

<details>
<summary>会误报吗？</summary>

检测基于「持有 socket 文件描述符」，只要 root 进程打开了网络套接字就会告警，因此像 `curl`、`wget`、`ping`、应用内下载等都会被记录。这是预期行为——模块的目的正是让你知道 root 进程在联网。内置对 MT 管理器 `mtio` 的过滤，可通过修改源码扩展。

</details>

<details>
<summary>耗电/性能如何？</summary>

默认 1 秒轮询一次，且只在 root 进程数量变化时才会执行较重的 `fd` 扫描，日常开销很低。你可以通过修改 `main.cpp` 中的 `LOOP_SLEEP_SEC` 调整轮询间隔。

</details>

## 贡献

欢迎提交 Issue 与 Pull Request，请先阅读 [CONTRIBUTING.md](CONTRIBUTING.md)。

## 免责声明

本项目仅用于**安全审计与个人学习**，帮助你了解设备上 root 进程的网络行为。请勿将其用于任何非法用途。使用本模块产生的任何后果由使用者自行承担。

## 许可证

本项目基于 [GNU General Public License v3.0](LICENSE) 发布。

```
RootSnitch - 根进程联网告警
Copyright (C) 2026 null07089

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
