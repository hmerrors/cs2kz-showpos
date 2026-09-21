# CS2KZ ShowPos — 独立 C++ 插件

**本项目由ChatGPT-6 Astra协助完成**

1.0.0-rc2，原生 C++，Metamod Plugin API **18**。支持 Windows x64 / Linux x86_64 构建，独立安装，不替换 CS2KZ 核心、模式、样式或核心资源。

作者：**hm_error 狸喵w**。

## 运行条件

- Windows x64 或 Linux x86_64 CS2 服务器。Windows 游戏内验证基于 1.41.8.1 / ServerVersion 2000908；Linux 验证范围见下文。
- Metamod:Source 2.0 **API 18**；验证版本 2.0.0-dev+1469。
- CS2KZ **0.0.173**，导出 `ICS2KZ001`。
- MultiAddonManager **v1.6-0-ge85a483**，导出 `MultiAddonManager003`；CS2KZ 原有 HUD 资源须正常挂载。

## 安装与启动

**Linux 服主：**下载 `CS2KZ-ShowPos-Standalone-Linux.zip`，把其中 `addons`、`cfg` 合并到 `game/csgo`。已有配置先备份，保留 `addons/showpos/data`。然后在服务器控制台执行：

```text
meta load addons/showpos/bin/linuxsteamrt64/showpos.so
showpos_status
meta list
```

Linux 包中的 VDF 指向 `.so`，不要使用 Windows 包的 VDF。完整安装、检查与回退步骤见 [Linux 安装说明](docs/LINUX.md)。

**Windows：**

把 Windows 包内的 `addons`、`cfg` 合并到服务器的 `game/csgo`，或运行附带的 `Install-ShowPos.ps1 -Csgo "你的 game\csgo 路径"`。包中只有以下运行文件：

```text
addons/showpos/bin/win64/showpos.dll
addons/showpos/gamedata/showpos.games.txt
addons/metamod/zzz_showpos.vdf
cfg/showpos.cfg
```

先确认 `meta list` 中 CS2KZ 和 MultiAddonManager 已加载，然后在**服务器控制台**执行：

```text
meta load addons/showpos/bin/win64/showpos.dll
showpos_status
meta list
```

以后启动由 VDF 自动加载。若因文件枚举顺序提示依赖未加载，等 CS2KZ / MAM 就绪后手动执行上述 `meta load`。同一插件不要配置两份 VDF。

`showpos_status` 的 CS2KZ、MAM、schema、assets 应全部为 `ready`。有玩家移动时采样计数增加；玩家开启显示后，HUD 更新计数增加。

## 玩家操作

聊天输入 `!showpos` 或 `/showpos`，或客户端控制台输入 `kz_showpos`，打开点击切换菜单。默认关闭显示，点击第一行开启。

| 设置 | 可选值 |
|---|---|
| ShowPos | 关闭 / 简洁（2 位）/ 详细（6 位） |
| 位置 | 开 / 关；XYZ |
| 视角 | 关 / Pitch、Yaw / Pitch、Yaw、Roll |
| 速度 | 关 / 水平速率 / 三维速率 / XYZ 向量 |
| 体力 | 开 / 关；Stamina |
| 蹲伏 | 开 / 关；DuckAmount、DuckSpeed |
| 位置与字号 | X 0..70%、Y -40..40%、12..32px |

默认左侧、垂直偏移 -24%、18px，可移动以避开现有 HUD。按菜单关闭按钮，或输入 `kz_showpos close`；菜单 60 秒无操作自动释放鼠标。高级定位：`kz_showpos x 4`、`kz_showpos y -24`、`kz_showpos size 18`。

`cfg/showpos.cfg` 中 `language` 支持 `zh` / `en`，重载生效。这是插件读取的 KeyValues 文件，不要 `exec showpos.cfg`。

认证玩家设置独立保存到 `addons/showpos/data/<SteamID64>.txt`。机器人不写入永久设置；认证前的设置暂存于会话。观战使用观看者的显示设置，读取被观察玩家的采样；无有效目标时隐藏。

## 卸载与更新

服务器控制台运行 `meta unload showpos`。等待至少一秒后再加载；紧接在同一批命令中重载会明确拒绝，避免旧 DLL 尚在异步清理时重入。

更新 DLL / SO 前先卸载。永久移除时删掉上面的 VDF 和 `addons/showpos`，以及不再需要的 `cfg/showpos.cfg`；保留 `data` 可保留玩家设置。无需改动 CS2KZ。

如果卸载过 **CS2KZ 核心**，必须先恢复核心，再重载 ShowPos。ShowPos 会暂停采样，防止挂钩顺序改变后显示错误数据。

## 验证范围

2026-09-19，维护者确认 Windows rc1 功能测试已全部通过，包括游戏内 HUD 和菜单。rc2 增加 Linux 适配；编译和测试情况见 [Linux 验证记录](docs/LINUX.md)。目前没有 Linux CS2KZ 实服验收，不能把 Windows 的功能确认当作 Linux 游戏内验收。Windows 历史测试与已知依赖组合问题见 [验收记录](docs/TESTING.md)。

源码构建见 [BUILD.md](docs/BUILD.md)，实现说明见 [ARCHITECTURE.md](docs/ARCHITECTURE.md)，GitHub 上传与发布步骤见 [GITHUB_UPLOAD.md](docs/GITHUB_UPLOAD.md)。

原 ShowPos 思路来自 [zer0.k](https://github.com/zer0k-z/showpos)。当前实现为原生 C++ 重写；公共接口及部分声明/签名来源见 `THIRD_PARTY.md`。源码以 AGPL-3.0 分发。
