# 来源与许可

- ShowPos 的功能概念来自 zer0.k 的 [CS:GO ShowPos](https://github.com/zer0k-z/showpos)。本项目没有复制其 SourcePawn 实现，是面向 Source 2 的独立 C++ 实现。
- [CS2KZ](https://github.com/KZGlobalTeam/cs2kz-metamod) v0.0.173（AGPL-3.0）：`include/ics2kz.h` 公共接口原文；`include/movedata.h` 精简适配的移动数据声明；`gamedata/showpos.games.txt` 必需签名和偏移；schema / HUD 约定的审计依据。根目录 LICENSE 为 AGPL-3.0。
- [MultiAddonManager](https://github.com/Source2ZE/MultiAddonManager)：`include/imultiaddonmanager.h` 公共接口原文，保留原 GPL-3.0 许可声明。只查询该接口确认依赖；资源分发由现有 CS2KZ/MAM 完成。
- [Metamod:Source](https://github.com/alliedmodders/metamod-source) 及内置 KHook：构建依赖，遵循其原有许可。运行包不分发 Metamod。
- [HL2SDK-CS2](https://github.com/alliedmodders/hl2sdk/tree/cs2)：构建依赖，遵循 Valve / AlliedModders 的原有许可。引擎接口、tier1、protobuf 等 SDK 代码参与构建；源码包通过固定版本说明复现，不包含游戏程序或游戏资源。

没有包含或发布 CS2KZ 核心 DLL、第三方模式/样式 DLL、Steam 运行库或 Workshop VPK。测试 DLL 不随运行包分发。
