# CS2KZ ShowPos — 独立 Windows 插件

linux尚未支持

1.0.0-rc1，原生 C++，Metamod Plugin API **18**。独立安装，不替换 CS2KZ DLL、模式、样式或核心资源。

作者：**hm_error and 狸喵w**。

## 运行条件

- Windows x64 CS2 服务器；本次验证游戏版本 1.41.8.1 / ServerVersion 2000908。
- Metamod:Source 2.0 **API 18**；验证版本 2.0.0-dev+1469。早期 API 17 构建不兼容。
- CS2KZ **0.0.173**，导出 `ICS2KZ001`。
- MultiAddonManager **v1.6-0-ge85a483**，导出 `MultiAddonManager003`；CS2KZ 原有 HUD 资源须正常挂载。

无新增 SQL 数据库依赖。编译用 HL2SDK，不需要将 SDK 安装到服务器。Linux 尚未移植/构建。

## 安装与启动

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

更新 DLL 前先卸载。永久移除时删掉上面的 VDF 和 `addons/showpos`，以及不再需要的 `cfg/showpos.cfg`；保留 `data` 可保留玩家设置。无需改动 CS2KZ。

如果卸载过 **CS2KZ 核心**，必须先恢复核心，再重载 ShowPos。ShowPos 会暂停采样，防止挂钩顺序改变后显示错误数据。

## 验证范围

2026-09-19，维护者确认功能测试已全部通过。此前编译、格式化、偏好文件测试、实际 API 18 挂钩顺序、地图机器人采样、菜单点击消息及服务器 HUD 更新也已验证。版本号保留 1.0.0-rc1；测试记录与已知依赖组合问题见 [验收记录](docs/TESTING.md)。Linux 尚未验证。

源码构建见 [BUILD.md](docs/BUILD.md)，实现说明见 [ARCHITECTURE.md](docs/ARCHITECTURE.md)，GitHub 上传与发布步骤见 [GITHUB_UPLOAD.md](docs/GITHUB_UPLOAD.md)。

原 ShowPos 思路来自 [zer0.k](https://github.com/zer0k-z/showpos)。当前实现为原生 C++ 重写；公共接口及部分声明/签名来源见 `THIRD_PARTY.md`。源码以 AGPL-3.0 分发。
