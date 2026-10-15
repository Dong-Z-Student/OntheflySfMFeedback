# S11-C2：单独编译VLFeat

当前：已通过。用户输出生成成功、0警告/0错误、退出0，用时0.71秒；助手于2026-10-14 19:38复核VLFeat.lib为3322520字节，时间19:37:29，源码版本0.9.20。本文件保留操作供查阅，当前下一步C3编译Base。

## 1. 编译Release/x64

继续使用VS自带MSBuild与已经验收的公共配置，保存独立日志：

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' 'E:\sfm\OntheflySfMFeedback\thirdparty\VLFeat\VLFeat.vcxproj' /t:Build /p:Configuration=Release /p:Platform=x64 /m:1 /v:normal /nologo '/flp:LogFile=E:\sfm\setup\S11C-VLFeat-build.log;Verbosity=normal;Encoding=UTF-8'
$taskVlfeatBuildExit = $LASTEXITCODE
$taskVlfeatBuildExit
```

等待命令结束再保存退出码。本阶段只生成VLFeat；不重做已通过的SIFTGPU。

## 2. 检查产物

退出码0后执行：

```powershell
Test-Path 'E:\sfm\runtime\Release\VLFeat.lib'
Get-Item -LiteralPath 'E:\sfm\runtime\Release\VLFeat.lib' | Select-Object Name,Length,LastWriteTime
Test-Path 'E:\sfm\setup\S11C-VLFeat-build.log'
```

通过标准：输出生成成功、0错误、退出0；两个存在性检查True；VLFeat.lib非空，生成时间对应此次首次构建。警告和错误分别记录，不把只存在的残留产物当作整次构建通过。

## 3. 返回结果

返回末尾生成汇总、退出码、产物文件大小/时间。若失败，保留第一条原始error及前后内容和末尾汇总；完整日志E:\sfm\setup\S11C-VLFeat-build.log。必要时定位：

```powershell
Select-String -LiteralPath 'E:\sfm\setup\S11C-VLFeat-build.log' -Pattern 'error','fatal' -Context 3,5 | Select-Object -First 6
Get-Content -LiteralPath 'E:\sfm\setup\S11C-VLFeat-build.log' -Tail 30
```

C2通过后进入C3生成Base。完成这两个第三方静态库仍不代表UI程序已生成或SfM功能已复现。
