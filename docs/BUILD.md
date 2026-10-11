# 构建独立插件

构建只生成 ShowPos，不需要编译或替换 CS2KZ。

## 固定依赖

| 项目 | 提交 |
|---|---|
| alliedmodders/metamod-source | eed24a61352b32bf85a4780664f4feccb0b059db |
| alliedmodders/s2sdk，cs2 分支 | 23c73ccf3ae5816d8dc77b4ec553f3a74179eb42 |
| alliedmodders/ambuild | 01212cb57c96561f664b6dfb2ee10e66de6f81e4 |
| CS2KZ 接口、gamedata 与 UI 基准 | df0abea78c2db6908002558d9599476919bb55ce |

CS2 SDK 已从 HL2SDK 迁移至 S2SDK。克隆各仓库后 checkout 对应提交，初始化 Metamod 与 SDK 的 recursive submodules。工作流使用 `deps/metamod-source`、`deps/hl2sdk`（该目录实际来自 S2SDK）、`deps/ambuild`。

## Windows x64

使用 Visual Studio 2022 C++ Build Tools、Windows SDK、Python 3、Git。在 x64 Native Tools 命令行中安装 AMBuild 并构建：

```powershell
python -m pip install ./deps/ambuild
New-Item -ItemType Directory build-windows
Set-Location build-windows
python ../configure.py --mms-path ../deps/metamod-source --sdk-path ../deps/hl2sdk
ambuild -j 4
Set-Location ..
python scripts/package.py --platform windows --version 1.0.0-rc4 --binary build-windows/showpos/windows-x86_64/showpos.dll
```

## Linux x86_64

发布构建使用官方 Steam Runtime 3 Sniper SDK，匹配 CS2 的运行环境。固定镜像：

```text
registry.gitlab.steamos.cloud/steamrt/sniper/sdk@sha256:1c33c507bc75d012e77df5727f93b0d5b8c3f7c8d4142ba5f7a16882cc92e014
```

在 SDK 容器内，以仓库为工作目录：

```bash
(cd deps/ambuild && python3 setup.py install)
chmod +x deps/hl2sdk/devtools/bin/linux/protoc
mkdir build-linux
cd build-linux
CC=clang CXX=clang++ AR=llvm-ar-11 python3 ../configure.py --mms-path ../deps/metamod-source --sdk-path ../deps/hl2sdk
ambuild -j 4
cd ..
python3 scripts/package.py --platform linux --version 1.0.0-rc4 --binary build-linux/showpos/linux-x86_64/showpos.so
```

保留 SDK manifest 设置的 C++ ABI。运行包不包含 SDK / 游戏运行库。

## 自动构建和打包

`.github/workflows/build.yml` 在推送、PR 或手动运行时分别构建 Windows 与 Linux 插件，生成运行 ZIP、每个 ZIP 的 SHA256 以及 Linux 二进制信息。它不构建测试目标，也不运行单元测试。实际 Release 使用成功构建提交的二进制，并附源码与合并校验文件。
