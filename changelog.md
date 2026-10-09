# 更新日志

本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/) 与 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/) 规范。

## [1.2] - 2026-10-06

### 新增
- 新增 `action.sh`：在 KernelSU / APatch 中点击模块“操作”按钮时，解除隐藏进程的挂载，并列出持有 socket 的 root 进程。
- 已将其清理为纯 POSIX `sh`（原脚本依赖 bash 数组与 `[[ =~ ]]`，且内嵌了 MT 管理器终端的环境封装，无法直接运行）。

## [1.1] - 2026-10-06

### 修复
- `service.sh` 改为按设备 ABI 选择 `main-<abi>`，并对模块目录与可执行文件缺失做回退，不再依赖固定的 `main` 文件名。
- `customize.sh` 保留架构专用二进制，同时建立 `main` 软链接作为兜底。

### 文档
- 精简 README，改为更直白的说明。
- 补充「使用 Magisk 模块开关暂停监控」的说明。

## [1.0] - 2026-10-06

### 新增
- 首个公开版本。
- 基于 SELinux 上下文识别 `su` / `ksu` / `magisk` 域的 root 进程。
- 检测进程是否持有 socket 文件描述符，判断其是否发起网络连接。
- 发现 root 进程联网时，通过系统通知（`cmd notification`）发送告警。
- 支持按架构（`arm64` / `arm` / `x86_64` / `x86`）编译与安装。
- 支持 Magisk 在线更新（`updateJson`）。

[1.2]: https://github.com/null07089/RootSnitch/releases/tag/v1.2
[1.1]: https://github.com/null07089/RootSnitch/releases/tag/v1.1
[1.0]: https://github.com/null07089/RootSnitch/releases/tag/v1.0
