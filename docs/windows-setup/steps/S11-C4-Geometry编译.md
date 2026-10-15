# S11-C4：单独编译Geometry

当前：已通过。用户Geometry首次35警告/0错误、退出0，耗时21.56秒；助手于2026-10-14 20:07复核Geometry.lib50846562字节，时间20:06:05。本文件保留操作供查阅，下一步C5生成Scene。

## 1. 编译

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\src\Geometry\Geometry.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-Geometry-build.log;Verbosity=normal;Encoding=UTF-8'
$taskGeometryBuildExit = $LASTEXITCODE
$taskGeometryBuildExit
```

公共属性及glog兼容头已接入该模块，沿用Release/x64和固定工具集。等待此命令结束，再保存退出码。

## 2. 检查产物

退出0后执行：

```powershell
Test-Path 'E:\sfm\runtime\Release\Geometry.lib'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\Geometry.lib' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-Geometry-build.log'
```

通过标准：生成成功、0错误、退出0，两个路径True，Geometry.lib非空且时间对应本轮首次编译。

## 3. 返回结果

返回末尾汇总、退出码、库文件大小/时间。若失败，返回第一条实际error及上下文和末尾汇总；日志原件保留。必要时筛选：

```powershell
Select-String -LiteralPath 'E:\sfm\setup\S11C-Geometry-build.log' -Pattern 'error','fatal' -Context 3,5 | Select-Object -First 6
Get-Content -LiteralPath 'E:\sfm\setup\S11C-Geometry-build.log' -Tail 30
```

C4通过后继续C5（Scene），整体S11还需其余模块及UI完成生成。
