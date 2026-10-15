# Windows 11 本地复现记录

本目录记录OntheflySfMFeedback在一台Windows 11、RTX 4060 Laptop GPU机器上的完整构建与功能复现过程。它是个人验证分支的环境记录，绝对路径和版本应按目标机器调整。

## 已验证基准

| 组件 | 版本/状态 |
| --- | --- |
| 上游源码 | `e2f511042edb93db4b0c375527ea7ecc6d0ef794` |
| 本机配置提交 | `f3d023b30e34164b840d8534fde05b9fa1988ec7` |
| Visual Studio | VS 2022 Community 17.13.35828.75 |
| MSVC | 14.36.32532，工具集v143 |
| Windows SDK | 10.0.26100.0（另安装10.0.22621.0） |
| Qt | 5.12.12 msvc2017_64；Qt VS Tools 3.5.0 |
| CUDA | Toolkit 12.2 Update 2，nvcc 12.2.140 |
| Python | 3.10.22，64位 |
| PyTorch | 2.2.2+cu118；torchvision 0.17.2+cu118；torchaudio 2.2.2+cu118 |
| oneTBB | 2021.9.0 |
| vcpkg基线 | `b1b19307e2d2ec1eefbdb7ea069de7d4bcd31f01` |

## 配置阶段

1. 安装VS C++桌面开发、Python工具和MSVC 14.36组件。
2. 安装Qt 5.12.12 MSVC 64位并在Qt VS Tools中注册。
3. 安装CUDA Toolkit 12.2的Runtime、Development和Visual Studio Integration。
4. 创建独立Python 3.10环境，安装固定版本的PyTorch与科学计算依赖。
5. 准备四个NetVLAD/PCA模型文件；模型权重不纳入Git。
6. 将vcpkg固定到manifest声明的基线，安装`x64-windows`依赖。
7. 准备oneTBB 2021.9.0 Windows SDK。
8. 配置`settings.ini`中的Python、模型和运行参数。
9. 将本分支的两个本机property sheet导入九个Visual Studio工程，以`Release | x64`顺序构建SIFTGPU、VLFeat、Base、Geometry、Scene、Estimator、Feature、Workflow和UI。
10. 用`windeployqt`部署Qt运行库，并部署python310.dll、ffi-8.dll和tbb12.dll。
11. 运行Python模型检查、C++属性检查、GUI启动、SfM试跑、Feedback显示检查及导出检查。
12. 用冻结脚本记录Python、Conda、vcpkg、工具版本、Git状态与关键文件哈希。

完整逐步记录、故障原因和每一步通过标准见`SETUP_LOG.zh-CN.md`。辅助脚本位于仓库的`tools/windows-local-setup`目录。

各次操作的详细说明保存在[steps目录](steps)，通过测试的Python/Conda/vcpkg版本清单保存在[tested-environment目录](tested-environment)。历史记录中的本机证据链接已标记为本机归档；照片、模型和构建日志没有随本目录上传。

### 在新机器上恢复辅助文件

以下以与本次相同的盘符目录为例。在新机器或新的工作目录中，先把仓库内的辅助包复制到原脚本约定的位置，再阅读对应步骤。原有机器已经完成这些操作，无须覆盖其配置。

```powershell
Copy-Item -LiteralPath 'E:\sfm\OntheflySfMFeedback\tools\windows-local-setup' -Destination 'E:\sfm\setup' -Recurse
Copy-Item -LiteralPath 'E:\sfm\OntheflySfMFeedback\tools\windows-local-setup\triplets' -Destination 'E:\sfm\triplets' -Recurse
Copy-Item -LiteralPath 'E:\sfm\OntheflySfMFeedback\tools\windows-local-setup\overlay-ports' -Destination 'E:\sfm\overlay-ports' -Recurse
```

GMP overlay来自固定vcpkg基线的GMP 6.3.0#2端口，仅更新Autoconf 2.71的MSYS2包装修订3→4和对应SHA512；其许可保存在`tools/windows-local-setup/overlay-ports/LICENSE.vcpkg.txt`。安装依赖时同时传入`--overlay-triplets=E:\sfm\triplets --overlay-ports=E:\sfm\overlay-ports`，详见[故障修复说明](steps/GMP下载修复说明.md)。压缩工具包需从说明中的官方地址下载，Git中只保存配方与校验值。

这些工具按本次机器的目录编写，并非自动适配任意路径的安装器。请先检查两个`.props`文件、`configure_windows_projects.py`、`check_windows_build_settings.py`和冻结脚本中的目录。`QtMsBuild`模板已改为按当前用户的`LocalAppData`定位；其余参考路径仍是`E:\sfm`、`E:\Qt`及`D:\CUDA`。已验证仓库根目录的配置保留在原提交中。

## 已验证结果

- 九个工程全部编译与链接成功，生成UI.exe。
- 内嵌Python 3.10和四个NetVLAD/PCA模型成功加载。
- South Building 128/128张影像进入同一增量SfM模型，生成31039个三维点。
- Show Mesh、Show Quality和Show Guide Path均正常显示。
- 导出的SQLite数据库完整；COLMAP二进制/文本模型、日志和匹配矩阵齐全。
- 末次Feedback记录3000个低质量面、20个聚类、928个候选视点、60个选中视点和59段路径。

网格能够生成，但稀疏区域存在长三角形，几何质量较粗。本记录证明默认软件功能链能够运行，不代表论文定量指标复现，也不代表真实无人机传输和飞控闭环已经验证。

## 不纳入Git的内容

- NetVLAD/PCA模型权重；
- South Building或其他数据集；
- Conda环境、Qt/CUDA/TBB安装目录；
- `vcpkg_installed`、构建中间文件、DLL/EXE/LIB；
- 运行数据库、导出模型和大体积构建日志；
- GitHub令牌或其他凭据。

## 双击启动

复制`tools/windows-local-setup/Launch-OntheflySfMFeedback.example.vbs`，改名为`Launch-OntheflySfMFeedback.vbs`后按本机目录修改开头四个变量。双击VBS即可设置运行所需环境并启动UI，无须手工打开PowerShell。直接双击UI.exe可能缺少Python、CUDA或Qt插件搜索路径。
