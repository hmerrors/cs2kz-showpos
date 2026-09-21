# Windows x64 / Linux x86_64 构建

## Windows

使用 Visual Studio 2022 C++ Build Tools（实测 MSVC 14.44.35207）、Windows SDK（实测 10.0.26100.0）、Python 3、Git、AMBuild 2.2。在 x64 Native Tools 命令行中构建，建议源码放在 `C:\dev\showpos` 等短路径。

依赖均为构建时依赖，固定提交如下：

| 项目 | 提交 |
|---|---|
| alliedmodders/metamod-source | 7e24ce9e7a03bfeb5c8ab1e4dd55d5d5747f3d33 |
| alliedmodders/hl2sdk，cs2 分支 | d53b5f945536adcdaaa20511dd61afcbc260b2ad |
| alliedmodders/ambuild | 01212cb57c96561f664b6dfb2ee10e66de6f81e4 |
| 公共 CS2KZ 声明基准 v0.0.173 | f9be83a0e5b8364ee30647e8845389d8809c0948 |

克隆对应仓库、checkout 指定提交，然后对 Metamod / HL2SDK 执行 `git submodule update --init --recursive`。在 AMBuild 仓库执行 `python -m pip install .`。不需要编译或替换 CS2KZ 核心。

在 ShowPos 源码根目录：

```powershell
New-Item -ItemType Directory build -Force
Set-Location build
python ../configure.py --mms-path C:/dev/metamod-source --sdk-path C:/dev/hl2sdk --tests
ambuild
```

发布 DLL：`build/showpos/windows-x86_64/showpos.dll`。测试程序在 `showpos-tests`、`showpos-prefs-tests` 对应目录，原生探针在 `showpos-native-probe`。不带 `--tests` 只构建运行插件。

运行测试程序前，把实际游戏的 `game/bin/win64` 加入当前进程 PATH，以便加载 tier0.dll。不要将游戏 DLL 放进分发包。

```powershell
$env:PATH = 'D:/CS2/game/bin/win64;' + $env:PATH
./showpos-tests/windows-x86_64/showpos-tests.exe
./showpos-prefs-tests/windows-x86_64/showpos-prefs-tests.exe
```

探针只能放在隔离的 CS2KZ 测试服，正常加载依赖和 ShowPos 后执行 `meta load <探针 DLL 路径>`。输出必须为 PASS。地图内可用测试命令 `showpos_test_menu` / `showpos_test_click item0` 向 slot 0 发送菜单请求和点击消息；需要该槽位已有玩家/机器人。它验证服务器处理流程，不能验证客户端渲染。

## Linux

使用 x86_64 Ubuntu 22.04、Clang、GNU C++ 标准库、Python 3。SDK manifest 会设置 `_GLIBCXX_USE_CXX11_ABI=0`，不能改成 libc++ 或新 libstdc++ string ABI。依赖提交与上表相同，克隆并初始化其 submodules；不需要编译 CS2KZ。

下面假设依赖位于本项目 `deps/metamod-source`、`deps/hl2sdk`、`deps/ambuild`：

```bash
sudo apt-get update
sudo apt-get install -y clang build-essential python3-venv
python3 -m venv .venv
.venv/bin/pip install ./deps/ambuild
chmod +x deps/hl2sdk/devtools/bin/linux/protoc
mkdir -p build-linux
cd build-linux
CC=clang CXX=clang++ ../.venv/bin/python ../configure.py \
  --mms-path ../deps/metamod-source --sdk-path ../deps/hl2sdk --tests
../.venv/bin/ambuild -j 2
export LD_LIBRARY_PATH="$(realpath ../deps/hl2sdk/lib/linux64)${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
./showpos-tests/linux-x86_64/showpos-tests
./showpos-prefs-tests/linux-x86_64/showpos-prefs-tests
./showpos-module-tests/linux-x86_64/showpos-module-tests
./showpos-loader-tests/linux-x86_64/showpos-loader-tests "$PWD/showpos/linux-x86_64/showpos.so"
cd ..
.venv/bin/python scripts/package_linux.py \
  --binary build-linux/showpos/linux-x86_64/showpos.so
```

输出 `dist/CS2KZ-ShowPos-Standalone-Linux.zip` 和 SHA256 校验文件。构建文件叫 `showpos.so`，打包时安装为 `addons/showpos/bin/linuxsteamrt64/showpos.so`，VDF 使用该完整文件名。

上述独立测试使用 SDK 提供的 `libtier0.so`。运行服务器时使用游戏自己的库，不把 SDK/游戏库打入插件安装包。测试通过不代表已经完成 Linux 游戏内验收。原生探针 `build-linux/showpos-native-probe/linux-x86_64/showpos-native-probe.so` 仅供隔离实服测试。

`.github/workflows/linux.yml` 包含依赖下载、编译、测试、打包和上传 Actions artifact 的完整步骤。推送仓库或手动点击 Run workflow 可触发；它不会自动创建 Release。工作流尚未在远端 GitHub 账号运行，须以实际 Actions 结果为准。
