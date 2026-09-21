# 更新记录

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
