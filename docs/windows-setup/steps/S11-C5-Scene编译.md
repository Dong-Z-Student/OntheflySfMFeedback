# S11-C5：单独编译Scene

当前：已通过。用户Scene首次741警告/0错误、退出0，耗时36.93秒；助手于2026-10-14 20:13复核Scene.lib为178690940字节，时间20:12:08，实际nvcc路径为D:\CUDA\v12.2\bin\nvcc.exe。本文件保留操作供查阅，下一步C6生成Estimator。

Scene包含相机、影像、数据库、重建、网格、路径等代码，另有ray_tetra_cuda.cu的CUDA计算部分。工程源文件多于Geometry，日志会包含C++及CUDA两个阶段；沿用已验收的Release/x64公共配置和CUDA12.2。

## 1. 编译

```powershell
$env:CUDA_PATH = 'D:\CUDA\v12.2'
$env:PATH = "$env:CUDA_PATH\bin;$env:PATH"
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\src\Scene\Scene.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-Scene-build.log;Verbosity=normal;Encoding=UTF-8'
$taskSceneBuildExit = $LASTEXITCODE
$taskSceneBuildExit
```

等待命令完整结束，再保存退出码。当前只生成Scene，glog兼容头已经由公共属性表接入C++编译。

## 2. 检查产物

退出0后执行：

```powershell
Test-Path 'E:\sfm\runtime\Release\Scene.lib'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\Scene.lib' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-Scene-build.log'
```

通过标准：生成成功、0错误、退出0，两项路径True，Scene.lib非空且生成时间对应此次首次构建。日志中CUDA编译器应为D:\CUDA\v12.2\bin\nvcc.exe。

## 3. 返回结果

返回末尾生成汇总、退出码及文件大小/时间。若失败，保留第一条原始C++或nvcc错误及前后内容和末尾汇总，不仅返回最后的MSB3721等工具退出摘要。定位命令：

```powershell
Select-String -LiteralPath 'E:\sfm\setup\S11C-Scene-build.log' -Pattern 'error','fatal' -Context 3,5 | Select-Object -First 6
Get-Content -LiteralPath 'E:\sfm\setup\S11C-Scene-build.log' -Tail 35
```

C5通过后进入C6（Estimator）；整体功能验证仍需其余模块及UI生成后继续进行。
