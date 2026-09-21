# Linux x86_64 安装与验收

版本：1.0.0-rc2。作者：hm_error and bell_meow。

该版本使用原生 C++ / Metamod API 18，独立安装到现有 CS2KZ 服务器。游戏内插件依赖仍只有 Metamod 2.0（API 18）、CS2KZ 0.0.173（ICS2KZ001）、MultiAddonManager（MultiAddonManager003），复用已挂载的 CS2KZ HUD 资源。系统目标为 x86_64 glibc Linux；不面向 ARM64 或 musl/Alpine。

## 安装

1. 确认服务器 `meta version` 显示 API 18，`meta list` 中 CS2KZ 和 MultiAddonManager 正常加载。
2. 解压 **CS2KZ-ShowPos-Standalone-Linux.zip** 到临时目录。
3. 首次安装将包内 `addons`、`cfg` 合并到服务器 `game/csgo`。更新时先执行 `meta unload showpos`，等待至少一秒；备份旧 SO、gamedata、VDF 和配置，再复制新文件，保留玩家 `data` 和已有语言设置。
4. 使用运行 CS2 服务器的用户读取这些文件，并允许其写入 `addons/showpos/data`。

安装后应有：

```text
game/csgo/addons/showpos/bin/linuxsteamrt64/showpos.so
game/csgo/addons/showpos/gamedata/showpos.games.txt
game/csgo/addons/metamod/zzz_showpos.vdf
game/csgo/cfg/showpos.cfg
```

在**服务器控制台**（不是 Linux shell）执行：

```text
meta load addons/showpos/bin/linuxsteamrt64/showpos.so
showpos_status
meta list
```

VDF 已自动加载时无需再次 `meta load`。`meta list` 作者应显示 `hm_error and bell_meow`；`showpos_status` 应显示 `Linux x86_64`，CS2KZ、MAM、schema、assets 四项都为 `ready`。如果启动时依赖加载顺序不满足，等核心和 MAM 加载完成后手动加载 ShowPos。

`cfg/showpos.cfg` 是插件读取的 KeyValues 文件，不能 `exec`。客户端聊天 `!showpos` / `/showpos` 或客户端控制台 `kz_showpos` 打开菜单；默认显示关闭，点击第一行开启。

## 验证记录与边界

2026-09-20/21，在本机隔离的 Ubuntu 22.04 x86_64 虚拟机内完成原生构建，编译器为 Clang 14，使用固定版本的 Metamod / HL2SDK。测试使用 SDK 提供的 libtier0.so。

| 检查 | 结果 |
|---|---|
| Linux 插件及原生探针编译 | 通过 |
| 数值格式化、288 种显示组合和缓冲区边界 | 590,985 项通过 |
| 偏好文件解析、覆盖保存、提交失败保留状态与重试 | 1,815 项通过 |
| ELF 地址映射、唯一匹配、截断/越界拒绝、实际加载模块识别 | 142 项通过 |
| ELF 动态加载、CreateInterface、API / 版本 / 署名、dlclose | 通过；未调用游戏内 Load |
| Windows 构建及同一套格式化/偏好回归 | 通过；现有服务器文件未替换 |

作者读回为 `hm_error and bell_meow`，插件版本 `1.0.0-rc2`，API 18。二进制依赖与符号版本记录随交付保存在 `verification/linux-binary.txt`。GitHub Actions 工作流已提供，尚未在远端仓库实际运行。

本包的直接符号要求为 **GLIBC 2.34、GLIBCXX 3.4.26、CXXABI 1.3.5**；Ubuntu 22.04 是实际构建/测试基线。完整运行环境还需满足 CS2 自身要求。二进制未写入构建机的 RPATH/RUNPATH，使用游戏提供的 libtier0.so 和系统 C/C++ 运行库；SDK 环境下 `ldd -r` 无缺失符号。

尚未验证：Linux 实服的 Metamod 加载、与 CS2KZ 的真实 Hook 顺序、游戏内 HUD/菜单/观战、换图及卸载。没有 Linux 游戏服务器可用，当前是待实服验收的候选版本。Windows rc1 的功能确认不覆盖这些 Linux 项目。

## 实服验收

- 加载：检查上述四项 ready 与无新增错误；玩家移动时采样计数增长。
- 菜单与 HUD：开启简洁/详细模式，切换每项数值、坐标和字号；关闭菜单后鼠标恢复。
- 保存：修改设置后重连，确认同一 SteamID64 设置恢复；确认 data 可写。
- 观战：使用观看者自己的设置显示被观察玩家的数据；无有效目标时隐藏。
- 生命周期：检查换图、暂停/恢复，以及卸载后延时重载；卸载后无残留 ShowPos HUD。
- 精确采样顺序：在隔离测试服加载构建生成的原生探针，要求 `Native API 18 pre-order/unload self-test: PASS`；探针不包含在运行包。

## 常见加载问题

`API mismatch`：检查 Metamod API，不能只看 2.0 版本名。`wrong ELF class` / `Exec format error`：检查是否为 Linux x86_64，以及是否误拿 DLL。依赖库错误可在 Linux shell 执行（替换实际路径）：

```bash
cd /path/to/Counter-Strike\ Global\ Offensive/game
LD_LIBRARY_PATH="$PWD/bin/linuxsteamrt64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
  ldd csgo/addons/showpos/bin/linuxsteamrt64/showpos.so
```

游戏自带的 `libtier0.so` 从 `game/bin/linuxsteamrt64` 加载，不要从互联网另找同名文件放进插件包。`GLIBC` / `GLIBCXX` 版本不足时，使用符合下方二进制记录要求的系统运行环境，或参照 BUILD.md 在目标环境重编译。

`Signature not found`、`not unique` 或 schema 错误表示游戏版本与 gamedata 不匹配。保留加载日志和 `version`、`meta version`、`meta list` 输出，重新核对函数签名/偏移；不要绕过校验强行加载。

## 回退

执行 `meta unload showpos`，等待至少一秒后恢复备份的本插件文件，或移除 `zzz_showpos.vdf` 停止自动加载。保留 data 可保留玩家偏好。整个过程不需要替换 CS2KZ 核心。
