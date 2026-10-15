# S11-C3：glog0.7.1头文件C4996局部兼容修复

当前：修复已通过正式Base编译验收。用户重试80警告/0错误、退出0；Base.lib29458872字节，时间2026-10-14 19:59:23。助手复核cl命令中/sdl和/FI兼容头均存在。首次失败记录和下方修复操作保留供追溯，当前无需重做C3。

## 原因和修复范围

首个错误为vcpkg_installed\x64-windows\include\glog\logging.h(503,38)的CustomPrefixCallback弃用声明。glog0.7.1先声明该类型为deprecated，随后在其自己的旧InitGoogleLogging重载声明中使用它；Base自身调用的是一参的InitGoogleLogging。Base当前/sdl会将C4996提升为error，见[微软SDL文档](https://learn.microsoft.com/en-us/cpp/build/reference/sdl-enable-additional-security-checks?view=msvc-170)及[glog0.7.1源码](https://github.com/google/glog/blob/v0.7.1/src/glog/logging.h)。7个错误是在不同编译单元读取同一头文件时重复触发。

新增本地local.glog-compat.h，在包含glog/logging.h的范围内push/disable4996/pop，恢复调用方原有警告政策。local.windows.late.props将其作为Base、Geometry、Scene、Estimator、Feature、Workflow、UI的C++强制包含文件，以覆盖这些模块直接及传递包含glog的情况；只在既有Release/x64公共配置中生效。SIFTGPU/VLFeat不加入该强制包含，CUDA编译项未改。依赖版本、SDL配置及C++算法源码不变。

助手测试：同一最小MSBuild/SDL编译，旧属性表退出1并报同一C4996；新属性表退出0并生成对象，命令仍含/sdl和/FI；第三个测试在调用方代码主动使用deprecated函数，仍报自身C4996错误（预期非0），证明没有全局关闭弃用检查。报告在feedback_local_setup\validation\glog-sdl-probe。正式Base尚未重编译。

## 1. 关闭VS，备份并复制两个修复文件

```powershell
$taskGlogPropsBackup = 'E:\sfm\setup\S11C3-late-props-before-glog-fix.props'
if (-not (Test-Path -LiteralPath $taskGlogPropsBackup)) {
    Copy-Item -LiteralPath 'E:\sfm\OntheflySfMFeedback\local.windows.late.props' -Destination $taskGlogPropsBackup
}
Copy-Item -LiteralPath '<原工作记录目录>\feedback_local_setup\local.glog-compat.h' -Destination 'E:\sfm\OntheflySfMFeedback\local.glog-compat.h' -Force
Copy-Item -LiteralPath '<原工作记录目录>\feedback_local_setup\local.windows.late.props' -Destination 'E:\sfm\OntheflySfMFeedback\local.windows.late.props' -Force
```

检验：

```powershell
Test-Path 'E:\sfm\OntheflySfMFeedback\local.glog-compat.h'
Test-Path 'E:\sfm\setup\S11C3-late-props-before-glog-fix.props'
Select-String -LiteralPath 'E:\sfm\OntheflySfMFeedback\local.windows.late.props' -Pattern 'ForcedIncludeFiles'
```

前两项True，最后显示强制包含local.glog-compat.h的那一行。原Base.vcxproj的SDLCheck保持true，不更改vcpkg下载缓存中的头文件。

## 2. 重试Base，并保存单独日志

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\src\Base\Base.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-Base-retry1-build.log;Verbosity=normal;Encoding=UTF-8'
$taskBaseBuildExit = $LASTEXITCODE
$taskBaseBuildExit
```

原S11C-Base-build.log保留本次失败记录；已由助手另存快照。

## 3. 验收

退出0后执行：

```powershell
Test-Path 'E:\sfm\runtime\Release\Base.lib'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\Base.lib' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-Base-retry1-build.log'
```

生成成功、0错误、退出0，两个路径True，Base.lib非空且为本轮产物才通过。实际cl命令应仍有/sdl，并新增/FI本地兼容头。若有其它错误，返回新的首条error及上下文和末尾汇总；不依据最小测试成功代替完整Base构建验收。
