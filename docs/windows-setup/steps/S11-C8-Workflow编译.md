# S11-C8：编译Workflow

当前：已通过。用户确认上一轮正确完成；助手于2026-10-14 20:30复核日志1433警告/0错误、69.99秒，Workflow.lib为265410444字节，时间20:29:32。日志包含moc生成源编译。退出码验收依据用户完成确认，未重跑构建。本文件保留操作供查阅，下一步C9生成UI.exe。

Workflow包含影像读取、特征提取/匹配、检索、重建及反馈任务的组织代码；工程包含QtMoc项目，需要Qt的代码生成步骤。继续沿用已验收的Release/x64公共配置及glog兼容头。

## 1. 编译

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\src\Workflow\Workflow.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-Workflow-build.log;Verbosity=normal;Encoding=UTF-8'
$taskWorkflowBuildExit = $LASTEXITCODE
$taskWorkflowBuildExit
```

等待命令完整结束，再立即保存退出码。该工程引用前七个项目，因此日志中可能同时出现这些项目的检查或构建内容；保持Build，不改为Rebuild。

## 2. 检查产物

退出0后执行：

```powershell
Test-Path 'E:\sfm\runtime\Release\Workflow.lib'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\Workflow.lib' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-Workflow-build.log'
```

通过标准：0错误、退出0，两项路径True，Workflow.lib非空且时间对应此次首次构建。构建成功尚不代表任务流程或Qt运行验证成功。

## 3. 返回结果

返回末尾警告/错误汇总、退出码、文件大小与时间。若失败，保留完整日志，并提供第一条实际错误及上下文；若涉及QtMoc/QtMsBuild，也提供该处具体报错：

```powershell
Select-String -LiteralPath 'E:\sfm\setup\S11C-Workflow-build.log' -Pattern 'error','fatal','错误' -Context 3,5 | Select-Object -First 6
Get-Content -LiteralPath 'E:\sfm\setup\S11C-Workflow-build.log' -Tail 35
```

C8通过后进入C9（UI编译与链接）；完整GUI、内嵌Python及SfM/Feedback验证尚待后续。
