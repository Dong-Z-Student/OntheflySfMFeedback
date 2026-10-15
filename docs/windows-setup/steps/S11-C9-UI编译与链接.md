# S11-C9：编译并链接UI

当前：已通过。用户549警告/0错误、退出0，耗时180.85秒；助手于2026-10-14 20:39复核UI.exe为5773312字节，时间20:35:22。八个静态库及UI.exe均已生成，S11构建完成。本文件保留操作供查阅，下一步S12部署与启动。

本步骤编译界面、生成Qt代码和资源，并将此前静态库与外部依赖链接成可执行文件。沿用公共配置、glog兼容头和UI的tbb12.lib补充项。成功生成exe后进入S12部署及首次启动。

## 1. 编译与链接

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\src\UI\UI.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-UI-build.log;Verbosity=normal;Encoding=UTF-8'
$taskUiBuildExit = $LASTEXITCODE
$taskUiBuildExit
```

等待命令完整结束，再立即保存退出码。保持Build，无须改为Rebuild；日志可能包含引用工程的检查内容。

## 2. 检查产物

退出0后执行：

```powershell
Test-Path 'E:\sfm\runtime\Release\UI.exe'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\UI.exe' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-UI-build.log'
```

通过标准：0错误、退出0，两项路径True，UI.exe非空且时间对应此次首次构建。此步骤仅验收构建；运行所需DLL、INI及Python环境在S12继续检查。

## 3. 返回结果

返回末尾警告/错误汇总、退出码、文件大小与时间。若失败，保留完整日志并提供第一条实际编译或链接错误及上下文：

```powershell
Select-String -LiteralPath 'E:\sfm\setup\S11C-UI-build.log' -Pattern 'error','fatal','错误','LNK\d+' -Context 3,5 | Select-Object -First 8
Get-Content -LiteralPath 'E:\sfm\setup\S11C-UI-build.log' -Tail 40
```

若有LNK错误，提供具体缺失库名或未解析符号，不仅提供最后的LNK1120汇总；不要自行删除依赖、复制网上DLL或压制错误。

C9通过后S11的构建部分完成；接下来S12进行运行依赖部署与首次启动，S13/S14验证实际SfM/Feedback功能。
