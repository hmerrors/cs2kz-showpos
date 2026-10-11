# 更新记录

## 1.0.0-rc4 — 2026-10-11

- 更新至 CS2KZ ICS2KZ002 与最新 HUD / 菜单资源约定；保持独立 Metamod 插件。
- 修正 Windows / Linux 的 Teleport 挂钩槽位至 165 / 164。
- 对已有 UI 条目发送精确网络通知，防止文本被自动本地化，显示菜单点击提示。
- 使用 Metamod 1474 或更新开发构建和迁移后的 S2SDK；Linux 发布使用 Steam Runtime 3 SDK。
- 卸载时禁用回调并由 Metamod 延迟移除挂钩；后续调用使用公共 KHook 函数，避免已销毁的插件接口。
- 使用 SDK 声明读取实体系统和网络接收槽位，并修正新 SDK 下的命令注销顺序。
- 新增 Windows / Linux 插件编译和运行包工作流，无新增或执行单元测试；当前版本仍需实服验收。

## 1.0.0-rc2 — 2026-09-20

- 新增 Linux x86_64 构建、专用安装包和安装说明，保留 Windows 构建支持。
- Linux 使用 ELF 可执行段扫描与实际模块校验、Linux gamedata、平台 Hook 参数和原子偏好文件保存。
- 新增 ELF 边界/模块测试，修复首次并行构建时测试程序抢先引用生成头文件的问题。
- 提供 GitHub Actions Linux 构建和打包工作流，作者仍为 `hm_error and bell_meow`。
- Linux 游戏内验收尚未完成；详细验证边界见 docs/LINUX.md。

## 1.0.0-rc1 — 2026-09-19

- 独立 C++ ShowPos，Metamod API 18，适配已验证的 Windows CS2KZ 0.0.173 / MultiAddonManager 组合。
- 支持点击切换的中英文菜单、2/6 位小数、位置/视角/速度/体力/蹲伏、观战显示和独立玩家设置保存。
- `meta list` 的作者署名为 `hm_error and bell_meow`。
- 维护者确认功能测试已全部通过；保留隔离测试结果及既有依赖组合退出异常记录。
- 运行包与源码包独立提供；不替换 CS2KZ 核心。
