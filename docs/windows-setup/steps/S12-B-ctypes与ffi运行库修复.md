# S12-B：_ctypes依赖与ffi运行库修复

当前：修复已验证成功。用户retry1进程存活，21:11:51出现全局模型成功加载日志；助手复核日志无后续异常、ffi-8.dll来源hash一致。用户随后明确确认GUI可见可操作，S12完整通过；SfM数据处理尚未开始。本文件保留故障与操作供查阅，无须重复复制或启动。

## 已有证据

内嵌Python3.10.22初始化成功，实际已找到GlobalFeatureExtractor.py及NetVlad.py。第一条真实错误位于torch→ctypes→_ctypes导入链。

dumpbin显示_ctypes.pyd直接依赖ffi-8.dll。正确DLL位于E:\sfm\envs\feedback310\Library\bin，31232字节；UI.exe输出目录缺少它。Conda libffi3.7.0、build h3d046cb_1；SHA256为922FE231D83F412D351E2560714E248D097AAE0B44C8410FB1826411E2528E69。

独立工具进程的LoadLibraryExW对照：仅注册Python根目录时加载失败，错误126；增加Conda Library\bin后加载成功，错误0。该测试没有启动模型，正式UI重试仍是最终验收。

Python扩展的依赖搜索与普通PATH查找不同，依据[Python官方说明](https://docs.python.org/3.10/library/os.html#os.add_dll_directory)。微软[LoadLibraryEx说明](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw)表明DEFAULT_DIRS包含应用目录，因此本轮补充部署这一已确认的运行DLL到UI.exe同目录；保持源码和包版本。

## 1. 关闭当前UI

通过窗口右上角关闭当前程序，继续使用之前设置运行环境的PowerShell窗口。检查：

```powershell
Get-Process -Name UI -ErrorAction SilentlyContinue | Select-Object Id,Path
```

通过标准：不再列出E:\sfm\runtime\Release\UI.exe进程。若关闭后仍存在，先记录情况，不强制终止或继续启动第二份程序。

## 2. 补齐DLL并检查

```powershell
Copy-Item -LiteralPath 'E:\sfm\envs\feedback310\Library\bin\ffi-8.dll' -Destination 'E:\sfm\runtime\Release\ffi-8.dll' -Force
Test-Path -LiteralPath 'E:\sfm\runtime\Release\ffi-8.dll'
(Get-FileHash -LiteralPath 'E:\sfm\envs\feedback310\Library\bin\ffi-8.dll' -Algorithm SHA256).Hash -eq (Get-FileHash -LiteralPath 'E:\sfm\runtime\Release\ffi-8.dll' -Algorithm SHA256).Hash
```

通过标准：复制无报错，两项检查True。这里只证明运行文件已补齐，接下来必须验证内嵌导入。

## 3. 重试启动

仍在原PowerShell窗口中执行，继承上一轮已设置的运行PATH、CUDA_PATH、Qt插件路径及GLOG_logtostderr。如果另开终端，先执行原S12说明B部分的环境设置，再执行下方启动命令。

```powershell
Set-Location -LiteralPath 'E:\sfm\runtime\Release'
$taskUiProcess = Start-Process -FilePath 'E:\sfm\runtime\Release\UI.exe' -WorkingDirectory 'E:\sfm\runtime\Release' -RedirectStandardOutput 'E:\sfm\setup\S12B-retry1-UI-stdout.log' -RedirectStandardError 'E:\sfm\setup\S12B-retry1-UI-stderr.log' -PassThru
Start-Sleep -Seconds 10
$taskUiProcess.Refresh()
$taskUiProcess | Select-Object Id,HasExited
if ($taskUiProcess.HasExited) { $taskUiProcess.ExitCode }
Get-Content -LiteralPath 'E:\sfm\setup\S12B-retry1-UI-stderr.log' -Tail 70
```

首次日志不覆盖。模型尚在加载时，等待后再读取retry1日志。

## 4. 验收

通过标准：窗口稳定、进程存活；原_ctypes错误消失；出现Python3.10.22和“The global extractor has been successfully loaded!”；没有后续DLL、导入或模型异常。若原错误消失但出现新错误，记录为部分推进，S12-B仍待完成。

返回两项DLL检查结果、进程状态与重试日志末尾，以及窗口是否显示。若失败，不重新编译或重装全部包，按新错误进一步定位。
