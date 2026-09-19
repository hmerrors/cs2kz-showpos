# 将 CS2KZ ShowPos 上传到 GitHub

当前插件版本：1.0.0-rc1。`meta list` 作者显示：`hm_error and bell_meow`。

以下步骤由你在自己的 GitHub 账号中执行。本地准备好的文件不会自动创建或发布远程仓库。

## 1. 准备要上传的源码

优先使用交付目录里的 `GitHub/cs2kz-showpos` 文件夹。也可以解压 `CS2KZ-ShowPos-Standalone-source.zip`，进入其中的 `showpos` 文件夹。两者包含同一份可发布源码。

进入后应直接看到这些文件和目录：

```text
README.md
LICENSE
THIRD_PARTY.md
CHANGELOG.md
.gitignore
.clang-format
AMBuildScript
AMBuilder
configure.py
src/
include/
gamedata/
cfg/
addons/metamod/zzz_showpos.vdf
scripts/
tests/
docs/
```

这些内容将放在 GitHub 仓库根目录。网页上传时选择文件夹内的所有内容，让 README.md 直接出现在仓库首页。

源码仓库保存源码和文档；可安装的 Windows ZIP 放在 Releases。不要把实际 `game/csgo`、整个工作目录、旧版内置 ShowPos 包、玩家 data、服务器配置、数据库、密钥或日志一起上传。本交付的 GitHub 目录已经按此整理。

## 2. 创建 GitHub 仓库

1. 登录 GitHub，打开 [创建仓库](https://github.com/new)。
2. **Owner** 选择自己的账号或有权限的组织。
3. **Repository name** 建议填 `cs2kz-showpos`。
4. **Description** 可填：`Standalone native C++ ShowPos for CS2KZ, using Metamod API 18. By hm_error and bell_meow.`
5. 希望公开下载和查看源码时选择 **Public**。
6. 不额外生成 README、.gitignore 或 License；这些文件已在源码中准备好。
7. 点击 **Create repository**。

此项目继续使用已有的 AGPL-3.0 LICENSE，并保留 THIRD_PARTY.md 和第三方头文件中的原署名。本次改动是本插件的作者显示，不更换已有许可证。

界面操作依据：[GitHub 创建仓库说明](https://docs.github.com/en/repositories/creating-and-managing-repositories/creating-a-new-repository)。

## 3. 使用网页上传源码

1. 空仓库页面点击 **uploading an existing file**；已有文件的仓库使用 **Add file → Upload files**。
2. 打开本地 `GitHub/cs2kz-showpos`，把里面的文件和子目录拖到上传区域。确认包含 `.gitignore` 和 `.clang-format`。
3. 检查上传列表，README.md 应处于根目录，`src/plugin.h` 应保持这个路径。
4. 提交说明可写 `Initial standalone ShowPos release`。
5. 新建的个人仓库可以提交到默认分支；完成页面的 **Commit changes** 提交操作。如果页面采用分支/PR 流程，则提交后创建并合并该 PR。
6. 回到仓库首页，确认 README 正常显示，`docs/BUILD.md`、`docs/TESTING.md` 等链接能打开。

GitHub 网页一次最多上传 100 个文件，单个文件最多 25 MiB。准备好的源码规模适合网页上传。如果文件未全部上传，补传遗漏项；不要把源码 ZIP 当作唯一的仓库内容。[GitHub 文件上传说明](https://docs.github.com/en/repositories/working-with-files/managing-files/adding-a-file-to-a-repository)

## 4. 发布供服主下载的安装包

1. 在仓库首页进入 **Releases → Draft a new release**；尚无版本时可能显示 **Create a new release**。
2. 新建标签 **v1.0.0-rc1**，Target 选择刚上传源码所在的 `main` 分支。
3. 标题填写 **CS2KZ ShowPos 1.0.0-rc1 — Windows x64**。
4. 将 `docs/RELEASE_NOTES.md` 的内容复制到版本说明。
5. 上传以下附件：
   - `CS2KZ-ShowPos-Standalone-Windows.zip`：服主使用的安装包。
   - `CS2KZ-ShowPos-Standalone-source.zip`：与 DLL 对应的源码快照。
   - `SHA256SUMS.txt`：交付文件校验值。
6. 当前 DLL 版本仍是 `1.0.0-rc1`，勾选 **Set as a pre-release**，再点击 **Publish release**。这是沿用现有版本号，不否定你已完成的功能验收。
7. 发布完成后，用页面附件下载一次 Windows ZIP，确认其中有 `addons/showpos/bin/win64/showpos.dll`。

GitHub 还会自动提供 `Source code (zip)` / `Source code (tar.gz)`；这些是标签对应的源码，不含已编译安装 DLL。让服主下载明确命名的 Windows 安装包。

`verification-logs.zip` 是本地验证记录，通常无需公开上传；`SHA256SUMS.txt` 中有它的条目也不要求上传该附件。若发布正式 1.0.0，应先同步修改代码中的版本号和文档、重新编译并打包，再使用 v1.0.0 标签，不要仅把 rc1 的安装包改名。

界面操作依据：[GitHub 管理 Release 说明](https://docs.github.com/en/repositories/releasing-projects-on-github/managing-releases-in-a-repository)。

## 5. 也可以用 Git 命令上传

如果选择这一种方式，不必再进行第 3 步网页上传。先按第 2 步创建空仓库，再在本地**准备好的源码文件夹**打开 PowerShell。将下方账号、路径、姓名和邮箱占位内容替换成你自己的信息。

```powershell
Set-Location '你的源码文件夹路径'
git init -b main
git config user.name '你的 Git 提交显示名'
git config user.email 'GitHub 已验证邮箱或 GitHub 提供的 noreply 邮箱'
git add .
git status --short
git commit -m "Initial standalone ShowPos release"
git remote add origin https://github.com/YOUR_USERNAME/cs2kz-showpos.git
git push -u origin main
```

`git config` 未加 `--global`，只设置本仓库。提交者姓名/邮箱用于 Git 历史，与 `meta list` 的插件作者字符串互不影响；无需把两人的名字都写成一个 Git 提交账号。

首次 push 按 Git Credential Manager 提示在浏览器授权。不要把 GitHub 网页登录密码当作 Git HTTPS 密码，也不要把 token 写进远程 URL 或提交到仓库。[GitHub 导入本地代码](https://docs.github.com/en/migrations/importing-source-code/using-the-command-line-to-import-source-code/adding-locally-hosted-code-to-github)、[GitHub 身份验证](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/about-authentication-to-github)

若第 3 步已上传过源码，后续从远程 clone 到新的本地目录，不要用 `push --force` 覆盖已有历史：

```powershell
git clone https://github.com/YOUR_USERNAME/cs2kz-showpos.git
Set-Location cs2kz-showpos
```

把更新后的源码复制进去，再 `git add .`、`git commit -m "Describe the change"`、`git push`。

## 6. 后续更新

每次功能修复后：更新版本号和 CHANGELOG，重新编译与验证，推送源码，再发布指向该源码提交的新标签和新 Windows 包。已有公开版本的附件保持与对应源码一致。

`GetURL()` 当前为空。仓库创建后，可把实际仓库 URL 填入 `src/plugin.h` 的 `GetURL()`，重新编译；填入真实地址即可，无需猜测作者的 GitHub 用户名。

如果需要两人共同维护，在仓库设置中邀请另一个人的真实 GitHub 账号为协作者；这与插件显示 `hm_error and bell_meow` 是两件独立的事。
