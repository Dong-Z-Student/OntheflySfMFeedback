# S09：准备oneTBB2021.9.0 Windows SDK

状态：已完成。用户复制到E:\sfm\onetbb后，助手于2026-10-14 18:37重复确认三项文件存在、版本宏2021/9/0；官方ZIP SHA256此前已通过。本文件保留操作步骤，当前无需重复复制。项目接入与编译在S11验证。

1. 从[官方发布页](https://github.com/uxlfoundation/oneTBB/releases/tag/v2021.9.0)的Assets下载oneapi-tbb-2021.9.0-win.zip（约14.9MB），保存为E:\sfm\setup\oneapi-tbb-2021.9.0-win.zip。

直达：[Windows SDK ZIP](https://github.com/uxlfoundation/oneTBB/releases/download/v2021.9.0/oneapi-tbb-2021.9.0-win.zip)。该步骤不需要下载Source code或安装完整oneAPI套件。

2. 核对官方SHA256，应为True：

```powershell
(Get-FileHash -LiteralPath 'E:\sfm\setup\oneapi-tbb-2021.9.0-win.zip' -Algorithm SHA256).Hash -eq '0C4D96EDD0469CC93F611F94365EC07F3A0DD529B96D4E6CEAC378DE63920B1C'
```

3. 解压并将SDK内容整理到E:\sfm\onetbb。下面命令从解压内容中寻找真正的SDK根目录，保留下载ZIP及解压副本。若最终目标已存在，先检查现有内容，命令会停止以免覆盖另一份SDK。

```powershell
$taskTbbZip = 'E:\sfm\setup\oneapi-tbb-2021.9.0-win.zip'
$taskTbbUnpacked = 'E:\sfm\setup\onetbb-2021.9.0-unpacked'
$taskTbbTarget = 'E:\sfm\onetbb'
if (Test-Path -LiteralPath $taskTbbTarget) { throw 'E:\sfm\onetbb already exists; inspect its contents first.' }
if ((Get-FileHash -LiteralPath $taskTbbZip -Algorithm SHA256).Hash -ne '0C4D96EDD0469CC93F611F94365EC07F3A0DD529B96D4E6CEAC378DE63920B1C') { throw 'ZIP SHA256 mismatch.' }
Expand-Archive -LiteralPath $taskTbbZip -DestinationPath $taskTbbUnpacked -Force
$taskTbbCandidates = @($taskTbbUnpacked) + @(Get-ChildItem -LiteralPath $taskTbbUnpacked -Directory -Recurse | Select-Object -ExpandProperty FullName)
$taskTbbSdkRoots = @($taskTbbCandidates | Where-Object { Test-Path -LiteralPath (Join-Path $_ 'include\tbb\global_control.h') })
if ($taskTbbSdkRoots.Count -ne 1) { throw 'Cannot identify one SDK root; inspect the extracted ZIP.' }
New-Item -ItemType Directory -Path $taskTbbTarget | Out-Null
Copy-Item -Path (Join-Path $taskTbbSdkRoots[0] '*') -Destination $taskTbbTarget -Recurse
```

也可以使用资源管理器解压后，新建E:\sfm\onetbb，将包含include、lib、redist的那层目录内容复制进去。最终必须直接有E:\sfm\onetbb\include\tbb。

本机已经解压，无需重复解压，可直接执行：

```powershell
New-Item -ItemType Directory -Force -Path 'E:\sfm\onetbb' | Out-Null
Copy-Item -Path 'E:\sfm\setup\oneapi-tbb-2021.9.0-win\oneapi-tbb-2021.9.0\*' -Destination 'E:\sfm\onetbb' -Recurse
```

4. 验收：

```powershell
Test-Path 'E:\sfm\onetbb\include\tbb\global_control.h'
Get-ChildItem -LiteralPath 'E:\sfm\onetbb' -Recurse -Filter 'tbb12.lib' | Select-Object -ExpandProperty FullName
Get-ChildItem -LiteralPath 'E:\sfm\onetbb' -Recurse -Filter 'tbb12.dll' | Select-Object -ExpandProperty FullName
Get-Content -LiteralPath 'E:\sfm\onetbb\include\oneapi\tbb\version.h' | Select-String '^#define TBB_VERSION_(MAJOR|MINOR|PATCH)'
```

通过标准：头文件True；能找到intel64目录下的Release tbb12.lib与tbb12.dll；版本宏为MAJOR2021、MINOR9、PATCH0。本次ZIP实际桌面x64目录为lib\intel64\vc14与redist\intel64\vc14；vc14_uwp对应另一种应用类型，本项目使用普通vc14。带_debug后缀的库用于Debug，本轮后续选Release/x64。

返回上述输出后，将记录实际include/lib/bin路径，并进入S10模型独立加载。
