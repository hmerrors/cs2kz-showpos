# Windows x64 构建

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
