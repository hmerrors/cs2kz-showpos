# 独立实现与审计边界

CS2KZ v0.0.173 公共接口提供玩家、SteamID 和观战信息，但不提供 pre-movement 回调、菜单/偏好注册或完整 DuckSpeed。实现不转换 KZPlayer 内部对象，不链接 CS2KZ 私有符号，也不修改核心；缺少的部分由本 DLL 通过 Metamod API 18、HL2SDK 接口和 Source 2 schema 完成。

## 采样

`MovementHook` 用 Metamod 自带 KHook 注册真正的 pre-only 回调，原生 post 指针为 null。在 CS2KZ 的 mixed hook 之后注册，经实际 Metamod 探针验证，观察先于核心 pre 回调与原函数。采样 CMoveData 的 origin、viewAngles、velocity，并在同一次调用中读取 movement services 的 stamina、duckAmount、duckSpeed。回调返回 Ignore，不写移动数据。

普通 `KHook::Member` 即使构造时传入 null post，也会注册 post 包装 thunk，不能据此保证所需先后顺序。不要用普通 wrapper 替换 `MovementHook`。核心卸载后锁定采样，重载 ShowPos 才恢复顺序；不声称支持任意第三方运动 detour 的动态插入顺序。

签名扫描精确对应加载路径的 server.dll 磁盘 PE 可执行节，要求唯一命中，再转换成模块 RVA。移动入口前 11 字节通过 KHook 原始 trampoline 校验，避免扫描已被核心改写的入口或误取 Metamod 同名 server.dll。签名和非 schema 偏移来自指定 CS2KZ 版本的 gamedata；游戏更新后需要重新核对，不能保证任意后续版本兼容。

## 菜单与 HUD

每个观看者最多两个独立 `custom_hud_layout`，实体名 `showpos_menu_<slot>` / `showpos_hud_<slot>`。复用 CS2KZ 已分发的通用 `menu` 和 `mhud` 资源，各自拥有布局状态；不采用/修改核心拥有的实体，也不覆盖 VPK。文本显示在本插件自己的 `mhud_timer` 标签内，因此不会替换核心计时器的文本。

通过 schema collection manipulators 操作 interned panel/class/dialog string 列表。检查玩家 slot 与消息中的完整 packed entity handle，仅处理自己菜单的 item0..8 / m_close。网络 CheckTransmit post 移除其他观看者的 ShowPos 实体。默认布局左侧，但自定义位置仍可能和用户已有 HUD 重叠。

约每 0.05 游戏秒刷新，64 tick 下通常为 16 次/秒。只在文本/样式变化时写布局状态。采样全体玩家以服务观战，显示使用观看者自己的偏好。有效性同时检查 pawn serial、存活、队伍、观战模式/目标和 0.25 秒时效；传送后清掉旧采样。格式化采用固定 1024 字节缓冲。

## 生命周期与保存

大部分挂钩注册在原生引擎函数并同步移除。控制台 dispatch 可能正包围 `meta unload showpos`，由 Metamod 跟踪并延迟移除；禁用目标后等待其调用完成才释放 DLL。不 detour 插件自己的可卸载函数。

ConCommand 按 Load/Unload 显式分配和注销，避开 ICvar 销毁后的静态析构。暂停/卸载/换图/断线清除所属 HUD 并释放输入。核心卸载要求重载本插件。

认证后的 SteamID64 为独立文件名，单文件上限 4096 字节，数值用 from_chars 完整校验和 int64 范围限制。写临时文件后 MoveFileEx 替换，延迟合并菜单操作；保存失败保留 dirty 状态重试。无需改动 CS2KZ preferences 或 SQLMM。

## 文件对应

| 文件 | 职责 |
|---|---|
| src/plugin.cpp | Metamod 生命周期、命令、采样、玩家/观战状态 |
| src/native_hook.h、hook.cpp | 原生 API 18 挂钩与可见性过滤 |
| src/engine.cpp | SDK 接口、schema、PE 签名扫描 |
| src/hud.cpp | 独立布局实体、菜单、文本差量更新 |
| src/prefs.cpp | 独立 SteamID 文件持久化 |
| src/format.cpp | 只读固定缓冲格式化 |
| tests | 格式化、偏好及真实 Metamod 原生探针 |

探针 DLL 仅供隔离测试，运行包不含它。探针临时挂钩自身测试函数，因此固定驻留进程，保证 KHook 留存的 detour 地址在退出前有效；生产 DLL 不执行该测试行为。
