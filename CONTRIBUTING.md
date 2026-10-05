# 贡献指南

感谢你对 RootSnitch 的关注！在提交 Issue 或 Pull Request 之前，请先阅读以下约定。

## 报告问题

- 使用 [Issue 模板](.github/ISSUE_TEMPLATE/bug_report.yml) 提交 Bug。
- 请附上设备型号、Android 版本、Magisk / KernelSU 版本、模块版本。
- 如果可能，请提供 `logcat` 或 Magisk 安装日志，以及复现步骤。

## 提交代码

1. Fork 本仓库并基于 `main` 创建功能分支：`git checkout -b feat/your-feature`。
2. 保持改动聚焦，一个 PR 只解决一件事。
3. 提交前请确保本地可以成功编译：

   ```bash
   export NDK=/path/to/android-ndk-r26b
   bash scripts/build.sh
   ```

4. 遵循现有代码风格（C++ 以同文件风格为准，不要引入无关的格式化改动）。
5. 提交信息请使用清晰的祈使句，例如 `fix: 修复 socket 检测遗漏`。
6. 在 PR 描述中说明动机、实现方式与测试情况。

## 版本发布（维护者）

1. 更新 `module.prop` 的 `version` 与 `versionCode`。
2. 在 `CHANGELOG.md` 记录本次变更。
3. 提交并推送 `main`。
4. 打与 `version` 一致的 tag 并推送：

   ```bash
   git tag v1.0
   git push origin v1.0
   ```

GitHub Actions 会自动完成构建、发布与 `update.json` 更新。

## 许可证

向本项目贡献代码即表示你同意你的贡献基于 [GPL-3.0](LICENSE) 授权。
