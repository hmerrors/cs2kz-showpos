# CS2KZ ShowPos 1.0.0-rc1 — Windows x64

独立原生 C++ ShowPos 插件，作者：**hm_error and bell_meow**。

在现有 CS2KZ 服务器独立安装，无需替换 CS2KZ 核心。

- 聊天 `!showpos` / `/showpos`，或控制台 `kz_showpos` 打开点击菜单。
- 支持简洁 2 位 / 详细 6 位小数、位置、视角、速度、体力、蹲伏信息。
- 支持位置和字号调整、观战目标显示、独立 SteamID 设置保存。
- 只依赖 Metamod:Source 2.0 API 18、CS2KZ、MultiAddonManager。

验证组合：Windows x64，CS2 1.41.8.1，Metamod 2.0.0-dev+1469，CS2KZ 0.0.173，MultiAddonManager v1.6-0-ge85a483。维护者已于 2026-09-19 确认功能测试全部通过。Linux 不在本次发布范围。

安装请下载 **CS2KZ-ShowPos-Standalone-Windows.zip**，将其中 addons / cfg 合并至 game/csgo，按 README 加载。GitHub 自动生成的 Source code 压缩包用于编译，不包含可直接安装的 DLL。

已知情况：隔离测试中，原有依赖组合在退出阶段存在可复现异常；不加载 ShowPos 的对照也能复现。详情见仓库 `docs/TESTING.md`。

功能概念来自 zer0.k 的 CS:GO ShowPos；第三方接口和声明保留原有署名，详见 THIRD_PARTY.md。源码按仓库现有 AGPL-3.0 LICENSE 分发。
