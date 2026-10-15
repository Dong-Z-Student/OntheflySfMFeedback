# S11-C7：单独编译Feature

当前：已通过。用户确认上一轮正确完成；助手于2026-10-14 20:26复核日志156警告/0错误、4.07秒，Feature.lib为29944734字节，时间20:25:28。退出码验收依据用户完成确认，未重跑构建。本文件保留操作供查阅，下一步C8生成Workflow。

Feature工程包括影像读取、特征提取、特征匹配以及PyLoader.cpp。沿用Release/x64公共配置、Python/NumPy头文件路径和glog兼容头；编译通过后，内嵌Python运行仍需在完整程序中验证。

## 1. 编译

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\src\Feature\Feature.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-Feature-build.log;Verbosity=normal;Encoding=UTF-8'
$taskFeatureBuildExit = $LASTEXITCODE
$taskFeatureBuildExit
```

等待命令完整结束，再立即保存退出码。当前只生成Feature。

## 2. 检查产物

退出0后执行：

```powershell
Test-Path 'E:\sfm\runtime\Release\Feature.lib'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\Feature.lib' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-Feature-build.log'
```

通过标准：0错误、退出0，两项路径True，Feature.lib非空且时间对应此次首次构建。

## 3. 返回结果

返回末尾警告/错误汇总、退出码、文件大小与时间。若失败，保留完整日志，并提供第一条实际错误及上下文：

```powershell
Select-String -LiteralPath 'E:\sfm\setup\S11C-Feature-build.log' -Pattern 'error','fatal' -Context 3,5 | Select-Object -First 6
Get-Content -LiteralPath 'E:\sfm\setup\S11C-Feature-build.log' -Tail 35
```

C7通过后进入C8（Workflow）；完整GUI、内嵌Python及SfM/Feedback验证尚待后续。
