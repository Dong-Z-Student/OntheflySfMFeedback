# S11-C6：单独编译Estimator

当前：已通过。用户确认上一轮正确完成；助手于2026-10-14 20:23复核日志108警告/0错误、54.49秒，Estimator.lib为185752884字节，时间20:22:03。退出码验收依据用户完成确认，未重跑构建。本文件保留操作供查阅，下一步C7生成Feature。

Estimator包括束调整、基础矩阵/本质矩阵/单应矩阵估计、增量建图及三角化等代码。沿用已验收的Release/x64公共配置和glog兼容头。

## 1. 编译

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\src\Estimator\Estimator.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-Estimator-build.log;Verbosity=normal;Encoding=UTF-8'
$taskEstimatorBuildExit = $LASTEXITCODE
$taskEstimatorBuildExit
```

等待命令完整结束，立即保存退出码。当前只生成Estimator；不要重复配置、重装依赖或提前编译整套解决方案。

## 2. 检查产物

退出0后执行：

```powershell
Test-Path 'E:\sfm\runtime\Release\Estimator.lib'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\Estimator.lib' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-Estimator-build.log'
```

通过标准：生成成功、0错误、退出0，两项路径True，Estimator.lib非空且生成时间对应此次首次构建。构建通过后仍需完整项目链接与功能测试。

## 3. 返回结果

返回末尾生成汇总、退出码及文件大小/时间。若失败，保留第一条实际编译错误及上下文和末尾汇总：

```powershell
Select-String -LiteralPath 'E:\sfm\setup\S11C-Estimator-build.log' -Pattern 'error','fatal' -Context 3,5 | Select-Object -First 6
Get-Content -LiteralPath 'E:\sfm\setup\S11C-Estimator-build.log' -Tail 35
```

C6通过后进入C7（Feature）；当前尚未运行完整SfM/Feedback流程。
