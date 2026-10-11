# CS2KZ ShowPos 1.0.0-rc4 — 最新 CS2KZ UI 适配

独立原生 C++ Metamod 插件，适配 2026-10-11 CS2KZ master `df0abea` 的公共接口和 HUD / 菜单资源。

- 公共接口更新到 `ICS2KZ002`，取消对旧接口的依赖。
- 修正传送挂钩槽位：Windows 165 / Linux 164，传送后丢弃旧采样。
- 适配最新 UI 的 class / dialog 网络更新；避免显示文本被 Panorama 本地化，并显示菜单切换提示。
- 继续复用 CS2KZ 已有资源，在独立布局实体中显示坐标、角度、速度、体力和蹲伏数据。不替换核心 HUD，不需要额外客户端文件。
- 构建使用当前 S2SDK 和含原生挂钩卸载修复的 Metamod；Linux 使用官方 Steam Runtime 3 SDK。
- 保留点击菜单、2/6 位小数、位置 / 字号、观战和独立玩家设置保存。

下载对应平台的运行 ZIP，把 `addons`、`cfg` 合并到服务器 `game/csgo`。更新前卸载 ShowPos，保留已有配置和 `addons/showpos/data`。确保 CS2KZ 的最新 UI 已通过 MultiAddonManager 正常挂载。

玩家输入 `!showpos` 或 `kz_showpos`，点击第一行开启；默认关闭。服务器使用 `showpos_status` 检查版本和依赖状态。

本版包含 Windows / Linux 编译和打包验证，没有新增或运行单元测试。当前二进制仍需真实游戏内验收，按候选版本发布。历史验收不作为本版实服验证。

作者：hm_error 狸喵w。功能概念来自 zer0.k；源码 AGPL-3.0，来源见 THIRD_PARTY.md。
