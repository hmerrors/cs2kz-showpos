# Linux x86_64 安装

版本 1.0.0-rc4。独立 Metamod API 18 插件，需要当前 CS2KZ（ICS2KZ002）、MultiAddonManager003 和已挂载的 CS2KZ UI 资源。

1. 下载 `CS2KZ-ShowPos-1.0.0-rc4-Linux-x86_64.zip`。
2. 更新时先在服务器控制台执行 `meta unload showpos`，等待至少一秒，备份旧文件。
3. 把包内 `addons`、`cfg` 合并到服务器 `game/csgo`，保留已有配置和 `addons/showpos/data`。
4. 让运行服务器的用户可以读取插件文件、写入玩家设置目录。

文件位置：

```text
addons/showpos/bin/linuxsteamrt64/showpos.so
addons/showpos/gamedata/showpos.games.txt
addons/metamod/zzz_showpos.vdf
cfg/showpos.cfg
```

VDF 自动加载；如果依赖尚未就绪，等 CS2KZ / MAM 加载完成后在服务器控制台执行：

```text
meta load addons/showpos/bin/linuxsteamrt64/showpos.so
showpos_status
meta list
```

状态应显示 `1.0.0-rc4` / `Linux x86_64`，四项依赖全部 `ready`。玩家输入 `!showpos` / `kz_showpos` 并开启第一行。

## 构建与验收范围

SO 在官方 Steam Runtime 3 Sniper SDK 中构建；固定依赖和镜像见 BUILD.md。对应 Actions artifact 中的 `Linux-binary.txt` 提供 ELF、依赖库和符号版本信息。运行服务器使用游戏自身的 `libtier0.so`，不要安装 SDK 库。

当前版本尚未完成 Linux CS2 实服加载和客户端显示验收，发布为候选版本。实际检查包括菜单、数值更新、观战、传送后采样、设置保存、换图与卸载；这些需要真实服务器 / 客户端，不能以编译成功替代。

## 常见问题

- `API mismatch`：确认 Metamod Plugin API 18。
- `ICS2KZ002` 缺失：更新 CS2KZ 核心；不支持旧接口。
- `assets=missing`：检查 CS2KZ 最新资源与 MultiAddonManager 的挂载。
- `wrong ELF class`：确认 Linux x86_64 包，没有误用 Windows DLL。
- 签名、唯一匹配或 schema 失败：保存加载日志和游戏版本，核对 gamedata，不能绕过检查。
- `GLIBC` / `GLIBCXX` 错误：结合 `Linux-binary.txt` 检查实际环境；完整环境也必须满足 CS2 要求。

回退时卸载 ShowPos，等待至少一秒后恢复备份，或移除 ShowPos VDF。玩家数据可保留，不需要改动 CS2KZ 核心。
