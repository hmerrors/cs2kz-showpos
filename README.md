# CS2KZ ShowPos — 独立 Metamod 插件

版本 **1.0.0-rc4**，作者：**hm_error 狸喵w**。本项目由 ChatGPT 协助完成。

原生 C++ / Metamod API 18，支持 Windows x64 与 Linux x86_64。适配截至 2026-10-11 的 CS2KZ master（`df0abea`）、`ICS2KZ002` 公共接口及最新 HUD / 菜单布局。ShowPos 自己创建显示实体，复用 CS2KZ 已有 Workshop 资源，不替换核心计时器，不需要额外的客户端文件。

## 运行条件

- 当前 Windows x64 / Linux x86_64 CS2 服务器。
- Metamod:Source **2.0.0-dev+1474 或更新开发构建**，Plugin API **18**，包含 `eed24a6` 原生挂钩卸载修复。插件会检查版本，旧构建会拒绝加载。
- 当前 CS2KZ，导出 **ICS2KZ002**；使用与核心匹配的最新 UI / Workshop 资源。
- MultiAddonManager，导出 **MultiAddonManager003**，正常挂载 CS2KZ 原有资源。

Metamod 对应下载见 [官方开发构建](https://www.sourcemm.net/downloads.php?branch=master)。

Linux 请使用官方 Metamod 文件，保留其符号表，不要额外 strip；缺少所需符号时 ShowPos 会拒绝加载。

旧 `ICS2KZ001` 核心不受支持。插件会检查接口、schema 和签名，加载失败时保留错误信息。

## 安装与更新

从 [Releases](https://github.com/hmerrors/cs2kz-showpos/releases) 下载对应平台的运行包：

- Windows：`CS2KZ-ShowPos-1.0.0-rc4-Windows-x64.zip`
- Linux：`CS2KZ-ShowPos-1.0.0-rc4-Linux-x86_64.zip`

更新前在服务器控制台执行 `meta unload showpos`，等待至少一秒，备份旧文件。将包内 `addons`、`cfg` 合并到服务器 `game/csgo`；保留已有 `cfg/showpos.cfg` 和 `addons/showpos/data`。首次安装可在 Windows 使用附带的 `Install-ShowPos.ps1 -Csgo "你的 game\csgo 路径"`。

VDF 会自动加载。如果启动时 CS2KZ / MAM 尚未就绪，等它们加载后在服务器控制台手动执行对应命令：

```text
// Windows
meta load addons/showpos/bin/win64/showpos.dll
// Linux
meta load addons/showpos/bin/linuxsteamrt64/showpos.so
showpos_status
meta list
```

已自动加载时不要再次加载。`showpos_status` 应显示版本 `1.0.0-rc4`，CS2KZ、MAM、schema、assets 都为 `ready`。不要混用两平台的 VDF，也不要保留第二份 ShowPos VDF。

## 玩家操作

聊天输入 `!showpos` / `/showpos`，或客户端控制台输入 `kz_showpos`。默认关闭显示，点击菜单第一行开启。

| 设置 | 可选值 |
|---|---|
| ShowPos | 关闭 / 简洁（2 位）/ 详细（6 位） |
| 位置 | 开 / 关；XYZ |
| 视角 | 关 / Pitch、Yaw / Pitch、Yaw、Roll |
| 速度 | 关 / 水平速率 / 三维速率 / XYZ 向量 |
| 体力 | 开 / 关；Stamina |
| 蹲伏 | 开 / 关；DuckAmount、DuckSpeed |
| 位置与字号 | X 0..70%、Y -40..40%、12..32px |

默认在左侧，Y 偏移 -24%、18px。可输入 `kz_showpos x 4`、`kz_showpos y -24`、`kz_showpos size 18` 调整。关闭按钮或 `kz_showpos close` 释放鼠标；菜单 60 秒无操作自动关闭。先关闭其他 CS2KZ 菜单，可避免独立菜单重叠。

`cfg/showpos.cfg` 的 `language` 支持 `zh` / `en`，重载生效。这是插件读取的 KeyValues 文件，不要 `exec showpos.cfg`。认证玩家设置保存到 `addons/showpos/data/<SteamID64>.txt`。观战使用观看者自己的设置，无有效目标时隐藏。最新 CS2KZ UI 会在计分板 / 结算画面隐藏 HUD。

## 卸载与验证范围

服务器控制台执行 `meta unload showpos`；等待至少一秒后才能重载。如果卸载过 CS2KZ 核心，先恢复核心，再重载 ShowPos，以恢复采样挂钩顺序。

本次发布构建 Windows DLL 与 Steam Runtime 3 Linux SO，并检查打包内容。没有新增、构建或执行单元测试。当前版本尚未进行真实玩家连接后的游戏内验收；历史版本的功能验收不能替代本次验收，因此作为候选版本发布。

[构建说明](docs/BUILD.md) · [架构说明](docs/ARCHITECTURE.md) · [Linux 安装](docs/LINUX.md) · [发布说明](docs/RELEASE_NOTES.md)

功能概念来自 [zer0.k](https://github.com/zer0k-z/showpos)。第三方声明与许可见 [THIRD_PARTY.md](THIRD_PARTY.md)。源码按 AGPL-3.0 分发。
