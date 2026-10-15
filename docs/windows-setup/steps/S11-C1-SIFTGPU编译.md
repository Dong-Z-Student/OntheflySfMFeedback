# S11-C1：首次单独编译SIFTGPU

当前：已通过。用户首次构建成功，36警告/0错误、退出0，用时18.89秒；助手于2026-10-14 19:32复核SIFTGPU.lib为22768192字节，生成时间19:30:00，cl14.36与nvcc D盘12.2路径正确。本文件保留操作供查阅，当前下一步C2编译VLFeat。

SIFTGPU是局部SIFT特征模块，包含C++和CUDA代码。此工程生成静态库SIFTGPU.lib，之后由其它模块及UI链接。

## 1. 执行编译

在PowerShell使用VS自带的MSBuild。/t:Build为增量生成，/p指定Release/x64，/m:1使用一个MSBuild节点，日志参数保存UTF-8日志。参数依据[MSBuild官方命令行说明](https://learn.microsoft.com/en-us/visualstudio/msbuild/msbuild-command-line-reference?view=vs-2022)，日志参数用单引号包住，以使分号传给MSBuild。

```powershell
$env:CUDA_PATH = 'D:\CUDA\v12.2'
$env:PATH = "$env:CUDA_PATH\bin;$env:PATH"
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
$taskSiftBuildStart = Get-Date
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\thirdparty\SIFTGPU\SIFTGPU.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-SIFTGPU-build.log;Verbosity=normal;Encoding=UTF-8'
$taskSiftBuildExit = $LASTEXITCODE
$taskSiftBuildExit
```

命令开始后等待其执行结束，不同时从VS启动另一轮生成。源文件名和编译命令会陆续显示；nvcc编译ProgramCU.cu时输出节奏可能不同。不要仅根据暂时没有新行判断失败。

可选GUI入口是VS工具栏Release/x64，在解决方案资源管理器右键SIFTGPU→生成。本轮推荐上面的命令，便于固定配置和保存完整日志。

## 2. 验收生成产物

只有编译退出码为0后，再检查：

```powershell
Test-Path 'E:\sfm\runtime\Release\SIFTGPU.lib'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\SIFTGPU.lib' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-SIFTGPU-build.log'
```

通过标准：输出显示生成成功且0个错误，退出码0，两个Test-Path均True，SIFTGPU.lib非空且生成时间对应此次首次编译。后续增量生成若显示目标已是最新，不要求已有成功产物每次都更新时间。

保留编译日志中的实际cl.exe工具集14.36.32532与nvcc.exe路径D:\CUDA\v12.2；若命令没有在普通日志显示完整cl路径，可以结合S11-B的VCToolsInstallDir核查，不能据仅看到v143推断具体MSVC小版本。

## 3. 若失败

退出码非0时，返回第一条error及其前后输出，并附末尾生成汇总，不进入下一个模块。可读取日志定位：

```powershell
Select-String -LiteralPath 'E:\sfm\setup\S11C-SIFTGPU-build.log' -Pattern 'error','fatal' -Context 3,5 | Select-Object -First 6
Get-Content -LiteralPath 'E:\sfm\setup\S11C-SIFTGPU-build.log' -Tail 30
```

error/fatal筛选仅帮助定位，日志全文应保留。最后一条MSB3721等可能只是工具退出的汇总，前面的nvcc/C++原始错误更有诊断价值。

本模块成功后继续S11-C2（VLFeat），随后Base、Geometry、Scene、Estimator、Feature、Workflow、UI。SIFTGPU.lib通过只代表此模块构建成功，整个S11还需所有模块完成。
