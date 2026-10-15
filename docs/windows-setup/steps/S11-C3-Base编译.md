# S11-C3：单独编译Base

当前：已通过。glog局部修复后正式Base重试80警告/0错误、退出0；Base.lib29458872字节，时间19:59:23。助手于2026-10-14 20:02复核日志和产物，/sdl仍启用。本文件保留首次编译步骤，当前下一步C4生成Geometry。

仓库的Base在Release/x64配置中为StaticLibrary；其它配置可能为Application，本轮必须沿用已验收的Release/x64。

## 1. 编译

在PowerShell执行：

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\src\Base\Base.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-Base-build.log;Verbosity=normal;Encoding=UTF-8'
$taskBaseBuildExit = $LASTEXITCODE
$taskBaseBuildExit
```

继续使用S11-B固定的工具集/目录，不重复安装依赖，不生成整份解决方案。命令结束后立即保存退出码。

## 2. 检查产物

退出码0后执行：

```powershell
Test-Path 'E:\sfm\runtime\Release\Base.lib'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\Base.lib' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-Base-build.log'
```

通过标准：生成成功、0错误、退出0；两项存在性检查True；Base.lib非空，时间对应首次编译。若有warning，保留日志并与error分开记录。

## 3. 返回结果

返回末尾生成汇总、退出码和库文件大小/时间；若失败，保留首条原始error及上下文和末尾汇总。完整日志E:\sfm\setup\S11C-Base-build.log，必要时筛选：

```powershell
Select-String -LiteralPath 'E:\sfm\setup\S11C-Base-build.log' -Pattern 'error','fatal' -Context 3,5 | Select-Object -First 6
Get-Content -LiteralPath 'E:\sfm\setup\S11C-Base-build.log' -Tail 30
```

C3验收后进入C4（Geometry），整个项目仍需其余库与UI.exe完成生成及运行验证。
