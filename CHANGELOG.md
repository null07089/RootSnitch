# 更新日志

本项目遵循 [语义化版本](https://semver.org/lang/zh-CN/) 与 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/) 规范。

## [1.0] - 2026-10-06

### 新增
- 首个公开版本。
- 基于 SELinux 上下文识别 `su` / `ksu` / `magisk` 域的 root 进程。
- 检测进程是否持有 socket 文件描述符，判断其是否发起网络连接。
- 发现 root 进程联网时，通过系统通知（`cmd notification`）发送告警。
- 支持按架构（`arm64` / `arm` / `x86_64` / `x86`）编译与安装。
- 支持 Magisk 在线更新（`updateJson`）。

[1.0]: https://github.com/null07089/RootSnitch/releases/tag/v1.0
