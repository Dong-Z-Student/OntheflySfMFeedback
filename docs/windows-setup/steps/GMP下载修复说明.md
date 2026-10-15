# S08-B：GMP旧版MSYS2工具包下载修复

日期：2026-10-14。当前状态：修复已通过。用户完整manifest安装成功，退出码0；助手复核GMP6.3.0#2为installed，头文件与库存在。本文件保留修复操作供后续复用；当前无需重做S08。

本次install退出1，表示失败。GMP需要的autoconf2.71-2.71-3包在多个源返回404；Ceres2.2.0#4等已构建并缓存，可以继续复用。

修复采用vcpkg官方overlay端口机制：GMP保持6.3.0#2，只将Autoconf2.71的MSYS2包装修订3改为4，并同步SHA512。复制了固定vcpkg基线中的全部9个GMP端口文件，只修改portfile.cmake两行；项目基线保持原值。后续构建也须使用此overlay路径。

来源：[MSYS2官方包页](https://packages.msys2.org/packages/autoconf2.71?repo=msys&variant=x86_64)、[vcpkg overlay文档](https://learn.microsoft.com/en-us/vcpkg/concepts/overlay-ports)。包从https://repo.msys2.org/msys/x86_64/autoconf2.71-2.71-4-any.pkg.tar.zst取得，335194字节，SHA256与官方一致：

```text
SHA256 55BE2B911BEAA0424D60FDCF206F2411A0EB24CB53E66A82964497EF8A4FAAC0
SHA512 C93B791EB55893CBE7C425E764074837355FD165DEB7B1775F652C8E25D9D1F0CDD4120AB710D56FB859B7DF55C4F971ECCDA7C112448F60615BFF8A2DC81166
```

1. 将已准备的修复端口及下载包复制到短路径：

```powershell
New-Item -ItemType Directory -Force -Path 'E:\sfm\overlay-ports' | Out-Null
Copy-Item -LiteralPath '<原工作记录目录>\feedback_local_setup\overlay-ports\gmp' -Destination 'E:\sfm\overlay-ports' -Recurse -Force
Copy-Item -LiteralPath '<原工作记录目录>\feedback_local_setup\downloads\autoconf2.71-2.71-4-any.pkg.tar.zst' -Destination 'E:\sfm\vcpkg\downloads\autoconf2.71-2.71-4-any.pkg.tar.zst' -Force
```

2. 校验复制结果：

```powershell
$taskAutoconfHash = (Get-FileHash -LiteralPath 'E:\sfm\vcpkg\downloads\autoconf2.71-2.71-4-any.pkg.tar.zst' -Algorithm SHA256).Hash
$taskAutoconfHash -eq '55BE2B911BEAA0424D60FDCF206F2411A0EB24CB53E66A82964497EF8A4FAAC0'
Test-Path 'E:\sfm\overlay-ports\gmp\vcpkg.json'
Select-String -LiteralPath 'E:\sfm\overlay-ports\gmp\portfile.cmake' -Pattern 'autoconf2.71-2.71-4','c93b791eb55893cb'
```

前两项应为True；Select-String应返回2.71-4 URL和对应SHA512两行。若校验False或文件不存在，先处理复制问题。

3. 恢复当前PowerShell环境，追加日志并重试整份manifest：

```powershell
$env:VCPKG_ROOT = 'E:\sfm\vcpkg'
$env:CUDA_PATH = 'D:\CUDA\v12.2'
$env:PATH = "$env:CUDA_PATH\bin;$env:PATH"
$env:VCPKG_FORCE_DOWNLOADED_BINARIES = '1'
Remove-Item Env:\VCPKG_FORCE_SYSTEM_BINARIES -ErrorAction SilentlyContinue
$env:VCPKG_DEFAULT_BINARY_CACHE = 'E:\sfm\vcpkg-cache'
$env:VCPKG_MAX_CONCURRENCY = '4'
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
Start-Transcript -Path 'E:\sfm\setup\S08-vcpkg-install.log' -Append
& 'E:\sfm\vcpkg\vcpkg.exe' install --triplet x64-windows --host-triplet x64-windows --overlay-triplets='E:\sfm\triplets' --overlay-ports='E:\sfm\overlay-ports'
$taskVcpkgInstallExit = $LASTEXITCODE
Stop-Transcript
$taskVcpkgInstallExit
```

观察安装计划或日志中的GMP本地端口路径E:\sfm\overlay-ports\gmp；命令含--overlay-ports，vcpkg即优先采用本地GMP配方。下载包已放入downloads，vcpkg仍会检查其SHA512。无需重克隆或删除已成功的库。

4. 完整安装通过条件：整次install退出0，末尾确认全部请求包安装完成。随后执行：

```powershell
& 'E:\sfm\vcpkg\vcpkg.exe' list --x-install-root='E:\sfm\OntheflySfMFeedback\vcpkg_installed'
Test-Path 'E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\include\ceres\ceres.h'
Test-Path 'E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\include\CGAL'
Test-Path 'E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\include\eigen3\Eigen\Core'
```

三个存在性检查应为True；存在文件仅是补充证据，不能替代完整install退出0。若再次失败，返回首个失败包和最后错误；本修复解决当前旧Autoconf地址，不保证其余历史下载地址及实际编译一定成功。
