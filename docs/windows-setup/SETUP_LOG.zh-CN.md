# OntheflySfMFeedback 环境配置与功能复现记录

首次记录：2026-10-13 20:51（Asia/Shanghai）。最近核查：2026-10-14 22:55。

目标：先完成本机默认流程的照片输入、增量 SfM、网格/质量/引导路径显示及结果导出。论文指标复现和真实无人机飞控接入属于之后的独立验证。

项目：[IRIS-LAB-whu/OntheflySfMFeedback](https://github.com/IRIS-LAB-whu/OntheflySfMFeedback)。

当前已下载源码：`<工作记录目录>\OntheflySfMFeedback`。

正式构建源码：`E:\sfm\OntheflySfMFeedback`。用户已于21:01–21:02复制源码，助手于21:05只读复核通过。本地配置分支：`local/windows-setup`。

辅助文件：`E:\sfm\setup`，已确认配置模板存在。

上游源码基线：`e2f511042edb93db4b0c375527ea7ecc6d0ef794`。已验证的本机配置提交：`f3d023b30e34164b840d8534fde05b9fa1988ec7`。

完整参考指南：[本地配置与 Git 入门指南](<E:/101研0/on th fly SFM/OntheflySfMFeedback_本地配置与Git入门指南.md>)。本文件负责逐步验收及版本记录；安装细节与异常处理可交叉查看指南。下方命令默认在 PowerShell/Anaconda PowerShell Prompt 中执行。

## 使用规则

1. 每次只执行当前步骤，检查通过再进入下一步。
2. “已发现安装”不等于“已通过项目运行验证”；“目标版本”不等于“实际版本”。
3. 失败时保留第一条真实错误及完整输出，不盲目继续执行后面的命令。
4. 你返回步骤结果后，助手重新检查并更新本文件；没有证据的步骤保留待完成。
5. S00–S16已全部完成：12个已验证Windows配置文件已提交为f3d023b30e34164b840d8534fde05b9fa1988ec7，工作区干净；最终冻结快照也记录了该提交、175项vcpkg条目和32个文件哈希。本轮环境配置与功能复现已闭环。

## 一、进度总表

| 编号 | 步骤 | 状态 | 已有证据/通过条件 |
| --- | --- | --- | --- |
| S00 | 本机环境与仓库检查 | 已完成 | 命令版本、VS 组件、GPU、源码提交已核查 |
| S01 | 下载新项目源码 | 已完成 | Git 仓库干净，origin 正确，提交与文首一致 |
| S02 | 建立英文短路径与本地配置分支 | 已完成 | 助手复核：提交一致、origin正确、分支正确、工作区干净、四项文件检查True |
| S03 | VS 组件与兼容 MSVC | 已完成 | 五项组件均True；MSVC14.36.32532目录已复核；VS17.13.35828.75 |
| S04 | Qt 5.12.12 与 Qt VS Tools | 已完成 | 助手复核qmake及三个路径通过；用户报告扩展3.5.0；截图注册名/路径/Windows/msvc正确；QtMsBuild编译在S11核查 |
| S05 | CUDA Toolkit 12.2 | 已完成 | D:\CUDA\v12.2；nvcc12.2.140；开发文件/VS集成通过；12.4保留；驱动617.42 |
| S06 | 独立 Python 3.10 与 Python 包 | 已完成 | Python3.10.22/64位；完整包版本已记；pip check通过；GPU True与运算512.0；三个Python开发文件True |
| S07 | 四个 NetVLAD/PCA 模型 | 已完成（文件验收） | 文件大小与Drive元数据一致；助手复核SHA256与用户一致；模型加载兼容性在S10验证 |
| S08 | vcpkg manifest C++ 依赖 | **已完成（A/B均通过）** | 用户完整安装成功，用时1.1h、退出0；助手复核149个基础端口均installed、manifest直接依赖无缺失，五项头文件/目录True；保留GMP局部overlay |
| S09 | oneTBB SDK | **已完成（文件验收）** | 用户与助手均确认头文件/lib/DLL True，版本宏2021/9/0；include、lib\intel64\vc14、redist\intel64\vc14；VS接入及运行待后续 |
| S10 | INI 与 Python 模型独立验证 | **已完成（独立Python验证）** | 用户完整输出PASS、退出0；助手复核日志和INI；GPU特征(1,2048)、所有值有限、范数1.1084929019709315 |
| S11 | 工程路径修正与 C++ 编译 | **已完成（构建）** | C9用户549警告/0错误、退出0；UI.exe5773312字节；助手核查九项产物及日志 |
| S12 | DLL 部署与 GUI 启动 | **已完成（运行启动验收）** | retry1进程27640存活；Python3.10.22；21:11:51出现模型成功日志；用户确认主窗口可见可正常操作 |
| S13 | 小数据 SfM | **已完成** | 128/128张输入全部注册，31039个三维点；相机与点云可见；初始化、126条增量注册及最终渲染日志完整 |
| S14 | Feedback 显示 | **已完成** | 8次路径规划非空；网格、蓝—白—红质量面、紫红视点/橙色路径/绿色方向均有截图证据 |
| S15 | 导出与结果检查 | **已完成** | 自动检查PASS、退出0；SQLite完整；128影像/31039点；3372 matches、1178 two-view geometries；11项核心文件非空 |
| S16 | 固化环境和本机改动 | **已完成** | 最终快照PASS/退出0；提交f3d023b；分支local/windows-setup；工作区干净；vcpkg175项；32个文件哈希；HEAD补丁已保存 |

## 二、版本账本

### 2.1 已读取到的本机环境

| 部分 | 实际版本/状态 | 当前路径/证据 | 项目验证状态 |
| --- | --- | --- | --- |
| Git | 2.52.0.windows.1 | E:\Git\cmd\git.exe；git --version | 下载仓库已验证 |
| VS2022 Community | 17.13.3；安装版本 17.13.35828.75 | C:\Program Files\Microsoft Visual Studio\2022\Community；vswhere | IDE 安装已确认；项目未编译 |
| C++ 桌面开发工作负载 | 已安装 | vswhere -requires Microsoft.VisualStudio.Workload.NativeDesktop | 组件存在 |
| Python 开发工作负载 | 已安装 | vswhere -requires Microsoft.VisualStudio.Workload.Python 有匹配 | 组件存在；项目Python环境尚未创建 |
| Python 语言支持 | 已安装 | Microsoft.Component.PythonTools 查询True | 组件存在 |
| Python 本机开发工具 | 已安装 | Microsoft.ComponentGroup.PythonTools.NativeDevelopment 查询True | 组件存在；混合调试未验证 |
| MSVC | 14.36.32532、14.42.34433、14.43.34808 | VS 的 VC\Tools\MSVC 目录 | SIFTGPU实际cl.exe/Lib.exe使用14.36.32532\bin\HostX86\x64，编译及归档成功；其余模块待验收 |
| MSVC 14.36 组件 | 已安装；14.36.32532 | Microsoft.VisualStudio.Component.VC.14.36.17.6.x86.x64 查询True；目录存在 | 安装通过；项目编译在S11验证 |
| Windows SDK | 10.0.22621.0、10.0.26100.0均已安装；vcpkg env实际为10.0.26100.0 | Windows Kits\10\Include目录；用户SDK环境检查输出 | S11-B正式九工程解析目标及头文件SDK26100；S08 CMake目标变量22621另记，不能混为实际编译SDK |
| NVIDIA GPU | RTX 4060 Laptop；8188 MiB 显存 | nvidia-smi | 驱动可查询；项目 GPU 未验证 |
| NVIDIA 驱动 | 617.42 | nvidia-smi | 无须因本次配置主动回退 |
| CUDA Toolkit | 12.4；nvcc V12.4.99 | C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4 | 已验证 nvcc 命令；项目未验证 |
| Qt 6.5.3（已有） | 6.5.3目录及VS注册项 | D:\Qt\6.5.3；用户截图有msvc2019_64/arm64 | 本项目当前选择Qt5套件 |
| Qt 5.12.12 MSVC2017 64位 | Qt5.12.12；QMake3.1；win32-msvc | E:\Qt\5.12.12\msvc2017_64；正式输出Qt5Core.dll/qwindows.dll文件版本5.12.12.0 | Workflow/UI构建与主窗口运行通过 |
| Qt VS Tools | 3.5.0（用户报告） | kit名5.12.12_msvc2017_64；QtMsBuild=C:\Users\<用户名>\AppData\Local\QtMsBuild，qt.targets/qt.props/qt_defaults.props存在 | 9项目加载/正式B路径通过；Workflow/UI及Qt生成代码构建通过；运行待验证 |
| MSBuild | 17.13.19.12613 | VS2022 Community\MSBuild\Current\Bin\MSBuild.exe；助手/version读取 | 正式9工程属性评估及S11全部构建通过 |
| CUDA Toolkit 12.2 | 12.2 Update2；nvccV12.2.140；Build cuda_12.2.r12.2/compiler.33191640_0 | D:\CUDA\v12.2；SIFTGPU及Scene日志实际nvcc.exe位于此处 | ProgramCU.cu及ray_tetra_cuda.cu编译成功；实际运行待后续验证 |
| CUDA_PATH | 系统机器变量D:\CUDA\V12.2；助手既有进程仍为C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4；用户变量未设置 | 读取Process/User/Machine三层值；Windows路径大小写不影响定位 | 新开终端/重新启动VS后读取新系统值；S11仍明确指定Toolkit位置 |
| conda | 24.11.3 | D:\Anaconda\Scripts\conda.exe | 已验证 env list |
| base Python | 3.12.3 | D:\Anaconda\python.exe | 保留，不作为项目解释器 |
| 已有 torch_gpu Python | 3.12.13 | D:\conda_envs\torch_gpu\python.exe | 保留，不直接复用 |
| 已有 torch_gpu 包 | torch 2.5.1；torchvision 0.20.1；NumPy 2.4.6 | 读取包元数据；未导入 torch 执行 | 未证明此环境 GPU 可用；与参考组合不同 |
| feedback310 Python | 3.10.22；64位；conda-forge；MSC v.1944 | E:\sfm\envs\feedback310\python.exe；conda包python-3.10.22-hc20f281_0_cpython | 解释器位置/版本/位数通过 |
| feedback310 pip | 26.2.1 | 项目解释器执行pip --version | 后续原命令重试安装成功；pip check通过 |
| feedback310 基础包 | NumPy1.26.4、SciPy1.12.0、scikit-learn1.4.2、h5py3.10.0、Pillow10.2.0、joblib1.3.2 | 包元数据、pip check与S10完整输出 | 包导入、PCA加载及一次模型提取通过；项目内嵌Python未验证 |
| feedback310 torch组合 | torch2.2.2+cu118；torchvision0.17.2+cu118；torchaudio2.2.2+cu118 | 用户安装日志；助手导入并检查实际版本 | CUDA runtime11.8；GPU True；RTX4060Laptop；矩阵结果512.0；S06通过 |
| feedback310 NumPy头文件 | E:\sfm\envs\feedback310\lib\site-packages\numpy\core\include | numpy.get_include()，用户与助手输出一致 | 待S11接入C++工程 |
| feedback310 Python开发文件 | Python.h、python310.lib、python310.dll存在 | 环境include、libs、根目录；输出目录DLL3.10.22且来源hash一致 | 安装文件、C++链接与内嵌Python3.10.22初始化通过；内嵌模型加载成功 |
| CMake | 4.0.2 | E:\CMake\bin\cmake.exe | 已验证命令；vcpkg 历史端口兼容性未验证 |
| vcpkg | 2025-07-21-d4b65a2b83ae6c3526acd1c6f3b51aff2a884533 | E:\sfm\vcpkg\vcpkg.exe；源码提交b1b19307e2d2ec1eefbdb7ea069de7d4bcd31f01；非浅克隆 | S08-A/B通过；完整安装退出0，用时1.1h |
| vcpkg管理的LLVM | 15.0.6 | 用户最新日志LLVM-15.0.6-win64.exe下载成功 | GMP下载随后失败；不据辅助工具成功认定GMP完成 |
| Ceres（已安装） | 2.2.0#4；core,cuda,lapack,schur,suitesparse | 安装状态installed；ceres.h、ceres.lib、ceres_cuda_kernels.lib存在 | S08安装验收通过；项目链接/功能未验证 |
| Eigen（已安装） | 3.4.1-250818 | 最新安装计划already installed列表 | 依赖已确认；项目编译未验证 |
| Boost（已安装） | 1.88.0；boost-container带port-version1 | 安装状态文件与版本快照 | 完整manifest所需Boost子包均安装完成 |
| GMP overlay（已通过安装） | GMP仍6.3.0#2；Autoconf仍2.71，MSYS2包装修订3→4 | E:\sfm\overlay-ports\gmp；installed状态；gmp.h、gmp.lib存在 | 局部下载修复后安装通过；后续构建沿用此overlay |
| vcpkg管理的CMake | 3.30.1 | E:\sfm\vcpkg\downloads\tools\cmake-3.30.1-windows\cmake-3.30.1-windows-i386\bin\cmake.exe；助手执行--version确认 | 工具就绪；原系统CMake4.0.2保留 |
| vcpkg辅助7zip/7zr | 25.1.0 | 用户env输出下载并解压成功 | 工具已获取；不代表依赖已构建 |
| oneTBB SDK | 2021.9.0；官方ZIP15622378字节，SHA256通过；版本宏2021/9/0 | E:\sfm\onetbb；UI输出tbb12.dll2021.9、290944字节且来源hash一致 | S09文件、S11链接及S12-A部署通过；运行待S12-B |
| 项目模型/INI/C++输出 | S10模型测试PASS；八个静态库及UI.exe均已生成；S12-B内嵌模型初始化与GUI确认通过 | E:\sfm\models\netvlad；runtime\settings.ini；runtime\Release | 配置/构建/程序启动通过；SfM/Feedback完整运行待后续 |
| 仓库内VLFeat | 源码版本0.9.20；generic.h的VL_VERSION_STRING | thirdparty\VLFeat；本轮编译出VLFeat.lib | C2构建通过；运行/算法验证待完整流程 |

### 2.2 本项目计划采用的组合（逐项记录实际安装与验证）

| 部分 | 目标/约束 | 实际项目版本 | 项目位置/补充信息 |
| --- | --- | --- | --- |
| 源码 | 提交 e2f511042edb93db4b0c375527ea7ecc6d0ef794 | 短路径副本提交已复核一致 | E:\sfm\OntheflySfMFeedback；local/windows-setup |
| VS | 保留现有 VS2022 | 17.13.35828.75；五项组件已确认 | S03安装验收完成 |
| MSVC | v143，14.36.* | 14.36.32532；C1实际cl/Lib路径已核查 | 后续模块继续核查生成日志 |
| Windows SDK | 本机已安装22621/26100 | vcpkg env26100；正式九工程B为26100；S08 CMake目标变量22621 | 正式SDK目标及头文件路径已核查，实际编译在C验证 |
| Qt | 5.12.12，msvc2017_64 | 5.12.12；QMake3.1 | E:\Qt\5.12.12\msvc2017_64；S04通过 |
| Qt VS Tools | 支持 VS2022 的版本 | 3.5.0（用户报告） | Qt版本注册正确；QtMsBuild=C:\Users\<用户名>\AppData\Local\QtMsBuild |
| CUDA Toolkit | 12.2 Update2（12.2.2） | nvcc V12.2.140；安装/VS集成检查通过 | 实际D:\CUDA\v12.2，与原C盘12.4并存 |
| Python | CPython 3.10，64 位 | 3.10.22；64位；已创建 | E:\sfm\envs\feedback310 |
| PyTorch | 2.2.2/cu118 | 2.2.2+cu118；GPU运算通过 | E:\sfm\envs\feedback310；runtime11.8 |
| torchvision | 0.17.2 | 0.17.2+cu118 | 同上；导入通过 |
| torchaudio | 2.2.2 | 2.2.2+cu118 | 同上；导入通过 |
| NumPy | 1.26.4 | 1.26.4，包元数据确认 | E:\sfm\envs\feedback310\lib\site-packages\numpy\core\include |
| SciPy | 1.12.0 | 1.12.0，包元数据确认 | 从源码导入推导的参考版本 |
| scikit-learn | 1.4.2 起步 | 1.4.2，包元数据确认 | 若作者 PCA 保存版本不同，需据证据调整 |
| h5py | 3.10.0 | 3.10.0，包元数据确认 | 推导的参考版本 |
| Pillow | 10.2.0 | 10.2.0，包元数据确认 | 推导的参考版本 |
| joblib | 1.3.2 | 1.3.2，包元数据确认 | 推导的参考版本 |
| vcpkg 工具 | 定位到作者 manifest 基线的工具仓库 | 2025-07-21-d4b65a2b83ae6c3526acd1c6f3b51aff2a884533；HEAD为b1b19307e2d2ec1eefbdb7ea069de7d4bcd31f01 | E:\sfm\vcpkg；完整克隆成功 |
| vcpkg使用的CMake | 基线工具清单3.30.1 | 3.30.1，实际exe版本已确认 | VCPKG_FORCE_DOWNLOADED_BINARIES=1；原系统CMake4.0.2保留 |
| manifest 基线 | b1b19307e2d2ec1eefbdb7ea069de7d4bcd31f01 | 源码已确认 | 不主动更新 |
| C++ 依赖 | 原清单，x64-windows，overlay 选14.36 | 已安装；149个基础端口；直接依赖无缺失 | E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows；具体版本见2.4及CSV快照 |
| oneTBB | 2021.9.0 SDK | 2021.9.0，版本/文件验收通过 | E:\sfm\onetbb\include；lib\intel64\vc14；redist\intel64\vc14（桌面x64，非vc14_uwp） |
| NetVLAD/PCA 模型 | 四个匹配的作者文件 | 文件/hash验收及S10单照片加载/提取通过 | E:\sfm\models\netvlad；本机scikit-learn1.4.2可加载本次PCA并完成提取，不据此推断作者保存版本 |
| deep-image-matching | 默认调用链首轮不安装 | 不适用 | 当前初始化被注释；不能据README认为已启用 |

上述参考版本不构成作者的完整锁定文件。只有功能验证成功后，才能将实际组合标为“本机测试通过”。CUDA Toolkit 与 torch.version.cuda 分开记录，后者不是本机 nvcc 版本。

### 2.3 S06完整Python包快照（2026-10-14 01:02）

用户pip list与助手重新读取结果一致；此表记录安装版本，模型推理及项目功能仍待后续验证。

| 包 | 实际版本 |
| --- | --- |
| filelock | 3.32.3 |
| fsspec | 2026.7.0 |
| h5py | 3.10.0 |
| Jinja2 | 3.1.6 |
| joblib | 1.3.2 |
| MarkupSafe | 3.0.3 |
| mpmath | 1.3.0 |
| networkx | 3.4.2 |
| numpy | 1.26.4 |
| packaging | 26.3 |
| pillow | 10.2.0 |
| pip | 26.2.1 |
| scikit-learn | 1.4.2 |
| scipy | 1.12.0 |
| setuptools | 84.0.0 |
| sympy | 1.14.0 |
| threadpoolctl | 3.7.0 |
| torch | 2.2.2+cu118 |
| torchaudio | 2.2.2+cu118 |
| torchvision | 0.17.2+cu118 |
| typing_extensions | 4.16.0 |
| wheel | 0.48.0 |

### 2.4 S08完整C++依赖快照（2026-10-14 18:24）

用户报告All requested installations completed successfully in: 1.1 h，紧接保存LASTEXITCODE为0。助手只读复核vcpkg\status：149个带Version字段的基础端口均为install ok installed，manifest直接依赖无缺失；Ceres/CGAL/Eigen/GMP/SQLite五项头文件或目录均True，主要Release导入库已存在。该验收证明依赖安装完成，项目源码编译和功能仍待后续。

| 依赖 | 实际版本（#为vcpkg端口修订） |
| --- | --- |
| Boost清单组件及传递组件 | 1.88.0；boost-container为#1 |
| Ceres | 2.2.0#4；cuda/lapack/schur/suitesparse功能 |
| Eigen | 3.4.1-250818 |
| CGAL | 6.0.1 |
| GMP / MPFR | 6.3.0#2（本地overlay）/ 4.2.2 |
| Faiss / FLANN | 1.8.0 / 2022-10-28 |
| FreeImage / Jasper | 3.18.0#27 / 4.2.8 |
| METIS / SQLite | 2025-07-04 / 3.50.4（json1功能） |
| GLEW / glog | 2.2.0#6 / 0.7.1 |
| OpenBLAS | 0.3.29 |
| vcpkg-cmake / vcpkg-cmake-config | 2024-04-23 / 2024-05-23 |

完整149端口版本表：[CSV快照](<E:/101研0/on th fly SFM/feedback_local_setup/S08-vcpkg-installed-20261014.csv>)。含功能、依赖与状态的原始安装账本：[status快照](<E:/101研0/on th fly SFM/feedback_local_setup/S08-vcpkg-status-20261014.txt>)；原账本SHA256：3ABA3F4FB04343225E8B2503D2DAC67EE0CE43BA8B959EACC470D9FD778E5089。cuda端口显示10.1#13是端口配方版本，不据此更改实际Toolkit12.2记录。后续VS/MSBuild须保留triplet和GMP overlay路径以采用相同配方。

### 2.5 S10模型独立运行记录（2026-10-14 18:48）

* 输入：E:\sfm\data\trial01\images\00001.jpg，570051字节；SHA256：619F6733A8390B3CF62CF02AF6FF25A851D8C6D1FFB65324DC20335E0ADB45FF。
* 配置：E:\sfm\runtime\settings.ini；PythonHome=E:/sfm/envs/feedback310，PythonWorkspace=E:/sfm/OntheflySfMFeedback/scripts/pyscripts，GlobalFeatureModel=E:/sfm/models/netvlad；16键检查通过。
* 解释器：Python3.10.22；torch2.2.2+cu118，Torch runtime11.8；RTX4060 Laptop GPU；CUDA矩阵运算通过。
* 四模型加载成功；实际Extract输出(1,2048)，所有元素有限，L2范数1.1084929019709315；Result: PASS，用户立即保存的退出码0。
* 助手复核日志与INI内容；日志原件E:\sfm\setup\S10-model-test.log，快照见feedback_local_setup\S10-model-test-passed-20261014.log。
* 这是命令行Python对单张照片的验证；不代表C++已编译、内嵌Python可用或SfM/Feedback实验完成。未重复执行模型测试，无须重新下载权重或更改包版本。

### 2.6 S11-B正式工程配置验收（2026-10-14 19:23）

用户应用脚本和正式九工程属性检查均PASS、退出0；助手复核E:\sfm\setup\S11B-settings-check.log及正式JSON。两个.props与准备文件SHA256一致，九份JSON报告已存档到feedback_local_setup\S11B-properties-passed-20261014，检查日志存档为S11B-settings-check-passed-20261014.log。

* Release/x64，v143，VCToolsVersion14.36.32532及对应VCToolsInstallDir。
* SDK目标与实际IncludePath为10.0.26100.0。
* CudaToolkitCustomDir=D:\CUDA\v12.2\；Qt的UI/Workflow根目录为E:\Qt\5.12.12\msvc2017_64。
* vcpkg实际安装目录=E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\；VcpkgManifestInstall=false，使用S08已验收库。
* OutDir=E:\sfm\runtime\Release\；IntDir=E:\sfm\build\obj\项目名\x64\Release\。
* 正式Git差异：9个.vcxproj，25行新增/7行删除；两份新.props未跟踪，尚未提交；C++算法源码未改。
* 备份位于E:\sfm\setup\S11B-original-projects。工具MSBuild实际17.13.19.12613。

B完成表示编译配置已准备，尚未证明编译/链接成功。助手确认SIFTGPU.lib与S11C-SIFTGPU-build.log当前不存在；进入C1用户执行首次SIFTGPU构建。

### 2.7 S11-C模块构建账本

| 编号 | 模块 | 状态 | 构建结果/产物 |
| --- | --- | --- | --- |
| C1 | SIFTGPU | 已通过（构建） | 2026-10-14 19:29:41–19:30:00；18.89秒；36警告、0错误、退出0；SIFTGPU.lib 22768192字节 |
| C2 | VLFeat | 已通过（构建） | 2026-10-14 19:37:29；0.71秒；0警告、0错误、退出0；VLFeat.lib 3322520字节；源码0.9.20 |
| C3 | Base | 已通过（修复后构建） | 19:59:23产物；重试26.02秒；80警告/0错误、退出0；Base.lib 29458872字节；首次失败记录保留 |
| C4 | Geometry | 已通过（构建） | 2026-10-14 20:06:05产物；21.56秒；35警告/0错误、退出0；Geometry.lib 50846562字节 |
| C5 | Scene | 已通过（构建） | 2026-10-14 20:12:08产物；36.93秒；741警告/0错误、退出0；Scene.lib 178690940字节 |
| C6 | Estimator | 已通过（构建） | 2026-10-14 20:22:03产物；54.49秒；108警告/0错误；用户确认上一步正确完成；Estimator.lib 185752884字节 |
| C7 | Feature | 已通过（构建） | 2026-10-14 20:25:28产物；4.07秒；156警告/0错误；用户确认上一步正确完成；Feature.lib 29944734字节 |
| C8 | Workflow | 已通过（构建） | 2026-10-14 20:29:32产物；69.99秒；1433警告/0错误；用户确认上一步正确完成；Workflow.lib 265410444字节 |
| C9 | UI | 已通过（构建与链接） | 2026-10-14 20:35:22产物；180.85秒；549警告/0错误、退出0；UI.exe5773312字节 |

C1助手复核：日志生成成功，SIFTGPU.lib非空，LastWriteTime=2026-10-14 19:30:00；SHA256=9B96DD3D0E0377BFE9F84DEAB0AF3710597C7A23D443C7122E7509DDAA252C6E。实际cl.exe/Lib.exe路径为MSVC14.36.32532\bin\HostX86\x64，nvcc.exe=D:\CUDA\v12.2\bin\nvcc.exe，目标为64位、CRT /MD，源文件ProgramCU.cu及C++文件均已编译并归档。日志原件E:\sfm\setup\S11C-SIFTGPU-build.log，快照feedback_local_setup\S11C-SIFTGPU-build-passed-20261014.log。

36个警告均记录在日志，类别包括[C4267](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4267?view=msvc-170)的size_t到较小整数转换、[C4312](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-1-c4312?view=msvc-170)的整数到更大指针类型转换，以及[C4996](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996?view=msvc-170)的旧CRT/弃用CUDA接口。本轮未压制或修改警告代码；警告未阻止构建成功，源码学习及真实运行阶段仍需按调用位置审查，不能据构建通过认定算法功能已复现。

C2助手复核：VLFeat日志生成成功，VLFeat.lib非空，LastWriteTime=2026-10-14 19:37:29；SHA256=D38B221B0E0EDAD388471295C697C668E5DF8834C250A40CF2894A7D42032597。Lib.exe实际MSVC14.36.32532\bin\HostX86\x64，生成X64静态库。generic.h的VL_VERSION_STRING为0.9.20，与源码提交共同记录。日志原件E:\sfm\setup\S11C-VLFeat-build.log，快照feedback_local_setup\S11C-VLFeat-build-passed-20261014.log。C++算法源码未修改；C3尚未执行。

C3失败与修复准备（2026-10-14 19:54）：用户首次Base日志首错为glog\logging.h(503,38)的google::CustomPrefixCallback弃用声明。该glog0.7.1头文件在自身旧重载声明中使用deprecated类型；正式Base命令有/sdl，依据微软定义将C4996升级为error，因此7个编译单元重复失败。Base代码仅调用一参InitGoogleLogging，当前无需迁移其调用API。初次失败日志快照S11C-Base-build-failed-glog-20261014.log已保存。

修复准备：新增local.glog-compat.h，仅包围第三方glog头文件的解析时禁用C4996，之后pop恢复；更新local.windows.late.props，对7个src工程的ClCompile增加强制包含，保持/sdl及依赖版本。SIFTGPU/VLFeat不加此项，算法C++文件不改。配置部署脚本已同步纳入兼容头，新安装/复制配置会带上它。正式仓库当前仍未应用此修复，Base.lib不存在。

验证：最小MSBuild同配置编译，旧表退出1并重现glog C4996；新表退出0、对象已生成，仍包含/sdl和/FI；额外调用方测试仍将调用方自身C4996报为错误（预期退出2）。测试产物全部在原工作区validation\glog-sdl-probe，未自动编译正式Base。应用/验收步骤见[glog局部修复](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C3-glog兼容修复.md>)；重试日志S11C-Base-retry1-build.log，只有完整Base退出0和非空产物才完成C3。

C3正式重试验收（2026-10-14 20:02）：用户重试80警告/0错误、退出0；助手复核S11C-Base-retry1-build.log和Base.lib，大小29458872字节，时间2026-10-14 19:59:23；SHA256=0468D3FFC3F40DCE4FEF844D151C4324ABA289483147B015B517E1E7CCF73EDE。实际cl仍为MSVC14.36.32532、CRT /MD，命令保留/sdl且包含/FI本地glog兼容头；局部修复已通过完整Base编译。其余warning保留，未全局关闭检查。

日志原件E:\sfm\setup\S11C-Base-retry1-build.log，成功快照feedback_local_setup\S11C-Base-build-passed-20261014.log；首次失败快照保留用于追溯。当前C4尚未执行，不把Base成功认定为整个S11完成。

C4正式验收（2026-10-14 20:07）：用户Geometry生成35警告/0错误、退出0，两个路径检查True；助手复核日志与产物。Geometry.lib为50846562字节，时间2026-10-14 20:06:05；SHA256=23C7086200148225A9CBCEC7CCAC3EB6F552835ED88C22A8B0B72EF6FA5D51EB。成功日志原件E:\sfm\setup\S11C-Geometry-build.log，快照feedback_local_setup\S11C-Geometry-build-passed-20261014.log。C4构建完成，几何算法运行验证仍需后续实际数据流程。

C5正式验收（2026-10-14 20:13）：用户Scene生成741警告/0错误、退出0，两项路径检查True；助手复核日志与产物。Scene.lib为178690940字节，时间2026-10-14 20:12:08；SHA256=A6E630414151CA81A3B7B9CD8D8042BD061B5A2BF560D505BFC00426CB778B64。ray_tetra_cuda.cu实际命令使用D:\CUDA\v12.2\bin\nvcc.exe及MSVC14.36.32532。成功日志原件E:\sfm\setup\S11C-Scene-build.log，快照feedback_local_setup\S11C-Scene-build-passed-20261014.log。

日志保留C4828（源字符集65001中的无效字符）、C4530（未启用异常展开语义）以及C4251/C4244/C4267和CUDA #68-D等警告。此处只记录类别，不按日志行数统计独立警告，因为末尾汇总会重复列出。没有全局压制警告或据此修改算法源码；字符编码与异常配置在源码/运行审查时继续核对。C5通过表示编译及归档成功，网格、路径规划和CUDA运行效果尚未验证。Estimator.lib及其日志当前不存在，进入C6。

C6正式验收（2026-10-14 20:23）：用户确认上一轮步骤正确完成；助手只读复核Estimator构建日志，末尾108警告/0错误、耗时54.49秒，Estimator.lib为185752884字节，时间2026-10-14 20:22:03；SHA256=52747D46F31CDE5FDD5C884D44BAA161A992341A54EEDEB498851FC61DAA4FC9。退出码验收依据用户对上一轮步骤的完成确认，未重新构建以获取新退出码。日志原件E:\sfm\setup\S11C-Estimator-build.log，成功快照feedback_local_setup\S11C-Estimator-build-passed-20261014.log；警告保留。Feature.lib及其日志尚不存在，进入C7。C6通过不代表束调整或增量建图功能已运行验证。

C7正式验收（2026-10-14 20:26）：用户确认上一轮步骤正确完成；助手只读复核Feature构建日志，末尾156警告/0错误、耗时4.07秒，Feature.lib为29944734字节，时间2026-10-14 20:25:28；SHA256=C255A10409D3D28A42FE70DA09900A176236B0A12C017FD41BC2D94B29E2DC46。退出码验收依据用户对上一轮步骤的完成确认，未重新构建以获取新退出码。日志原件E:\sfm\setup\S11C-Feature-build.log，成功快照feedback_local_setup\S11C-Feature-build-passed-20261014.log；原始警告保留。PyLoader.cpp随模块构建成功不代表内嵌Python已运行通过。Workflow.lib及其日志当前不存在，进入C8；该工程包含QtMoc项目和对前七模块的项目引用。

C8正式验收（2026-10-14 20:30）：用户确认上一轮步骤正确完成；助手只读复核Workflow构建日志，末尾1433警告/0错误、耗时69.99秒，Workflow.lib为265410444字节，时间2026-10-14 20:29:32；SHA256=F1AD459B8771D25651E914C919169F666091C99B275599466EBE46644D399A69。退出码验收依据用户对上一轮步骤的完成确认，未重新构建以获取新退出码。日志包含QtMoc目标及moc_Workflow.cpp的编译内容，原始警告保留。日志原件E:\sfm\setup\S11C-Workflow-build.log，成功快照feedback_local_setup\S11C-Workflow-build-passed-20261014.log。Workflow任务实际运行仍需完整程序；UI.exe及其日志当前不存在，进入C9编译与最终链接。

C9正式验收（2026-10-14 20:39）：用户返回549警告/0错误、耗时180.85秒、立即保存退出码0及两项路径True；助手复核UI.exe为5773312字节，时间2026-10-14 20:35:22；SHA256=BB3E4F35310BBDF1AD29993003858EB0D5C9F06B528361ACB789BD4BE6616198。日志原件E:\sfm\setup\S11C-UI-build.log，成功快照feedback_local_setup\S11C-UI-build-passed-20261014.log。八个静态库及UI.exe均已生成，S11构建完成；保留警告，包括Qt静态对象的LNK4099调试信息提示。

S12准备核查：dumpbin读取UI.exe直接依赖，包含cudart64_12.dll、tbb12.dll、python310.dll及Qt/vcpkg库。构建已自动部署多数Qt/vcpkg运行库，Qt5Core.dll与platforms\qwindows.dll文件版本均5.12.12.0；输出目录缺python310.dll和tbb12.dll，其正确来源文件均存在，D:\CUDA\v12.2\bin\cudart64_12.dll存在。源码main.cpp按当前工作目录读取../settings.ini，正式INI已存在。助手仅只读核查并运行windeployqt --dry-run，尚未部署或启动正式UI。当前进入S12-A；详细操作见feedback_local_setup\S12-部署与首次启动.md。

### 2.8 S12-A运行文件部署验收（2026-10-14 20:49）

用户确认上一轮步骤符合通过标准（Qt部署退出0，九项Exists True，复制无报错）；助手复核正式目录与正确来源，不重新部署或启动程序。Qt日志原件E:\sfm\setup\S12A-qt-deploy.log，快照feedback_local_setup\S12A-qt-deploy-passed-20261014.log。

| 输出文件/部分 | 实际版本 | 大小/校验 |
| --- | --- | --- |
| Qt5Core.dll/Qt5Gui.dll/Qt5Widgets.dll/platforms\qwindows.dll | 文件版本均5.12.12.0 | 均存在；qwindows.dll1473200字节 |
| python310.dll | 文件版本3.10.22 | 4929024字节；SHA256=D7067FBF756107D62E4F51CF4964588D95DA5495F97EEAAFCBD5CCC19F7FEBEF；与feedback310根目录源文件一致 |
| tbb12.dll | 文件版本2021.9，对应SDK2021.9.0 | 290944字节；SHA256=0DC5C6904AFBDF1336E2A554A325F09AC4BF144688347354A5F4052CCCC7F1B2；与onetbb\redist\intel64\vc14源文件一致 |
| settings.ini/CUDA运行库 | 保留已验收配置及Toolkit12.2 | runtime\settings.ini与D:\CUDA\v12.2\bin\cudart64_12.dll均存在 |

S12-A文件验收通过。核查时无UI进程，S12B-UI-stderr.log尚不存在；S12-B首次启动与内嵌Python仍待执行，不能把DLL存在认定为模型加载或SfM运行成功。

### 2.9 S12-B首次失败与修复准备（2026-10-14 20:56–21:01）

用户进程23140的HasExited=False；日志显示内嵌Python3.10.22初始化成功，但GlobalFeatureExtractor→NetVlad→torch→ctypes→_ctypes导入链失败，ImportError: DLL load failed while importing _ctypes。末尾Module GlobalFeatureExtractor not found是项目对导入失败的概括提示；脚本本身已被找到并执行，不能据此改PythonWorkspace或下载同名脚本。模型加载成功日志尚未出现，S12-B未通过。

助手dumpbin核查：_ctypes.pyd直接依赖ffi-8.dll；该DLL存在于E:\sfm\envs\feedback310\Library\bin，为31232字节，UI输出目录缺少它。Conda元数据libffi版本3.7.0、build h3d046cb_1；文件名中的8不当作包版本。源DLL SHA256=922FE231D83F412D351E2560714E248D097AAE0B44C8410FB1826411E2528E69。ffi-8.dll自身依赖仅Windows系统库及VCRUNTIME140.dll，输出目录已有后者。

加载对照：在独立PowerShell工具进程中用LoadLibraryExW flags=0x1100读取_ctypes.pyd，仅注册Python根目录时失败/Win32Error126；额外通过AddDllDirectory注册Conda Library\bin后成功/Win32Error0。测试后移除临时目录注册并释放加载句柄，未改变用户UI进程或系统环境。该对照支持DLL搜索路径问题，但不代替完整内嵌模型加载验证。依据[Python3.10 DLL目录说明](https://docs.python.org/3.10/library/os.html#os.add_dll_directory)及[微软LoadLibraryEx说明](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw)，PATH存在不保证Python扩展依赖可解析。

准备方案：关闭当前UI，复制此环境的ffi-8.dll到UI.exe同目录，校验SHA256一致后以原启动环境重试。应用目录包含在LOAD_LIBRARY_SEARCH_DEFAULT_DIRS中。此轮仅补充运行文件，不更改C++/Python算法源码或包版本；助手未执行正式复制、关闭UI或启动重试。首次stderr/stdout快照保存在feedback_local_setup，重试日志采用S12B-retry1-UI-stderr.log及对应stdout，保留首次记录。操作见[S12-B ffi修复](<E:/101研0/on th fly SFM/feedback_local_setup/S12-B-ctypes与ffi运行库修复.md>)。

### 2.10 S12-B重试模型加载成功（2026-10-14 21:11–21:15）

用户提供retry1进程27640、HasExited=False，Python3.10.22及21:11:51.739284的“The global extractor has been successfully loaded!”。助手重新读取日志：三行初始化/版本/成功记录，无后续导入异常。ffi-8.dll已部署，31232字节，SHA256=922FE231D83F412D351E2560714E248D097AAE0B44C8410FB1826411E2528E69，与环境来源一致。_ctypes问题通过补充运行文件解决，没有更改项目算法源码或包版本。

原件E:\sfm\setup\S12B-retry1-UI-stderr.log，成功模型加载快照feedback_local_setup\S12B-UI-retry1-model-loaded-stderr-20261014.log；首次失败日志继续保留。内嵌导入与模型构造验证通过，内嵌照片特征提取和SfM仍需S13。

GUI确认：只读进程查询Responding=True，但未取得用于确认窗口显示的标题/句柄；不据此断言窗口异常，也不标记GUI已验收。已向用户询问主窗口是否可见可操作。数据确认：trial01\images只有00001.jpg，570051字节；trial02尚不存在；已询问现有同场景多角度照片目录或公开样例需求。当前没有启动照片处理。

### 2.11 GUI验收与S13公开数据准备（2026-10-14 21:27）

用户回复主窗口能看到且可正常操作，结合2.10内嵌模型成功日志，S12完整通过。用户没有同场景照片，选择获取公开样例。助手读取[COLMAP作者数据目录](https://demuc.de/colmap/datasets/)确认South Building入口，并通过官方GitHub release元数据核查：3.11.1/south-building.zip为419421847字节，updated_at=2024-12-13T12:45:31Z，digest=null。来源快照feedback_local_setup\S13A-south-building-source.json。

代下载shell操作未获用户批准；当前工作区数据目录没有压缩包，助手未通过其他工具重试下载。提供官方浏览器下载链接，目标E:\sfm\setup\south-building.zip；准备inspect_sfm_archive.py，校验官方大小、全ZIP成员CRC并记录本地SHA256、实际照片数/目录。脚本语法检查通过，实际数据验证尚待用户下载后执行。不预先认定照片数或把本地hash当作作者发布hash。

下一步按[S13-A照片准备说明](<E:/101研0/on th fly SFM/feedback_local_setup/S13-A-照片准备说明.md>)下载与验收，之后根据实际ZIP结构解压并建立trial02。公开样例用于功能试跑，不据此认定论文UAV数据及定量指标复现完成。

### 2.12 S13-A已有South Building源照片验收（2026-10-14 21:35）

用户提供E:\ColmapProject\south-building；助手只读确认包含images、sparse、database.db（221487104字节）。只复用images，保留已有COLMAP数据库和重建结果，正式新实验拟建立E:\sfm\data\trial02。

check_sfm_images.py实际检查源images：128张JPG全部解码、尺寸均3072x2304、128个不同SHA256、总大小233850173字节，Result: PASS、退出0。排序后的文件名/hash清单SHA256=5183A51BCA95F895C68491E1C27BD9D5ED644260B27AF8227F98150B095E8855；逐文件清单及尺寸/hash见feedback_local_setup\S13A-existing-south-building-images.json。该标识用于本机实验追踪，不据此宣称与作者官方发布hash一致。

trial02当前不存在，助手未复制照片或写入该目录；GUI进程27640仍存在。当前给用户创建images副本/result目录及同名同hash验证命令，副本报告拟为E:\sfm\setup\S13A-trial02-images.json。旧下载/ZIP校验步骤转为备用，当前不再下载。验收副本后再指导GUI建项目、Start Receive及SfM证据检查。

### 2.13 S13-A trial02副本验收（2026-10-14 21:42）

用户确认上一轮复制/校验符合通过标准；助手复核E:\sfm\setup\S13A-trial02-images.json，128张图全部解码，128个不同内容，3072x2304，总233850173字节，reference_matches=true。清单SHA256=5183A51BCA95F895C68491E1C27BD9D5ED644260B27AF8227F98150B095E8855，与源目录一致。报告快照feedback_local_setup\S13A-trial02-images-passed-20261014.json。

正式副本目录实际128文件、大小一致，result目录存在且为空，trial02.db不存在；UI进程27640仍在运行，当前日志只有S12模型初始化成功三行。S13-A通过；当前准备按[S13-B首次运行说明](<E:/101研0/on th fly SFM/feedback_local_setup/S13-B-trial02首次SfM运行.md>)执行Project→New Project、设置Database/Images、Save，再Transmit→Start Receive一次。

运行证据待补充：Initialization successful、后续registered successfully、模型相机/点云及已注册Images/Points计数。不得据输入照片128张推断最终注册数或算法结果；尚未点击运行，不标记S13完整通过。

### 2.14 S13-B SfM完成与Feedback计算证据（2026-10-14 21:58）

用户提供同一轮trial02运行日志和主窗口截图。截图底部显示1min 38s、128 Images、31039 Points，Model Viewer中可见South Building稀疏点云及相机轨迹；最后一张P1180347.JPG注册成功后仍完成两次求解和最终渲染。结合128条全局特征日志、1次初始化和126条`registered successfully!`日志，可确认初始化的2张影像加后续126张影像，共128/128张输入进入当前模型。S13完整通过。

日志同时包含8条非空`[Path planning]`结果，对应注册计数达到50、60、70、80、90、100、110、120时的反馈调度。末次为low-quality faces=3000、clusters=20、candidates=928、selected viewpoints=60、path segments=59。这里3000是进入规划的低质量面数量，不能当作整个网格总面数；60/59证明本轮已经算出候选视点与连接路径，但仍需GUI开关的显示对照完成S14。

证据快照：[运行日志](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/trial02/S13-trial02-runtime-20261014.log>)、[S13主窗口截图](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/trial02/S13-sfm-128images-31039points.png>)。E:\sfm\data\trial02\result仍为空；当前尚未Export Project。源码中的`OutputMeshPly`只有定义、没有调用点，因此当前无`runtime\output`目录并不构成S13/S14计算失败，导出另在S15验收。

### 2.15 S14-A网格显示验收（2026-10-14 22:12）

用户勾选Show Mesh后，Model Viewer清楚显示绿色三角面和边，界面仍可正常使用，S14-A通过。证据：[Show Mesh截图](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/trial02/S14A-show-mesh-pass.png>)，SHA256=D1D9F0913132487B378FD0CB6AB72E7317A8029E46A6AFFB8C4D4D01B45BC5CC。

截图中的建筑主体已有连续表面，但顶部和场景边界存在明显跨越空白区域的长三角形，网格几何质量只记为“功能可用、质量较粗”。源码对稀疏SfM点执行Delaunay构建/更新和图割提取表面；当前`FilterScene`只按量化坐标合并重复点与重复射线，没有看到面边长、三角形尺度或场景外延的后处理裁剪。因此这种结果与本轮稀疏点分布及在线网格方法相符，不能把Show Mesh显示成功等同于可交付的高质量稠密网格。当前不改算法参数，先完成原功能链的显示验收。

### 2.16 S14-B质量显示验收（2026-10-14 22:18）

用户同时勾选Show Mesh和Show Quality后，网格面由统一底色切换为明显的蓝、白、红分区着色，绿色网格边仍可见，界面保持正常，S14-B通过。证据：[Show Quality截图](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/trial02/S14B-show-quality-pass.png>)，SHA256=9A00A8A458874FA39DE2FADDE0DC58F2037EC6EFE1E76D1DAC3E334264975BE5。

源码按每个面的质量分数排名映射颜色：低分位由蓝色向白色过渡，高分位由白色向红色过渡；质量分数由平均观测轨迹量和反向归一化重投影误差按0.8/0.2组合。本图证明质量数据已生成并传入渲染；颜色属于本模型内部的相对分位显示，不应直接解释为绝对误差单位或与其他模型横向等值比较。

### 2.17 S14-C引导路径显示验收（2026-10-14 22:24）

用户勾选Show Guide Path后，Model Viewer中清楚出现紫红色规划视点、橙色相邻路径段和绿色观察方向，界面保持正常，S14-C通过。画面与源码的视点/路径/方向颜色定义吻合，也与末次日志60个selected viewpoints、59个path segments相互印证。证据：[Show Guide Path截图](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/trial02/S14C-show-guide-path-pass.png>)，SHA256=2A20F6BB01820B7517616A3E0083270AA3F87AAF5F98E218790F36601AE64424。

至此S14的计算及三项显示全部通过。该路径是规划结果的可视化证据，并不表示已连接或控制真实无人机。当前转入[S15导出与结果检查](<E:/101研0/on th fly SFM/feedback_local_setup/S15-导出与结果检查.md>)；导出前result目录仍为空。

### 2.18 S15导出验收（2026-10-14 22:31）

用户执行Project → Export Project后运行自动检查，所有项目均为OK，Result: PASS、退出码0。trial02.db为103288832字节且SQLite integrity=ok；数据库包含128行images、128行keypoints、128行descriptors、3372行matches和1178行two_view_geometries。文本模型头部记录128张注册影像和31039个三维点，调试文件同样为(128,128)。

导出核心文件均非空：Log.txt 127337字节、match_matrix.bmp 11059254字节；二进制模型cameras/images/points3D分别64/14576944/2999757字节；文本模型分别197/22381484/4357479字节；images_info.txt 45068字节、model_info.txt 4908字节。证据：[S15检查报告](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/trial02/S15-trial02-export-pass.json>)、[模型统计](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/trial02/S15-model_info.txt>)、[逐影像统计](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/trial02/S15-images_info.txt>)。

检查后出现trial02.db-shm（32768字节）和trial02.db-wal（0字节），时间与Python SQLite检查一致；数据库完整性仍为ok。检查器已改用`mode=ro&immutable=1`，以后复查不会再创建此类旁文件。本轮不自动删除已产生文件，S16结果清单会如实记录。S15通过；当前进入[S16环境固化](<E:/101研0/on th fly SFM/feedback_local_setup/S16-A-环境冻结快照.md>)。

### 2.19 S16-A首次冻结检查（2026-10-14 22:38）

用户正常关闭UI后运行冻结脚本，pip freeze、pip check、Conda环境、Git记录、配置副本及32个文件hash均成功，脚本显示PASS、退出0。助手复核发现vcpkg-installed.txt只有2字节且控制台明确提示`No packages are installed`：脚本从runtime\Release调用vcpkg，未指向项目的vcpkg_installed目录，因此这一项是假阳性，S16-A暂不标记通过。

只读对照在E:\sfm\OntheflySfMFeedback目录执行vcpkg list可正常列出已安装依赖。脚本现已显式使用`--x-install-root=E:\sfm\OntheflySfMFeedback\vcpkg_installed`并要求非空条目数大于0；同时将Visual Studio记录拆分为本项目实际使用的VS2022 17.13.35828.75与机器上最新检测到的其他版本，避免原快照中单独记录18.6造成误解。当前只需重跑同一冻结脚本，原快照目录会安全更新。

### 2.20 S16-A修复后冻结验收（2026-10-14 22:45）

用户重跑修复后的脚本，输出vcpkg entries=175、file hashes=32、Result: PASS、退出0。助手复核S16-freeze-report.json：passed=true、vcpkg_entry_count=175、hashed_file_count=32；versions.json分别记录项目使用的VS2022 17.13.35828.75和机器最新检测版本18.6.11806.211，Qt5.12.12、CUDA12.2.140、RTX4060/617.42、Python3.10.22及vcpkg工具版本均完整。pip check仍为No broken requirements found。

Git复核显示local/windows-setup仍指向基线e2f511042edb，工作区只含9个已验证vcxproj修改及local.windows.early.props、local.windows.late.props、local.glog-compat.h三个新文件；`git diff --check`无错误。Git身份Dong Zhu/<已配置的Git提交邮箱>已配置。S16-A通过，证据快照位于[freeze证据目录](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/freeze/S16-freeze-report.json>)；当前按[S16-B Git本地提交](<E:/101研0/on th fly SFM/feedback_local_setup/S16-B-Git本地提交.md>)建立可回退节点。

### 2.21 S16-B本地提交验收（2026-10-14 22:50）

用户精确暂存9个vcxproj和3个本机配置文件；`git diff --cached --check`退出0，暂存统计为12 files changed、115 insertions、7 deletions。LF将在Git以后接触工作副本时转换为CRLF的提示符合Windows换行策略，未形成diff-check错误。

本地提交成功：f3d023b30e34164b840d8534fde05b9fa1988ec7，主题`chore(windows): record verified local build configuration`，分支仍为local/windows-setup，提交者Dong Zhu <<已配置的Git提交邮箱>>，时间2026-10-14T22:48:05+08:00；提交后`git status --short`为空、退出0。助手只读复核与用户输出一致。在该时点，冻结快照仍记录提交前基线e2f5110，因此随后按2.22重跑冻结脚本以捕获f3d023b、干净工作区及HEAD补丁。

### 2.22 S16最终冻结与全流程完成（2026-10-14 22:55）

用户在提交后重跑冻结脚本，所有检查均为`[OK]`，`Result: PASS`且退出码为0。冻结报告记录source_commit=f3d023b30e34164b840d8534fde05b9fa1988ec7、source_branch=local/windows-setup、source_worktree_clean=true、source_status_entry_count=0、vcpkg_entry_count=175、hashed_file_count=32、passed=true；`source-head.patch`存在。

助手只读复核最终仓库HEAD、分支和工作区状态与报告一致，并将最终快照同步到[freeze证据目录](<E:/101研0/on th fly SFM/feedback_local_setup/evidence/freeze/S16-freeze-report.json>)。其中`source-head.patch`为20014字节、`source-head-summary.txt`为772字节、`file-hashes.csv`为4382字节。至此，本机环境、九工程完整构建、内嵌Python/NetVLAD、128张影像增量SfM、Feedback三项显示、结果导出以及Git可回退节点全部通过。

本轮完成的是软件功能复现。论文中的定量精度指标、重复性统计、真实无人机传输与飞控闭环仍属于后续独立实验；本次网格存在长三角形，质量结论维持“功能可用、几何较粗”。源码学习从[S17源码学习路线](<E:/101研0/on th fly SFM/feedback_local_setup/S17-源码学习路线.md>)继续。

## 三、S02 操作存档：建立短路径源码（已完成）

目的：避免中文/空格路径与源码中部分窄字符接口和路径转换产生额外问题；保留原目录供查阅。

打开 PowerShell，先执行：

```powershell
Test-Path -LiteralPath 'E:\sfm\OntheflySfMFeedback'
```

如果是 False，执行下面这段。它只复制现有完整仓库，不重新下载：

```powershell
if (Test-Path -LiteralPath 'E:\sfm\OntheflySfMFeedback') { throw '目标目录已存在，请先检查该目录，不执行覆盖复制。' }
New-Item -ItemType Directory -Force -Path 'E:\sfm'
Copy-Item -LiteralPath '<工作记录目录>\OntheflySfMFeedback' -Destination 'E:\sfm\OntheflySfMFeedback' -Recurse -Force
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
git switch -c local/windows-setup
New-Item -ItemType Directory -Force -Path 'E:\sfm\setup'
Copy-Item -Path '<工作记录目录>\feedback_local_setup\*' -Destination 'E:\sfm\setup'
```

若目标原本存在，不执行覆盖复制，先检查它的 Git 信息；如果分支已经存在，使用 git switch local/windows-setup，不重复 -c。

检验：

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
git rev-parse HEAD
git branch --show-current
git remote -v
git status --short
Test-Path '.\OntheFlySfM.sln'
Test-Path '.\vcpkg.json'
Test-Path '.\scripts\pyscripts\GlobalFeatureExtractor.py'
Test-Path 'E:\sfm\setup\settings.ini.example'
```

通过标准：提交与文首相同；分支 local/windows-setup；origin 指向新项目；git status --short 无输出；四个 Test-Path 均为 True。

验收结果（2026-10-13 21:05）：用户已提供全部输出，助手在目标目录重新执行检验命令。提交、分支、origin符合要求；工作区干净；四个文件存在检查True。S02已完成，无须重复复制或重复创建分支。

## 四、后续逐步执行与检验

以下提供各步骤的操作和检查方式，用于随时查阅；S00–S13已通过。当前S14的Feedback计算已有日志证据，正在逐项验收GUI中的网格、质量和引导路径显示。

### S03：VS 组件与 MSVC（已完成，操作存档）

安装前检测：C++桌面开发存在；Python开发工作负载与MSVC14.36组件未检测到；原MSVC目录为14.42.34433、14.43.34808。以下保留当时的操作供查阅，无须重复安装。

操作：

1. 保存工作并关闭VS，通过Windows开始菜单打开Visual Studio Installer；找到Visual Studio Community 2022并点击“修改”。若提示更新安装器，可按提示完成安装器更新；实际IDE版本安装后重新记录。
2. “工作负载”中保留已勾选的“使用C++的桌面开发”，勾选“Python开发”。
3. 在Python开发的安装详细信息或“单个组件”中确认“Python语言支持”，勾选“Python本机开发工具 / Python native development tools”（用于后续混合调试）。暂不额外安装Python解释器，项目Python3.10环境将在S06通过已有Anaconda创建。
4. “单个组件”搜索14.36，选择“MSVC v143 - VS 2022 C++ x64/x86生成工具（v14.36-17.6）”。名称可能带“已停止支持 / Out of Support”；本次选择它是为了匹配较早CUDA的编译器范围。选择x64/x86基础生成工具，保留现有其他工具集。
5. 本机已有Windows SDK10.0.22621.0、10.0.26100.0，保持至少一套已安装SDK；现在不重复下载SDK。
6. 点击“修改”，等待安装完成。安装结束后如要求重启，先重启再检验。

若搜索14.36没有结果，先清除搜索之外的过滤条件，记录完整界面/搜索结果；不要擅自改用“最新MSVC”作为此参考路线的14.36替代。

官方依据：[修改VS组件](https://learn.microsoft.com/en-us/visualstudio/install/modify-visual-studio?view=vs-2022)、[Python支持](https://learn.microsoft.com/en-us/visualstudio/python/installing-python-support-in-visual-studio?view=vs-2022)、[组件目录中的14.36](https://learn.microsoft.com/en-us/visualstudio/install/workload-component-id-vs-community?view=vs-2022)。

检验：

```powershell
foreach ($taskComponent in @(
    'Microsoft.VisualStudio.Workload.NativeDesktop',
    'Microsoft.VisualStudio.Workload.Python',
    'Microsoft.Component.PythonTools',
    'Microsoft.ComponentGroup.PythonTools.NativeDevelopment',
    'Microsoft.VisualStudio.Component.VC.14.36.17.6.x86.x64'
)) {
    $taskMatch = & 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -latest -products '*' -requires $taskComponent -property installationPath
    [pscustomobject]@{Component=$taskComponent;Installed=[bool]$taskMatch}
}
Get-ChildItem 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC' | Select-Object Name
& 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -latest -property installationVersion
```

通过标准：组件表五项Installed均为True；MSVC目录包含14.36.*；记录VS安装后的完整版本和14.36完整目录名。若只部分成功，按实际证据分别记录。项目里真正选中14.36在S11再检查，目录存在不代表编译时一定使用它。

验收结果（2026-10-13 21:22）：用户提供五项组件全部True的输出，助手只读复核通过；MSVC目录新增14.36.32532，原两个版本保留；VS安装版本为17.13.35828.75。S03完成，尚不表示项目已编译或已选用此工具集。

### S04：Qt 5.12.12 与 VS 接入（已完成，操作存档）

用途：Qt提供项目的图形界面库；Qt VS Tools把Qt构建工具接入Visual Studio。二者分别安装和验收。源码指定的Qt套件名为5.12.12_msvc2017_64。

1. 从[Qt官方5.12.12历史目录](https://download.qt.io/archive/qt/5.12/5.12.12/)下载qt-opensource-windows-x86-5.12.12.exe（目录标注约3.7G）。[直接下载](https://download.qt.io/archive/qt/5.12/5.12.12/qt-opensource-windows-x86-5.12.12.exe)。文件名中的x86是安装器标识；本项目实际选择64位MSVC套件。
2. 运行安装器，建议安装根目录E:\Qt。到组件选择页展开Qt → Qt5.12.12，勾选MSVC2017 64-bit。Qt Creator是另一套IDE，可选；此项目使用VS，不必为本步骤安装MinGW、Android、iOS等其他套件。如需要Qt账户登录，按安装器提示由用户完成。
3. 完成安装，执行下方PowerShell检查。若安装到其他根目录，将命令路径替换为实际路径；不要仅按目录名判断版本。
4. VS中打开“扩展 → 管理扩展”，搜索Qt Visual Studio Tools，选择Qt Group/The Qt Company发布、适用于VS2022的扩展。已安装时仅记录版本并确认启用；未安装时下载，关闭VS完成VSIX安装，再重新打开。[官方安装说明](https://doc.qt.io/qtvstools/qtvstools-how-to-install.html)、[VS2022扩展页面](https://marketplace.visualstudio.com/items?itemName=TheQtCompany.QtVisualStudioTools2022)。
5. 打开“扩展 → Qt VS Tools → Qt Versions”，添加此Qt。名称填5.12.12_msvc2017_64，Host选Windows。当前官方文档的Location填写E:\Qt\5.12.12\msvc2017_64\bin\qmake.exe；部分旧版界面要求套件根目录时，填写E:\Qt\5.12.12\msvc2017_64。保存后应识别为MSVC套件而非MinGW。[官方添加版本说明](https://doc.qt.io/qtvstools/qtvstools-how-to-add-qt-versions.html)。
6. 在“管理扩展 → 已安装”记录Qt VS Tools完整版本；在Qt Versions确认上述名称、版本与位置。QtMsBuild实际路径和qt.targets/Qt.props会在S11编译前继续核查。

下载补充（2026-10-13）：用户反馈VPN下下载预计一天。助手实测此安装包在清华、中科大、阿里云、南京大学、腾讯和上海交大镜像的上述archive路径均返回404；沈阳航空航天大学连接未成功；华为测试地址返回HTML页面，未确认为安装包。中科大帮助说明不再同步已停止官方维护的Qt版本。未找到已验证可用的国内镜像，不能把通用Qt镜像帮助当作此旧安装包仍存在的证据。

已确认可用的[Qt官方主站直链](https://master.qt.io/archive/qt/5.12/5.12.12/qt-opensource-windows-x86-5.12.12.exe)与download.qt.io均返回200、文件大小3987337112字节，文件类型为Windows程序。可先关闭VPN试用主站直链，下载速度待用户实际测量。下载完成后依照[Qt官方文件信息](https://download.qt.io/archive/qt/5.12/5.12.12/qt-opensource-windows-x86-5.12.12.exe.mirrorlist)核对SHA256，应为27955827c9129e58c9147b201eee33f92d2b8360d9de58643800eb4ff1163f5a。下载检查不代表S04安装验收通过。

检验：

```powershell
& 'E:\Qt\5.12.12\msvc2017_64\bin\qmake.exe' -v
& 'E:\Qt\5.12.12\msvc2017_64\bin\qmake.exe' -query QT_INSTALL_PREFIX
& 'E:\Qt\5.12.12\msvc2017_64\bin\qmake.exe' -query QMAKE_XSPEC
Test-Path 'E:\Qt\5.12.12\msvc2017_64\include\QtWidgets'
Test-Path 'E:\Qt\5.12.12\msvc2017_64\lib\Qt5Widgets.lib'
Test-Path 'E:\Qt\5.12.12\msvc2017_64\bin\windeployqt.exe'
```

通过标准：qmake报告Using Qt version 5.12.12；QT_INSTALL_PREFIX为实际的msvc2017_64目录；QMAKE_XSPEC为win32-msvc；三个文件/目录检查均True。扩展已安装并启用；VS的Qt Versions里能看到名称5.12.12_msvc2017_64和正确位置。返回这些命令输出、扩展版本及注册结果。仅qmake通过但VS尚未注册时，S04仍保留待完成。项目实际Qt/MSBuild编译在S11验收。

验收结果（2026-10-13 22:28）：用户提交qmake版本、前缀、平台及三个路径检查输出，助手重新执行全部通过；Qt版本5.12.12，QMake3.1，前缀E:\Qt\5.12.12\msvc2017_64，平台win32-msvc。用户报告Qt VS Tools3.5.0；截图中的注册名5.12.12_msvc2017_64、套件目录、Windows、msvc及默认星标正确。若仍停留在截图对话框，点击“确定”保存。S04安装与注册检查完成，Qt/MSBuild项目构建仍待S11。

### S05：CUDA Toolkit 12.2 Update2（已完成，操作存档）

安装前检测：默认CUDA目录仅有v12.4，VS的BuildCustomizations里未发现CUDA12.2文件。以下操作保留供查阅，实际安装位置为D:\CUDA\v12.2，无须重复安装。

1. 打开[NVIDIA CUDA12.2 Update2下载页](https://developer.nvidia.com/cuda-12-2-2-download-archive)，依次选Windows、x86_64、本机Windows10或11版本、exe (local)。完整安装包名应为cuda_12.2.2_537.13_windows.exe。使用exe(local)方便一次下载及核对文件完整性。
2. 安装前核对文件MD5；将下方命令的下载路径改为实际位置。官方期望值2bd993797152533039048d43b8977ea5，来源[NVIDIA校验清单](https://developer.download.nvidia.com/compute/cuda/12.2.2/docs/sidebar/md5sum.txt)。
3. 关闭VS，运行安装器。初始解压目录只是临时位置；可设在D/E盘临时目录。在后续“安装位置/Installation Location”页面设置正式CUDA Toolkit目录，可选E:\CUDA\v12.2或D:\CUDA\v12.2，也可使用默认C盘位置。前文C盘位置为默认示例，不是必须要求。使用明确版本目录，安装后不要直接剪切搬动文件夹。
4. 选择“自定义/高级”，保留CUDA开发和运行组件，展开CUDA确认Development、Runtime及Visual Studio Integration选中。Nsight分析器可不选；Visual Studio Integration是必选，不能与可选的Nsight Visual Studio Edition混淆。
5. 取消Driver components中的Display Driver等驱动组件，继续使用现有617.42驱动。12.2 Update2的安装包包含较旧537.13驱动；NVIDIA说明新驱动向后兼容，且Windows交互安装可以跳过驱动。保留现有CUDA12.4，不卸载。
6. 等待安装结束，记录各组件安装结果；如提示重启，重启后打开新的PowerShell执行检查。S11再显式指定工程的Toolkit12.2和MSVC14.36；本步骤按绝对路径检查，不依赖PATH中nvcc的先后顺序。

组件选择检查（用户安装界面截图）：Runtime、Development、Visual Studio Integration已勾选；Documentation、Nsight VSE/Systems/Compute、GeForce Experience、Other components及Driver components未勾选。可见选择符合当前安装方案，CUDA父项显示部分选择是正常状态。截图阶段尚未完成安装；后续23:21安装验收已通过。

官方依据：[Windows安装说明](https://docs.nvidia.com/cuda/archive/12.2.2/cuda-installation-guide-microsoft-windows/index.html)、[12.2 Update2组件与驱动说明](https://docs.nvidia.com/cuda/archive/12.2.2/cuda-toolkit-release-notes/index.html)。下载版本12.2.2对应的nvcc组件版本是12.2.140，二者不要求尾号一致。

下载校验（示例路径，按实际替换）：

```powershell
Get-FileHash -LiteralPath 'C:\Users\<用户名>\Downloads\cuda_12.2.2_537.13_windows.exe' -Algorithm MD5
```

路径说明：Toolkit的编译器、头文件、库可位于D/E盘；Visual Studio Integration的CUDA12.2.props/targets仍放入本机VS目录。VS安装在C盘，所以集成检查中的C盘路径保持不变。安装器及系统仍可能在C盘写入临时文件或共享组件，不能据选择E盘认为C盘完全不占用。S11工程中的CUDA Toolkit Custom Dir及依赖构建配置将统一为实际安装路径，不以默认C盘位置写死。

检验（已按实际安装位置D:\CUDA\v12.2更新；其他安装位置只需修改第一行）：

```powershell
$taskCudaRoot = 'D:\CUDA\v12.2'
& "$taskCudaRoot\bin\nvcc.exe" --version
Test-Path "$taskCudaRoot\include\cuda_runtime.h"
Test-Path "$taskCudaRoot\lib\x64\cudart.lib"
Test-Path "$taskCudaRoot\lib\x64\cublas.lib"
Get-ChildItem 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Microsoft\VC\v170\BuildCustomizations' -Filter 'CUDA 12.2.*'
Test-Path 'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.4\bin\nvcc.exe'
nvidia-smi --query-gpu=name,driver_version --format=csv,noheader
```

通过标准：指定实际目录的nvcc报告release12.2、V12.2.140（若不同，先记录实际版本）；三个开发文件检查为True；构建定制目录至少包含CUDA12.2.props和CUDA12.2.targets；原12.4的nvcc存在检查为True；GPU仍为RTX4060Laptop，驱动保持617.42。将全部输出、实际Toolkit目录及安装结果返回。此阶段验收安装文件和VS接入存在；GPU计算、实际编译与工程使用版本分别在S06/S11/S13验证，不能仅据nvcc认定GPU功能通过。nvidia-smi的CUDA Version不作为Toolkit安装版本证明。

验收结果（2026-10-13 23:21）：用户返回完整安装检查输出，助手重新执行全部通过；实际Toolkit为D:\CUDA\v12.2，nvcc12.2.140；三个开发文件True，VS集成props/targets/xml存在；原12.4的nvcc存在；RTX4060Laptop与617.42驱动保留。系统机器级CUDA_PATH已指向D:\CUDA\V12.2，助手既有进程仍保留旧12.4值，需新开终端/重新启动VS后刷新读取。S05完成，尚未验证CUDA实际编译或GPU计算。

### S06：独立 Python 3.10 环境（已完成，操作存档）

目的：工程链接python310.lib，因此创建专用64位Python3.10环境；保留base和已有torch_gpu环境。作者README给出torch2.2.2/torchvision0.17.2/torchaudio2.2.2及cu118组合，其余六个包采用辅助清单中的起步版本，模型加载后再固化实际组合。

1. Windows开始菜单打开“Anaconda PowerShell Prompt”，本步骤所有命令在同一个窗口执行。先检查conda命令与配置清单：

```powershell
conda --version
Test-Path 'E:\sfm\setup\requirements-bootstrap.txt'
Test-Path 'E:\sfm\envs\feedback310'
```

创建前只读核查：conda24.11.3，清单可读取，目标环境目录False。2026-10-14核查已创建feedback310环境，不重复执行创建命令。后续打开新窗口时只需激活已有环境。若找不到conda，先返回错误，不在普通PowerShell里直接尝试环境创建。

2. 创建并激活环境，检查正在使用的解释器：

```powershell
conda create --prefix 'E:\sfm\envs\feedback310' python=3.10 pip -y
conda activate 'E:\sfm\envs\feedback310'
python -c "import sys,struct; print(sys.executable); print(sys.version); print(struct.calcsize('P')*8)"
```

通过标准：创建成功；sys.executable为E:\sfm\envs\feedback310\python.exe，版本3.10.x，位数64。激活后的提示符可能显示完整环境路径，属于正常现象。

3. 在已确认解释器正确的同一个窗口安装包，每条命令成功后再执行下一条：

```powershell
python -m pip install --upgrade pip
python -m pip install -r 'E:\sfm\setup\requirements-bootstrap.txt'
python -m pip install torch==2.2.2 torchvision==0.17.2 torchaudio==2.2.2 --index-url https://download.pytorch.org/whl/cu118
```

清单包括numpy1.26.4、scipy1.12.0、scikit-learn1.4.2、h5py3.10.0、Pillow10.2.0、joblib1.3.2。cu118表示PyTorch包使用的CUDA运行时版本；之前安装的Toolkit12.2用于C++编译，二者分开记录，不因为cu118再安装Toolkit11.8。若下载/安装失败，返回完整错误及失败包，暂不升级到其他torch/NumPy组合。

4. 验收包依赖、GPU计算以及C++所需Python开发文件：

```powershell
python -c "import sys,struct; print(sys.executable); print(sys.version); print(struct.calcsize('P')*8)"
python -m pip check
python -c "import torch,torchvision,numpy; print(torch.__version__,torchvision.__version__,numpy.__version__); print('CUDA runtime:',torch.version.cuda); print('CUDA available:',torch.cuda.is_available()); print(torch.cuda.get_device_name(0) if torch.cuda.is_available() else 'NO CUDA')"
python -c "import torch; x=torch.ones((8,8),device='cuda'); print((x@x).sum().item())"
python -c "import numpy; print(numpy.get_include())"
Test-Path 'E:\sfm\envs\feedback310\include\Python.h'
Test-Path 'E:\sfm\envs\feedback310\libs\python310.lib'
Test-Path 'E:\sfm\envs\feedback310\python310.dll'
python -m pip list
```

通过标准：专用路径、Python3.10/64位；pip check显示No broken requirements found；torch为2.2.2+cu118，torchvision0.17.2+cu118，NumPy1.26.4；torch.version.cuda为11.8，CUDA available为True；GPU为RTX4060Laptop；运算512.0；三个Python开发文件True。返回本节检查输出及pip list，记录Python完整小版本、实际包版本与NumPy头文件路径。若CUDA available为False，先停在该项，不运行依赖GPU的矩阵命令。

官方依据：[conda按路径创建和激活环境](https://docs.conda.io/projects/conda/en/stable/user-guide/tasks/manage-environments.html#specifying-a-location-for-an-environment)、[PyTorch2.2.2官方安装组合](https://pytorch.org/get-started/previous-versions/#v222)。

#### S06下载故障及备用方案存档（2026-10-14，后续重试已解决）

01:02验收：用户重新执行原pip install命令成功安装全部包，并通过pip check、版本、GPU运算、NumPy头文件路径及三个Python开发文件检查；助手重复验收全部通过。以下保留此前备用下载方案，仅供未来需要时查阅；本次无须重新下载或重复安装，不据此声称用户已执行备用方案或手动SHA256检查。

用户日志：torch的2704.2MB wheel下载多次中断，五次续传后约到1.2GB，最终在pip-unpack临时wheel文件访问时WinError32。日志证明连接中断与文件占用，不能仅凭此确定占用来自哪个进程；不据此关闭杀毒软件或改变安装范围。--user会改变安装位置，不作为本项目环境的修复办法。

助手核查：Python3.10.22/64位、pip26.2.1、六个基础包版本正确；pip check通过；三个torch包尚未安装。失败日志所列临时wheel在核查时未发现，不能承诺沿用那1.2GB临时下载。原环境不重建，基础包不重复安装。

方案：将torch wheel单独下载到持久目录E:\sfm\downloads，再由项目解释器从本地文件安装。下载源来自[PyTorch官方cu118索引](https://download.pytorch.org/whl/cu118/torch/)，确切链接为https://download-r2.pytorch.org/whl/cu118/torch-2.2.2%2Bcu118-cp310-cp310-win_amd64.whl。已读取官方索引的sha256片段，并确认响应200、大小2704239630字节、Accept-Ranges:bytes。期望SHA256为593bea28e420118f60055787d4916209aa1f07371f3cbaf56c5b932f3a3d7335。

在Anaconda PowerShell Prompt逐段执行：

```powershell
conda activate 'E:\sfm\envs\feedback310'
python -c "import sys; print(sys.executable)"
New-Item -ItemType Directory -Force -Path 'E:\sfm\downloads'
$taskTorchUrl = 'https://download-r2.pytorch.org/whl/cu118/torch-2.2.2%2Bcu118-cp310-cp310-win_amd64.whl'
$taskTorchWheel = 'E:\sfm\downloads\torch-2.2.2+cu118-cp310-cp310-win_amd64.whl'
curl.exe --location --fail --continue-at - --connect-timeout 30 --speed-time 120 --speed-limit 1024 --output "$taskTorchWheel" "$taskTorchUrl"
$LASTEXITCODE
```

sys.executable必须是项目环境。curl返回0代表本轮下载成功；中断后保留文件，重复同一条curl命令，--continue-at -会从现有文件长度续传，不同时启动多个任务写同一个文件。若重开窗口，先重新设置两个变量。此方案不保证提高带宽，但保留已下载内容便于后续续传。[curl官方续传说明](https://curl.se/docs/manpage.html#-C)。

下载完成再校验：

```powershell
Get-Item -LiteralPath $taskTorchWheel | Select-Object Name,Length
$taskTorchActualHash = (Get-FileHash -LiteralPath $taskTorchWheel -Algorithm SHA256).Hash
$taskTorchActualHash
$taskTorchActualHash -eq '593bea28e420118f60055787d4916209aa1f07371f3cbaf56c5b932f3a3d7335'
```

通过标准：文件大小2704239630字节，SHA256比较为True。未下载完整/校验不符时不执行安装。

```powershell
python -m pip install "$taskTorchWheel" torchvision==0.17.2 torchaudio==2.2.2 --index-url https://download.pytorch.org/whl/cu118 --timeout 120 --retries 10 --resume-retries 30
```

该命令从本地wheel安装torch，其余两个包及所需依赖由pip下载；较小下载增加超时与续传次数。安装成功后回到本节第4项检查pip check、版本和GPU计算。此方案制定时S06仅部分通过；后续01:02原命令重试及GPU检查成功，S06已完成。

### S07：模型资源（文件验收已完成，操作存档）

源码明确加载四个文件，统一放入E:\sfm\models\netvlad：

| 文件 | 用途 | 获取状态 |
| --- | --- | --- |
| vgg16-397923af.pth | VGG16基础权重 | 基础项目作者Drive有同名文件；也有torchvision官方来源 |
| VGG16_64_desc_cen.hdf5 | NetVLAD初始化的centroids与descriptors | 基础项目作者Drive有同名文件，待下载 |
| VGG16_NetVlad_NoSplit.pth.tar | NetVLAD训练checkpoint | 基础项目作者Drive有同名文件，待下载；需要epoch、best_score、state_dict等键 |
| PCA_dims32768to2048.model | 将32768维描述子降到2048维的PCA | 基础项目作者Drive有同名文件，待下载；序列化环境版本未知 |

初查新仓库README、GlobalFeatureExtractor.py、项目主页和可读取的Issues页面，未发现后三个文件的下载说明。用户已明确回答没有模型文件，需要获取。随后追溯基础项目作者README，找到了其公开的[预训练权重目录](https://drive.google.com/drive/folders/1L9QuDZ8azWhNjukgIJEwczfXLeg68Gd0?usp=drive_link)。助手通过HTTP读取公开目录返回200，确认包含以下四个文件及其大小。该来源由[sygant/onthefly作者README](https://github.com/sygant/onthefly#build-from-source)发布；这里仅获取候选模型资源，不安装旧工程或其Python环境。

| 文件 | Drive元数据大小（字节） | 单文件预览链接 |
| --- | --- | --- |
| vgg16-397923af.pth | 553433881 | https://drive.google.com/file/d/19eEcSmBXpgecE6x0i49QU7GZ07_EpCgH/view |
| VGG16_64_desc_cen.hdf5 | 102533120 | https://drive.google.com/file/d/1gQzdogrFMLFx6qVIzeCamFq-LjXtYn6X/view |
| VGG16_NetVlad_NoSplit.pth.tar | 118260221 | https://drive.google.com/file/d/1XhM24RAO6I_eAmOHMRRooltfe2giHY31/view |
| PCA_dims32768to2048.model | 268608477 | https://drive.google.com/file/d/1-UN3W_d38YILgzb1HwxQ0oLPAXQxsCN4/view |

总大小1042835699字节（约1.04GB）。已对比基础项目GlobalFeatureExtractor与新项目源码，四文件加载结构一致，故这组文件是有依据的候选资源；尚未下载、核对hash或进行新环境实际加载，不把同名文件直接标为新项目功能复现通过。

当前优先操作：打开上述Drive文件夹，选择四个文件下载；若网页打包成ZIP，解压后取出上述四个模型文件，直接放入E:\sfm\models\netvlad。不要把VGG16_NetVlad_NoSplit.pth.tar继续解压，新代码直接将它交给torch.load。保持文件名完全一致，避免额外嵌套目录。创建目录：

```powershell
New-Item -ItemType Directory -Force -Path 'E:\sfm\models\netvlad'
```

浏览器能读取目录不等于已确认完整下载/配额；实际下载以用户结果为准。若目录打不开、需要权限或下载失败，返回页面文字/错误，使用备选询问草稿向作者获取资源。下面VGG16官方单文件下载保留为备选，Drive已下载该文件时无须重复执行。

可以先获取有明确官方来源的VGG16文件：

```powershell
New-Item -ItemType Directory -Force -Path 'E:\sfm\models\netvlad'
curl.exe --location --fail --continue-at - --connect-timeout 30 --output 'E:\sfm\models\netvlad\vgg16-397923af.pth' 'https://download.pytorch.org/models/vgg16-397923af.pth'
$LASTEXITCODE
```

下载返回0后检查，若中断则保留文件、再次执行同一curl命令续传：

```powershell
Get-Item -LiteralPath 'E:\sfm\models\netvlad\vgg16-397923af.pth' | Select-Object Name,Length
$taskVggHash = (Get-FileHash -LiteralPath 'E:\sfm\models\netvlad\vgg16-397923af.pth' -Algorithm SHA256).Hash
$taskVggHash
$taskVggHash.ToLowerInvariant().StartsWith('397923af')
```

VGG文件应非空，SHA256前缀检查True。官方提供的文件名使用hash前缀，此处不虚构完整官方SHA256；用户返回实际完整hash后记录。文件正确加载和整体模型输出仍在S10验证。[torchvision0.17.2官方VGG16定义](https://github.com/pytorch/vision/blob/v0.17.2/torchvision/models/vgg.py)。

如果Drive资源不可获取或S10不兼容，可使用已准备的[模型资源询问草稿](<E:/101研0/on th fly SFM/feedback_local_setup/模型资源询问草稿.md>)在新项目Issues自行询问；助手未发送。询问中包含固定源码提交、文件名及PCA的scikit-learn/joblib版本需求。

四个文件获取齐全后检验：

```powershell
Get-Item 'E:\sfm\models\netvlad\vgg16-397923af.pth','E:\sfm\models\netvlad\VGG16_64_desc_cen.hdf5','E:\sfm\models\netvlad\VGG16_NetVlad_NoSplit.pth.tar','E:\sfm\models\netvlad\PCA_dims32768to2048.model' | Select-Object Name,Length
Get-FileHash -Path 'E:\sfm\models\netvlad\*' -Algorithm SHA256
```

通过标准：四个文件存在且非空。记录下载来源、获取日期、文件大小、SHA256以及作者提供的模型/训练环境版本。这里仅验收文件，S10还须真实加载。

验收结果（2026-10-14 10:55）：用户已下载四个文件到指定目录并返回完整大小/hash，助手逐文件重新计算与用户一致；大小均符合Drive元数据；VGG16符合官方397923af hash前缀。未获得其余三个文件的独立官方完整hash，因此本表是本机取得文件的基准记录，不声称已与作者公布的完整hash对比。S07文件验收完成，整体加载仍待S10。

| 文件 | 实际大小（字节） | 本机SHA256 |
| --- | --- | --- |
| vgg16-397923af.pth | 553433881 | 397923AF8E79CDBB6A7127F12361ACD7A2F83E06B05044DDF496E83DE57A5BF0 |
| VGG16_64_desc_cen.hdf5 | 102533120 | 11E8DF48CCF842722BD53527EA5B8219110A8C7929E817D4027924582149E311 |
| VGG16_NetVlad_NoSplit.pth.tar | 118260221 | AA2B60CDB969849BC39A09B6DAE080C099849C01BC3A2FB852263F175C4736E1 |
| PCA_dims32768to2048.model | 268608477 | 63A94B8914FBFD0C44F8DB07C8256F00F3446F3B13FE3CA98D8D8C0BA95E599A |

### S08：vcpkg（A/B已完成，操作及故障处理存档）

用途：vcpkg根据项目vcpkg.json下载和构建C++第三方库。安装Python包不包含这些C++库。项目manifest基线保持b1b19307e2d2ec1eefbdb7ea069de7d4bcd31f01；工具仓库也固定在这一提交。基线工具元数据指定vcpkg发布tag2025-07-21、Windows CMake3.30.1；均以实际安装后输出进一步记录。

#### S08网络故障与修复检查（2026-10-14）

用户多次git clone出现curl56 Connection was reset，随后Failed to connect to github.com port443；均发生在远程连接阶段。用户确认Clash Verge已开启、关闭后同样失败。助手读取有效GitHub代理配置为空，当前进程HTTP_PROXY/HTTPS_PROXY/ALL_PROXY未设置，Windows系统代理开关为0；github.com能解析，但不指定代理的HTTP/1.1 ls-remote在正常网络权限下仍Connection was reset。助手沙箱曾出现SEC_E_NO_CREDENTIALS，正常权限重试后改为连接重置，因此不将沙箱TLS错误认定为用户Git凭据问题。

Clash Verge进程存在，127.0.0.1:7897端口监听，7890未监听。通过以下单次代理命令已成功读取远程HEAD（退出码0），说明7897是此机当前可用的代理入口：

```powershell
git -c http.proxy=http://127.0.0.1:7897 -c http.version=HTTP/1.1 ls-remote https://github.com/microsoft/vcpkg.git HEAD
```

该测试只证明远程访问成功，不是已克隆仓库；测试读取的当前远程HEAD不替代本项目固定基线。E:\sfm\vcpkg及其.git在本轮只读检查时均不存在，无须删除目录。

保持Clash开启，在执行S08-A的同一个PowerShell窗口先设代理环境变量，再验证并克隆：

```powershell
$env:HTTP_PROXY = 'http://127.0.0.1:7897'
$env:HTTPS_PROXY = 'http://127.0.0.1:7897'
git -c http.version=HTTP/1.1 ls-remote https://github.com/microsoft/vcpkg.git HEAD
```

预期输出一行40位提交hash和HEAD，且无连接错误。通过后：

```powershell
Test-Path 'E:\sfm\vcpkg'
git clone -c http.proxy=http://127.0.0.1:7897 -c http.version=HTTP/1.1 --filter=blob:none --no-checkout https://github.com/microsoft/vcpkg.git 'E:\sfm\vcpkg'
```

只在Test-Path为False时克隆，若已存在则先核查目录，不覆盖或删除。HTTP_PROXY/HTTPS_PROXY只影响当前窗口及其子进程；上方测试中的git -c只影响该条命令。13:44之后推荐的新克隆命令把-c放在clone之后，按Git定义会把代理和HTTP/1.1写入新vcpkg仓库的本地配置，便于后续按需取文件；不写全局配置，也不影响SfM仓库。Clash必须保持开启，端口变化后该本地配置需相应更新。助手未替用户执行clone或安装。[Git官方代理说明](https://git-scm.com/docs/git-config#Documentation/git-config.txt-httpproxy)、[clone本地配置与partial clone说明](https://git-scm.com/docs/git-clone/2.52.0)。

13:44故障进展：用户再次用普通git clone获取345242个对象时出现curl56/Connection was reset/early EOF/invalid index-pack output，属于传输中断、未完成克隆。助手检查E:\sfm\vcpkg及.git仍False，没有残留目录需要删除；未发现git传输进程；Clash7897端口仍监听。不能只凭普通git clone命令文字断言用户窗口的代理变量是否已设置，故改用显式代理并保留在新工具仓库本地配置的方式。

后续实际结果（13:58–14:07）：用户报告关闭代理后使用原git clone命令成功获取345242个对象，112.37MiB，完成接收、解析和检出；随后基线checkout、bootstrap、integrate和env检查成功。助手确认已是非浅克隆、基线正确且Git工作区干净。说明直连可用性随时间/网络状态变化，此前失败与单次代理测试不能推出必须始终使用代理。保留此已完成完整克隆，下方代理/partial clone方案归档为网络故障备用，不再要求重克隆或添加代理配置。

后续连接复核：助手再次使用http.proxy=http://127.0.0.1:7897与HTTP/1.1执行ls-remote，正常网络权限下退出码0，返回e456309491fd875487bf02b8a0dca76c1b1b00cc及HEAD。此次仍未执行clone；工具下载与基线检出等待用户操作。

降低单次传输量：--filter=blob:none保留提交历史和目录树，文件内容由Git在需要时补取；--no-checkout先不检出当前远程HEAD，随后直接checkout作者基线。未使用--depth，不截断所需历史。完成检出后仍需按S08-B实际安装验证vcpkg版本端口获取；下载过程中继续保持代理可用。该方案有Git官方支持，但不是保证网络不会再次中断。[GitHub官方partial clone解释](https://github.blog/open-source/git/get-up-to-speed-with-partial-clone-and-shallow-clone/)。

#### S08-A：准备工具与编译配置（已完成，操作存档）

打开新的普通PowerShell，逐条执行，每条成功后再执行下一条。本轮只准备工具和检查选择，不编译全部依赖。

```powershell
Test-Path 'E:\sfm\vcpkg'
```

创建前核查为False，网络故障时曾建议以下按需文件获取方式。用户后续实际用普通完整克隆成功；以下仅保留方案，不重复执行。

```powershell
git clone -c http.proxy=http://127.0.0.1:7897 -c http.version=HTTP/1.1 --filter=blob:none --no-checkout https://github.com/microsoft/vcpkg.git 'E:\sfm\vcpkg'
Set-Location -LiteralPath 'E:\sfm\vcpkg'
git checkout b1b19307e2d2ec1eefbdb7ea069de7d4bcd31f01
.\bootstrap-vcpkg.bat
.\vcpkg.exe integrate install
```

checkout后显示detached HEAD是固定工具版本的正常结果，不能把这条说明当成克隆失败。bootstrap负责获取vcpkg.exe；integrate install负责让VS识别本实例，二者都不等于C++依赖已经装好。出现下载失败时保留目录及第一条错误，暂不改用其他vcpkg提交。

clone成功但尚未checkout时，目录暂时只有.git是--no-checkout的正常结果。先完成基线checkout再运行bootstrap；基线检出时可能继续按需下载文件。可先检验git -C E:\sfm\vcpkg rev-parse --is-shallow-repository为false，checkout后HEAD为固定基线、bootstrap-vcpkg.bat存在，再继续初始化工具。

助手已更新原工作区triplet模板，指定MSVC14.36.32532及传给CMake的目标系统版本10.0.22621.0；该目标变量不保证覆盖vcvars选择的SDK环境。E:\sfm\setup中的旧副本尚未更新，以下直接从原工作区的新模板复制；用户已完成复制，无须重复。

```powershell
New-Item -ItemType Directory -Force -Path 'E:\sfm\triplets'
Copy-Item -LiteralPath '<工作记录目录>\feedback_local_setup\x64-windows.cmake.example' -Destination 'E:\sfm\triplets\x64-windows.cmake'
Get-Content -LiteralPath 'E:\sfm\triplets\x64-windows.cmake'
```

应包含：

```cmake
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_PLATFORM_TOOLSET v143)
set(VCPKG_PLATFORM_TOOLSET_VERSION 14.36.32532)
set(VCPKG_CMAKE_SYSTEM_VERSION 10.0.22621.0)
set(VCPKG_ENV_PASSTHROUGH CUDA_PATH)
```

在同一PowerShell里设置本轮使用的工具位置。变量只影响当前会话及其子进程：

```powershell
$env:VCPKG_ROOT = 'E:\sfm\vcpkg'
$env:CUDA_PATH = 'D:\CUDA\v12.2'
$env:PATH = "$env:CUDA_PATH\bin;$env:PATH"
$env:VCPKG_FORCE_DOWNLOADED_BINARIES = '1'
Remove-Item Env:\VCPKG_FORCE_SYSTEM_BINARIES -ErrorAction SilentlyContinue
```

FORCE_DOWNLOADED_BINARIES要求vcpkg使用本基线工具清单中的工具版本，包含CMake3.30.1；不会卸载原系统CMake4.0.2。配置依据：[triplet变量](https://learn.microsoft.com/en-us/vcpkg/users/triplets)、[vcpkg工具环境变量](https://learn.microsoft.com/en-us/vcpkg/users/config-environment)、[基线工具清单](https://github.com/microsoft/vcpkg/blob/b1b19307e2d2ec1eefbdb7ea069de7d4bcd31f01/scripts/vcpkg-tools.json)。

S08-A检验：

```powershell
git -C 'E:\sfm\vcpkg' rev-parse HEAD
& 'E:\sfm\vcpkg\vcpkg.exe' version
Test-Path 'E:\sfm\vcpkg\scripts\buildsystems\msbuild\vcpkg.targets'
& 'E:\sfm\vcpkg\vcpkg.exe' env --triplet x64-windows --overlay-triplets='E:\sfm\triplets' 'where cl'
& 'E:\sfm\vcpkg\vcpkg.exe' env --triplet x64-windows --overlay-triplets='E:\sfm\triplets' 'set WindowsSDKVersion'
& 'E:\sfm\vcpkg\vcpkg.exe' env --triplet x64-windows --overlay-triplets='E:\sfm\triplets' 'set CUDA_PATH'
```

通过标准（已更正SDK项）：Git HEAD为上述基线；vcpkg.exe version可运行并记录完整版本；vcpkg.targets存在；where cl选中含VC\Tools\MSVC\14.36.32532\bin\Hostx64\x64\cl.exe的路径；记录SDK环境实际值；CUDA_PATH为D:\CUDA\v12.2。env可能首次下载辅助工具，按输出等待完成。官方依据：[vcpkg env检查构建环境](https://learn.microsoft.com/en-us/vcpkg/commands/env)。

SDK检查更正：助手此前把WindowsSDKVersion必须22621写为通过条件不准确。官方定义VCPKG_CMAKE_SYSTEM_VERSION只规定传入CMAKE_SYSTEM_VERSION；本机windows.cmake第39–40行也仅设置CMake缓存项，不证明vcvars环境同步为相同值。用户env实际为10.0.26100.0，该SDK已安装；当前分别记录22621目标值和26100环境值，不把环境值不同认定为安装失败或声称实际编译使用22621。S08-B具体编译时头文件/库使用版本以日志为准。[官方变量定义](https://learn.microsoft.com/en-us/vcpkg/users/triplets#vcpkg_cmake_system_version)。

验收结果（2026-10-14 14:07）：用户提交全部输出，助手确认完整非浅克隆、Git基线一致、工作区干净、vcpkg完整版本2025-07-21-d4b65a2b83ae6c3526acd1c6f3b51aff2a884533；CMake实际3.30.1；用户integrate成功、cl路径14.36.32532、SDK环境26100、CUDA D盘12.2。S08-A完成；完整manifest依赖状态文件尚不存在，S08-B未完成。

#### S08-B：安装完整manifest（已完成）

历史下载故障（2026-10-14 15:05）：用户14:18启动install，PortableGit2.47.1下载解压成功，随后停在PowerShell7.2.24工具下载。助手只读检查vcpkg进程仍运行；PowerShell-7.2.24-win-x64.zip.12156.part文件两次均0字节，LastWriteTime为14:39；最终ZIP不存在。不能据无进度条直接判断下载卡住，但本次文件无增长证明当时没有实际文件传输进展。后续已取得完整ZIP，最新日志推进到GMP。本次当前故障见下方“GMP下载修复”；历史PowerShell备用下载命令仅供留档。原记录的Start-Transcript包含主机信息，助手读取后仅提取相关诊断事实，不将无关主机信息复制到本账本。

辅助工具说明：VCPKG_FORCE_DOWNLOADED_BINARIES使vcpkg按基线工具清单获取独立工具，因此即使系统已有Git/PowerShell，也可能下载指定版本。这是本路线选择的工具版本策略，不要求升级或替换用户的Windows PowerShell窗口。

助手以正常网络权限对PowerShell官方文件URL做HEAD请求，直连和Clash127.0.0.1:7897代理都返回HTTP200、application/octet-stream，说明新的下载连接当前可建立；HEAD成功不等于文件已下载。采用持久缓存文件单独下载和SHA512验证，避免让旧任务继续无进度等待。

处理步骤：在正在运行install的原窗口按Ctrl+C一次，等待PS提示符返回，再Stop-Transcript。只终止用户正在等待的这次命令，不主动终止其他PowerShell进程、不删除vcpkg仓库/缓存。用户随后在同一窗口执行：

```powershell
$taskPsTool = (Get-Content -LiteralPath 'E:\sfm\vcpkg\scripts\vcpkg-tools.json' -Raw | ConvertFrom-Json).tools |
    Where-Object { $_.name -eq 'powershell-core' -and $_.os -eq 'windows' }
$taskPsArchive = Join-Path 'E:\sfm\vcpkg\downloads' $taskPsTool.archive
curl.exe --progress-bar --location --fail --continue-at - --connect-timeout 30 --speed-time 120 --speed-limit 1024 --output "$taskPsArchive" "$($taskPsTool.url)"
$LASTEXITCODE
```

当前清单对应PowerShell-7.2.24-win-x64.zip，地址https://github.com/PowerShell/PowerShell/releases/download/v7.2.24/PowerShell-7.2.24-win-x64.zip，期望SHA512为a08b72958f5a552240d3f68c581d8c8cb580468a71f5e55ca54a1dd0c0fcd81da9df11036653e2300fc4a5778a77c0147832ca06f7837f03417e9795e577a76f（来自本机固定基线的官方工具清单）。下载中断时保留这个最终路径文件、重复同一curl命令续传；不要同时运行vcpkg与curl写入同一个最终缓存文件。

下载返回0后校验：

```powershell
Get-Item -LiteralPath $taskPsArchive | Select-Object Name,Length
(Get-FileHash -LiteralPath $taskPsArchive -Algorithm SHA512).Hash -eq $taskPsTool.sha512
```

校验True后无须手动解压，将install日志追加到原记录，再重试同一完整manifest安装命令：

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
Start-Transcript -Path 'E:\sfm\setup\S08-vcpkg-install.log' -Append
& 'E:\sfm\vcpkg\vcpkg.exe' install --triplet x64-windows --host-triplet x64-windows --overlay-triplets='E:\sfm\triplets'
$taskVcpkgInstallExit = $LASTEXITCODE
Stop-Transcript
$taskVcpkgInstallExit
```

vcpkg会检查缓存hash并自行解压工具，已下载的其他工具保留。本节下方初始设置保留供重开窗口时恢复环境；S08-B仍须整个install退出0才能完成。若curl失败或校验False，先返回错误/文件大小，不据HEAD200跳过验证。

按本次成功的网络方式继续，不强制启用代理；若重新打开终端，重设工具环境变量。设置下载工具、二进制缓存和并行数：

```powershell
$env:VCPKG_ROOT = 'E:\sfm\vcpkg'
$env:CUDA_PATH = 'D:\CUDA\v12.2'
$env:PATH = "$env:CUDA_PATH\bin;$env:PATH"
$env:VCPKG_FORCE_DOWNLOADED_BINARIES = '1'
Remove-Item Env:\VCPKG_FORCE_SYSTEM_BINARIES -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path 'E:\sfm\vcpkg-cache'
$env:VCPKG_DEFAULT_BINARY_CACHE = 'E:\sfm\vcpkg-cache'
$env:VCPKG_MAX_CONCURRENCY = '4'
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
Start-Transcript -Path 'E:\sfm\setup\S08-vcpkg-install.log'
```

必须在含vcpkg.json的SfM仓库根目录执行，不在vcpkg工具目录逐个安装最新版库：

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'
& 'E:\sfm\vcpkg\vcpkg.exe' install --triplet x64-windows --host-triplet x64-windows --overlay-triplets='E:\sfm\triplets'
$taskVcpkgInstallExit = $LASTEXITCODE
Stop-Transcript
$taskVcpkgInstallExit
```

通过标准：install完整执行成功、退出码0，末尾显示全部请求的库已安装。若失败，先停止并返回首个失败包、最后的错误、列出的日志路径；可从该失败包的configure/build日志诊断，不以残留头文件判断整步通过。此阶段会下载、编译较多依赖，实际时长取决于网络和构建过程。

S08-B补充检验：

```powershell
& 'E:\sfm\vcpkg\vcpkg.exe' version
git -C 'E:\sfm\vcpkg' rev-parse HEAD
Test-Path 'E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\include\ceres\ceres.h'
Test-Path 'E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\include\CGAL'
Test-Path 'E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\include\eigen3\Eigen\Core'
& 'E:\sfm\vcpkg\vcpkg.exe' list --x-install-root='E:\sfm\OntheflySfMFeedback\vcpkg_installed'
```

通过标准：install成功且完成全部包，三个头文件/目录True；日志选中正确工具集和Toolkit。记录vcpkg工具版本/提交、实际依赖版本、使用的CMake版本及E:\sfm\setup\S08-vcpkg-install.log。原manifest基线不升级。

#### S08-B当前故障：GMP下载修复（2026-10-14 17:10）

用户最新附件显示：LLVM15.0.6下载成功，随后GMP获取autoconf2.71-2.71-3-any.pkg.tar.zst失败。MSYS2官方源及多个镜像返回404；清华/中科大还存在12029连接失败。首要阻塞是旧文件地址不可用，日志中的代理建议是通用提示。最后退出码1表示整个install失败，Ceres/zlib等缓存完成只证明部分工作成功，不能进入S09。

官方当前提供同一Autoconf2.71的包装修订4。助手已从https://repo.msys2.org/msys/x86_64/autoconf2.71-2.71-4-any.pkg.tar.zst下载335194字节文件，并核对SHA256与[MSYS2官方包页](https://packages.msys2.org/packages/autoconf2.71?repo=msys&variant=x86_64)一致。记录：

* SHA256：55BE2B911BEAA0424D60FDCF206F2411A0EB24CB53E66A82964497EF8A4FAAC0。
* SHA512：C93B791EB55893CBE7C425E764074837355FD165DEB7B1775F652C8E25D9D1F0CDD4120AB710D56FB859B7DF55C4F971ECCDA7C112448F60615BFF8A2DC81166。
* GMP源来自固定vcpkg提交b1b19307e2d2ec1eefbdb7ea069de7d4bcd31f01；6.3.0#2。原portfile SHA256：CE93CF68ABED4628D53FA83F0A0CFF045639514324CDBF99714606C4DF5CC37A。
* 修复只替换portfile.cmake的Autoconf下载URL和SHA512两行；保留WANT_AUTOCONF=2.71及全部GMP补丁/版本。未换成2.72，因为原端口说明该版本存在dumpbin检测问题。
* 已准备文件：原工作区feedback_local_setup\overlay-ports\gmp（9个文件）及feedback_local_setup\downloads\autoconf2.71-2.71-4-any.pkg.tar.zst。

使用[vcpkg官方overlay机制](https://learn.microsoft.com/en-us/vcpkg/concepts/overlay-ports)，本机覆盖GMP构建配方，其余端口仍来自项目固定基线。具体复制、校验、重试命令见[本轮修复说明](<E:/101研0/on th fly SFM/feedback_local_setup/GMP下载修复说明.md>)。完成复制后，在原安装命令增加--overlay-ports='E:\sfm\overlay-ports'，并保持CUDA、工具集、缓存及并发设置。后续项目构建也要沿用此overlay路径，S11配置时核查。

检验：缓存包SHA256匹配；overlay portfile引用2.71-4及上述SHA512；安装输出确认GMP使用本地overlay；完整install退出0且全部请求包完成才将S08-B标为通过。准备文件和哈希已验证，尚未执行GMP实际构建。保留已有安装及缓存；此故障不要求升级整个manifest基线。

修复后进展（2026-10-14 17:16）：用户完成复制、hash与端口检查；日志确认MSVC14.36.32532、本地GMP overlay和缓存autoconf2.71-2.71-4。autoconf-wrapper-20240607-1在mirror.msys2主源下载失败，按1000/2000/4000ms重试后切换repo.msys2并成功，随后开始automake-wrapper下载重试。助手只读核查autoconf-wrapper缓存4997字节及vcpkg/cmake进程仍在运行。当前warning表示下载失败后的恢复尝试，该包已有备用源成功证据；不能据此认定整个安装失败或完成。vcpkg本轮自动读取Windows代理127.0.0.1:7897，暂不改动正在执行命令的网络设置。

S08-B最终验收（2026-10-14 18:24）：用户最新结果完整成功、退出码0，用时1.1h；助手复核完整安装账本、manifest直接依赖和主要头文件/库。S08完成，原基线保留，GMP局部overlay修复已通过实际安装；详细版本及证据见2.4。上述17:10/17:16描述是历史故障与重试状态。

### S09：TBB（已完成，操作存档）

操作：获取[oneTBB2021.9.0官方Windows SDK](https://github.com/uxlfoundation/oneTBB/releases/download/v2021.9.0/oneapi-tbb-2021.9.0-win.zip)，保存为E:\sfm\setup\oneapi-tbb-2021.9.0-win.zip。使用Release的Windows预编译包；本步骤准备SDK，S11再手动接入VS项目。当前manifest没有TBB条目，助手确认E:\sfm\onetbb尚不存在。

先核对[官方发布页](https://github.com/uxlfoundation/oneTBB/releases/tag/v2021.9.0)给出的SHA256：

```powershell
(Get-FileHash -LiteralPath 'E:\sfm\setup\oneapi-tbb-2021.9.0-win.zip' -Algorithm SHA256).Hash -eq '0C4D96EDD0469CC93F611F94365EC07F3A0DD529B96D4E6CEAC378DE63920B1C'
```

应为True再解压。资源管理器解压后，先新建E:\sfm\onetbb，再将SDK中包含include、lib、redist的那层目录内容复制进去，使include直接位于根目录。也可按[本轮S09操作说明](<E:/101研0/on th fly SFM/feedback_local_setup/S09-oneTBB配置说明.md>)自动解压并整理目录。本次2021.9.0 Windows ZIP实际使用redist保存DLL，先前写bin是通用布局假设，现已按实际文件更正。

2026-10-14 18:32核查：用户已下载并解压；ZIP SHA256 True。实际SDK根为E:\sfm\setup\oneapi-tbb-2021.9.0-win\oneapi-tbb-2021.9.0，桌面x64 Release库位于lib\intel64\vc14\tbb12.lib，DLL位于redist\intel64\vc14\tbb12.dll。目标onetbb尚不存在，当前需要新建并复制这一层内容，避免把整个外层解压目录放进去。S09尚未标为完成。

检验：

```powershell
Test-Path 'E:\sfm\onetbb\include\tbb\global_control.h'
Get-ChildItem 'E:\sfm\onetbb' -Recurse -Filter 'tbb12.lib'
Get-ChildItem 'E:\sfm\onetbb' -Recurse -Filter 'tbb12.dll'
Get-Content 'E:\sfm\onetbb\include\oneapi\tbb\version.h' | Select-String '^#define TBB_VERSION_(MAJOR|MINOR|PATCH)'
```

通过标准：ZIP hash True、global_control.h存在、版本2021/9/0，能明确选出intel64\vc14下的Release tbb12.lib及tbb12.dll。记录include/lib/DLL三个实际目录，不以压缩包文件名独自证明版本。此阶段不要求项目编译或安装完整oneAPI工具套件。

S09验收结果（2026-10-14 18:37）：用户创建onetbb并复制SDK，三个存在性检查True、版本2021/9/0；助手按最终路径重复读取并确认。include目录为E:\sfm\onetbb\include；lib目录为E:\sfm\onetbb\lib\intel64\vc14；DLL目录为E:\sfm\onetbb\redist\intel64\vc14。当前完成SDK准备，不据此认定项目已链接或运行。

### S10：INI 与模型独立加载（已完成，操作存档）

操作：创建E:\sfm\runtime\Release和E:\sfm\data\trial01\images，将setup中的settings.ini.example复制为E:\sfm\runtime\settings.ini。助手确认模板与原工作区版本hash一致；PythonHome=E:/sfm/envs/feedback310、PythonWorkspace=E:/sfm/OntheflySfMFeedback/scripts/pyscripts、GlobalFeatureModel=E:/sfm/models/netvlad均与当前路径匹配。INI是参考模板而非作者配置。

复制最新check_environment.py到setup。本轮增加模型加载阶段提示、描述子有限值与非零范数检查，语法已通过；完整模型测试待真实照片。本次runtime\settings.ini尚不存在，不覆盖已有用户配置。完整操作与预期输出见[S10配置与模型验证说明](<E:/101研0/on th fly SFM/feedback_local_setup/S10-配置与模型验证说明.md>)。

准备一张正常可打开的真实JPG照片，复制为E:\sfm\data\trial01\images\00001.jpg。若用PNG保留.png并同步修改测试命令中的文件名。本步骤仅验证模型提取，完整SfM数据实验仍在S13。

检验：

```powershell
& 'E:\sfm\envs\feedback310\python.exe' -u 'E:\sfm\setup\check_environment.py' --repo 'E:\sfm\OntheflySfMFeedback' --ini 'E:\sfm\runtime\settings.ini' --model-test --image 'E:\sfm\data\trial01\images\00001.jpg' 2>&1 | Tee-Object -FilePath 'E:\sfm\setup\S10-model-test.log'
$taskModelTestExit = $LASTEXITCODE
$taskModelTestExit
```

通过标准：没有[FAIL]，最终Result: PASS，退出码0；实际描述子形状(1,2048)，所有值有限且范数为正有限值。记录测试图片路径、检查输出与日志。若PCA或模型加载失败，保留首条异常按明确证据调整，不在模型测试前擅自换包版本。命令行Python通过不代表内嵌Python一定正常，后者在S12检查。

S10最终验收（2026-10-14 18:48）：用户完整模型测试PASS、退出0；助手复核日志/INI。特征维数、有限值和范数均符合检查，完整证据见2.5。上述未创建/待执行等描述为本步骤执行前状态。

### S11：工程修正与编译（当前步骤，分A/B/C验收）

S11-A已通过：用户替换CUDA四导入为12.2，diff恰为两个工程各两行替换，五项Qt/CUDA文件检查True；截图显示9项目全部加载，无不可用/卸载标记。未出现重定向对话框不影响加载验收；工具栏Release/x64在B应用后再次确认。操作留档见[S11-A说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-A工程加载准备.md>)。

S11-B已通过：用户在正式仓库应用local.windows.early.props、local.windows.late.props并完成九工程配置检查，均PASS、退出0，助手复核日志/JSON和两个props hash。Git差异与预期一致，完整证据见2.6；操作留档见[S11-B说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-B公共工程配置.md>)。早期属性表固定工具/SDK/Qt/CUDA/vcpkg根目录，后期补充头文件、库、UI tbb12.lib及OutDir/IntDir；只对Release/x64生效。C++算法文件未改。

B验证准备：已在原工作区validation\S11B-projects副本应用配置，并用本机MSBuild运行九项目/getProperty检查，全部[OK]、PASS、退出0；MSVC及VCToolsInstallDir=14.36.32532、SDK及头文件=26100、Qt UI/Workflow根目录正确、CUDA D盘12.2、vcpkg正式安装目录正确、统一输出E:\sfm\runtime\Release与各项目独立IntDir。完整报告存于feedback_local_setup\validation\S11B-checked。没有运行构建目标，也未对正式E:\sfm仓库执行B修改。

S08完整依赖已安装验收，B设置VcpkgManifestInstall=false使VS使用现有库；vcpkg集成/链接保持开启，overlay安装参数保留，后续改依赖时先显式执行S08安装命令。独立TBB按头文件/库手动接入；遗留Intel TBB/MKL集成关闭。正式B已通过，可进入C。

S11-C1已通过：用户生成成功、36警告/0错误、退出0，助手复核非空SIFTGPU.lib及实际cl/nvcc工具路径。操作留档见[SIFTGPU首次编译说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C1-SIFTGPU编译.md>)，完整证据见2.7。

S11-C2已通过：用户生成成功、0警告/0错误、退出0，助手复核VLFeat.lib大小3322520字节、时间与日志相符，并记录源码版本0.9.20。操作留档见[VLFeat首次编译说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C2-VLFeat编译.md>)，证据见2.7。

S11-C3已通过：首次glog C4996失败后，用户应用局部兼容头及属性表，正式Base重试80警告/0错误、退出0，Base.lib非空且时间相符。实际编译仍有/sdl及/FI修复头，详情见2.7和[glog局部修复说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C3-glog兼容修复.md>)。

S11-C4已通过：Geometry生成35警告/0错误、退出0，助手复核非空库及本轮时间。证据见2.7，操作留档见[Geometry首次编译说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C4-Geometry编译.md>)。

S11-C5已通过：Scene生成741警告/0错误、退出0，助手复核178690940字节静态库及CUDA12.2实际命令。警告与证据见2.7，操作留档见[Scene首次编译说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C5-Scene编译.md>)。

S11-C6已通过：用户确认上一步正确完成；助手复核Estimator108警告/0错误、54.49秒和185752884字节静态库。证据见2.7，操作留档见[Estimator首次编译说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C6-Estimator编译.md>)。

S11-C7已通过：用户确认上一步正确完成；助手复核Feature156警告/0错误、4.07秒和29944734字节静态库。证据见2.7，操作留档见[Feature首次编译说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C7-Feature编译.md>)。内嵌Python运行验证需之后GUI完整启动。

S11-C8已通过：用户确认上一步正确完成；助手复核Workflow1433警告/0错误、69.99秒和265410444字节静态库，日志包含moc生成源编译。证据见2.7，操作留档见[Workflow首次编译说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C8-Workflow编译.md>)。

S11-C9已通过：UI549警告/0错误、退出0，助手复核5773312字节exe及日志。八个静态库与UI.exe均已生成，S11构建完成；证据见2.7，操作留档见[UI首次编译与链接说明](<E:/101研0/on th fly SFM/feedback_local_setup/S11-C9-UI编译与链接.md>)。DLL部署与首次启动在S12进行。

操作：指南第10节逐项修正。Release/x64，统一MSVC14.36、SDK、CUDA12.2构建定制、Qt5套件、Python/NumPy/TBB/manifest路径；保留继承目录。统一OutDir为E:\sfm\runtime\Release\，分模块构建。

顺序：SIFTGPU → VLFeat → Base → Geometry → Scene → Estimator → Feature → Workflow → UI。

检验：生成输出中的成功/失败统计以及首条错误；检查实际cl/nvcc路径。辅助文件存在检查：

```powershell
Get-Item 'E:\sfm\runtime\Release\SIFTGPU.lib','E:\sfm\runtime\Release\VLFeat.lib','E:\sfm\runtime\Release\Base.lib','E:\sfm\runtime\Release\Geometry.lib','E:\sfm\runtime\Release\Scene.lib','E:\sfm\runtime\Release\Estimator.lib','E:\sfm\runtime\Release\Feature.lib','E:\sfm\runtime\Release\Workflow.lib','E:\sfm\runtime\Release\UI.exe' | Select-Object Name,Length,LastWriteTime
```

通过标准：本轮各模块构建成功，文件非空且时间对应本轮；不能拿以前残留exe判定本次成功。保存生成日志、实际编译工具版本和git diff。

### S12：部署与启动

当前：S12完整通过：A文件验收、B内嵌模型加载和用户GUI确认均完成，证据见2.8、2.10和2.11。失败与修复历史见2.9和[ffi运行库修复说明](<E:/101研0/on th fly SFM/feedback_local_setup/S12-B-ctypes与ffi运行库修复.md>)。原始启动操作见[S12部署与首次启动说明](<E:/101研0/on th fly SFM/feedback_local_setup/S12-部署与首次启动.md>)。

操作：A使用Qt5.12.12的windeployqt确认部署，复制本项目环境的python310.dll与独立SDK的tbb12.dll，检查INI及文件/hash；B设置本次进程的Python/CUDA/vcpkg路径及Qt插件路径，固定工作目录E:\sfm\runtime\Release，启动并采集日志。不修改系统PATH，不在部署阶段改算法源码。

检验：

```powershell
Set-Location -LiteralPath 'E:\sfm\runtime\Release'
Test-Path '..\settings.ini'
Test-Path '.\platforms\qwindows.dll'
.\UI.exe
```

通过标准：两个路径True；GUI出现；日志显示Python3.10和The global extractor has been successfully loaded!；无模型、DLL或模块加载错误。等待加载成功再开始照片输入。

### S13：本地照片 SfM

trial02已完成首次SfM运行，128张图全部注册并得到31039个三维点，证据见2.14及[S13-B trial02首次运行说明](<E:/101研0/on th fly SFM/feedback_local_setup/S13-B-trial02首次SfM运行.md>)。GUI数据库导出路径为E:\sfm\data\trial02\result\trial02.db，图像路径为E:\sfm\data\trial02\images。旧测试图和原COLMAP数据保留。以下trial01操作是早期通用计划，正式新实验以trial02专属说明为准。

操作：先准备建议80–120张清晰、有重叠的静态场景照片；数量是试跑建议，不是算法硬性最低要求。创建新result目录；GUI New Project，选择新的不存在的trial01.db和images目录；Save后Start Receive。

检验：日志中SIFT提取成功、初始化成功、影像registered successfully；Model Viewer出现相机和非空三维点；最后注册计数持续变化并能解释失败影像。

通过标准：相机和点云来自本轮照片；只有图片读取数增加不算SfM通过。记录输入/注册影像数、点数、照片尺寸、处理耗时、失败原因。照片输入结束后等待后台队列完成。

### S14：Feedback

操作：首轮模板采用StartCreateMesh=40、Step=10；实际首个调度点为50张注册影像。本轮已完成50至120张之间的8次Feedback计算。保持Track Model勾选，先只勾选Show Mesh；确认后再勾选Show Quality；最后勾选Show Guide Path，并通过开关前后对照判断新增显示内容。

检验：8条路径规划日志已经证明反馈任务实际完成，且本轮路径不为空。分别检查是否有非空网格、质量着色、引导路径；当前截图中的红色相机是已注册相机，不能单独作为规划路径显示证据。显示操作及通过标准见[S14 Feedback显示验证](<E:/101研0/on th fly SFM/feedback_local_setup/S14-Feedback显示验证.md>)。

通过标准：各目标功能有本轮证据或明确可解释的场景条件；截图/日志对应同一轮数据。记录每次反馈注册数、耗时、显示结果。GUI路径显示不等于真实飞控执行。

### S15：导出

操作：本轮回放、重建和Feedback已结束，在GUI选择Project → Export Project一次。导出线程使用New Project时指定的E:\sfm\data\trial02\result\trial02.db，不再弹出路径选择；等待日志出现`Export database completed!`和源码原样拼写的`Export mdoels completed!`，再进行文件检查。完整说明见[S15导出与结果检查](<E:/101研0/on th fly SFM/feedback_local_setup/S15-导出与结果检查.md>)。

检验：

```powershell
& 'E:\sfm\envs\feedback310\python.exe' -u `
  '<工作记录目录>\feedback_local_setup\check_trial_export.py' `
  --result 'E:\sfm\data\trial02\result' `
  --database-name 'trial02.db' `
  --expected-images 128 `
  --expected-points 31039 `
  --report 'E:\sfm\setup\S15-trial02-export.json'
```

通过标准：本轮.db、Log.txt、match_matrix.bmp、models下的相机/影像/点云文件存在且符合数据内容。记录实际输出路径。当前默认调用链不保证自动导出PLY网格和规划路径文件，这不是此步骤的必过项。

### S16：固化环境

操作及检验：先关闭UI以刷新所有日志，按[S16-A环境冻结快照](<E:/101研0/on th fly SFM/feedback_local_setup/S16-A-环境冻结快照.md>)生成固定目录。脚本保存Python/Conda/vcpkg依赖、工具版本、Git HEAD/状态/差异补丁、本机属性表、INI、模型hash和导出结果hash；它不提交或删除文件。

```powershell
conda activate 'E:\sfm\envs\feedback310'
python -m pip freeze | Set-Content -Encoding utf8 'E:\sfm\setup\requirements-tested.txt'
conda env export --prefix 'E:\sfm\envs\feedback310' | Set-Content -Encoding utf8 'E:\sfm\setup\environment-tested.yml'
git -C 'E:\sfm\OntheflySfMFeedback' rev-parse HEAD | Set-Content -Encoding utf8 'E:\sfm\setup\source-commit.txt'
git -C 'E:\sfm\OntheflySfMFeedback' diff --stat
Test-Path 'E:\sfm\setup\requirements-tested.txt'
Test-Path 'E:\sfm\setup\environment-tested.yml'
Test-Path 'E:\sfm\setup\source-commit.txt'
```

通过标准：三份记录存在；配置文件已备份；本机工程/源码修改逐项记录或提交；模型来源/校验值齐全；同一数据能重复得到对应结果。更新本账本中的所有实际版本，避免仅保留目标版本。

## 五、待解决事项

| 问题 | 当前事实 | 下一步 |
| --- | --- | --- |
| 模型加载兼容性（已验证） | S10独立GPU提取PASS；S12-B内嵌Python3.10.22及四模型构造成功；S13有128条内嵌全局特征日志 | 已完成，无须重复加载测试 |
| CUDA工程导入（A已解决） | SIFTGPU/Scene已改12.2导入；历史头文件路径由B公共配置覆盖，SIFTGPU旧/external:I由脚本替换 | B应用/属性验收后，C核查实际nvcc编译 |
| requirements.txt缺失 | 已提供从默认导入推导的参考清单 | 真实模型加载后固化测试版本 |
| Qt/MSBuild项目构建与启动（已验证） | S11九个工程构建完成；输出Qt版本5.12.12.0；用户确认主窗口可操作；S13整组运行完成 | 已完成，继续S14显示验收 |
| torch下载中断（已解决） | 用户再次执行原命令安装成功；全部S06检查通过 | 无须重复安装；备用wheel下载方案留档 |
| SDK目标与环境值不同 | CMake目标值22621、vcpkg env实际26100，两套SDK均已安装 | 更正原A通过标准；记录实际构建头文件/库选择；不将不同变量混为一项 |
| GMP旧工具包下载失效（已解决） | 局部overlay改为包装修订4后完整install退出0 | 保留下载包、overlay及版本记录，后续构建沿用 |
| Base/glog头文件兼容（已解决） | 局部兼容头应用后完整Base重试退出0；/sdl保留 | 保留修复及失败/成功日志；后续src模块继续使用该兼容头 |
| Scene警告审查 | C5构建成功；日志存在源字符集无效字符C4828和异常展开提示C4530等 | 保留原始日志，在源码/运行阶段核对编码、异常配置及调用影响 |
| 内嵌Python的_ctypes依赖（已解决） | ffi-8.dll补齐且来源hash一致；retry1内嵌模型加载成功 | 保留DLL部署记录及首次失败/重试成功日志 |
| 运行启动验证（已完成） | 环境/依赖、完整C++构建、内嵌模型初始化及用户GUI确认通过 | 已完成 |
| SfM试跑（已完成） | trial02的128张图全部注册，得到31039个三维点，相机和点云可见 | 保留本轮证据，后续做重复性/精度实验 |
| Feedback显示（已完成） | 8次非空路径规划；网格、质量和引导路径均有截图；网格存在长三角形 | 保留证据；后续单独研究网格优化及真实飞控接入 |
| 结果导出（已完成） | 数据库完整，COLMAP二进制/文本模型及统计、日志、匹配矩阵齐全；自动检查退出0 | 最终冻结快照已记录hash和依赖 |
| 环境与源码固化（已完成） | 本机配置提交f3d023b；工作区干净；最终冻结PASS；vcpkg175项、32个文件hash | 以后从该提交和冻结目录恢复或对比 |

## 六、执行历史

| 时间 | 步骤 | 操作/证据 | 结果 |
| --- | --- | --- | --- |
| 2026-10-13 20:51 | S00 | 检查Git、VS、MSVC、SDK、CUDA、Qt目录、conda环境、GPU | 完成只读基线记录 |
| 2026-10-13 20:51 | S01 | git status/branch/HEAD/remote | main；工作区干净；提交和origin正确 |
| 2026-10-13 20:51 | S02 | 检查E:\sfm不存在 | 待用户建立短路径并返回检验输出 |
| 2026-10-13 21:01–21:02 | S02 | 用户复制源码和辅助文件，建立local/windows-setup并提交检验输出 | 用户操作全部符合要求 |
| 2026-10-13 21:05 | S02 | 助手在E:\sfm\OntheflySfMFeedback复核HEAD/branch/remote/status及四项文件 | 已完成；工作区干净 |
| 2026-10-13 21:05 | S03 | 再次核查VS组件及MSVC目录 | C++已安装；Python工作负载及14.36未检测到，等待用户安装 |
| 2026-10-13 21:22 | S03 | 用户提供五项组件True、MSVC目录及VS版本；助手只读复核 | 已完成；MSVC14.36.32532；VS17.13.35828.75 |
| 2026-10-13 21:22 | S04 | 核查E:\Qt与D:\Qt下5.12.12 MSVC套件常见路径；阅读Qt官方安装与扩展注册说明 | 两处qmake路径False；等待安装、扩展版本与VS注册证据 |
| 2026-10-13 22:28 | S04 | 用户提交全部命令、Qt VS Tools3.5.0及Qt Versions截图；助手复核qmake及文件 | 安装与注册检查通过；Qt5.12.12，QMake3.1；构建未验证 |
| 2026-10-13 22:28 | S05 | 核查Toolkit默认目录及VS构建定制，阅读NVIDIA12.2 Update2官方文档 | 仅发现v12.4，未发现12.2构建定制；等待安装 |
| 2026-10-13 23:21 | S05 | 用户提供D盘安装结果；助手复核nvcc、开发文件、VS集成、12.4及驱动 | 已完成；D:\CUDA\v12.2，nvcc12.2.140；驱动617.42 |
| 2026-10-13 23:21 | S06 | 核查conda和辅助包清单、目标环境目录，核对作者及官方PyTorch安装命令 | conda24.11.3，清单可读，目标目录未创建；等待用户执行 |
| 2026-10-14 00:39 | S06 | 用户返回torch下载中断/WinError32；助手核查项目解释器、基础包、pip check及torch状态 | Python3.10.22/64位与六个基础包已安装；pip26.2.1；torch三个包未安装 |
| 2026-10-14 00:39之后 | S06 | 读取PyTorch官方wheel索引和响应头，制定持久目录续传/校验/本地安装方案 | 下载方案待用户执行；S06仍部分完成 |
| 2026-10-14 01:02 | S06 | 用户附件：原命令重试安装成功、pip check/版本/GPU/开发文件/pip list；助手重复检查 | 已完成；实际版本已记完整快照；GPU运算512.0 |
| 2026-10-14 01:02之后 | S07 | 阅读源码、仓库README、项目主页、Issues及torchvision官方定义 | VGG16来源已确认；其余三个文件待作者/用户提供；模型目录尚不存在 |
| 2026-10-14 01:02之后 | S07 | 用户明确没有模型；助手追溯基础项目README并读取其公开Google Drive目录 | 返回200；列出四个所需同名文件，大小已记；待下载及兼容验证 |
| 2026-10-14 10:50–10:55 | S07 | 用户建立模型目录、返回四文件大小/hash；助手逐文件重新计算 | 文件验收已完成；大小与Drive元数据一致，完整hash存档 |
| 2026-10-14 10:55 | S08-A | 确认vcpkg/triplets目录不存在、SDK22621存在；核对官方基线工具清单，更新原工作区triplet模板 | 等待工具准备；目标MSVC14.36.32532/SDK22621/CUDA D盘12.2，CMake由vcpkg管理 |
| 2026-10-14 11:11之后 | S08-A | 用户多次克隆失败，确认Clash Verge开启；助手核查代理、目标目录、DNS和连接 | Git未设代理；直连重置；目标目录不存在；7897端口监听 |
| 2026-10-14 11:11之后 | S08-A | 助手通过http.proxy=127.0.0.1:7897和HTTP/1.1执行ls-remote | 正常权限下退出码0，取得远程HEAD；等待用户在代理终端重试clone |
| 2026-10-14 13:44 | S08-A | 用户传输对象时curl56/early EOF；助手检查目标目录、Git进程和7897端口 | 克隆未完成；目标目录无残留；端口可用；改为显式代理及按需文件克隆，待用户执行 |
| 2026-10-14 13:58–14:07 | S08-A | 用户报告关闭代理后完成原命令完整克隆、checkout、bootstrap、integrate、triplet及env；助手复核仓库/工具 | A已完成；vcpkg2025-07-21-d4b65a2b83ae6c3526acd1c6f3b51aff2a884533；CMake3.30.1；SDK环境26100 |
| 2026-10-14 14:07 | S08-A/B | 核对SDK目标变量含义，更正原通过条件；未发现manifest依赖状态文件 | 保留工具及模板；进入B，不重克隆、不强制切换网络 |
| 2026-10-14 14:18–15:05 | S08-B | 用户开始manifest安装，停在PowerShell7.2.24下载；助手两次检查.part为0及vcpkg进程 | 尚未完成；新连接HEAD直连/代理200；准备单独缓存下载、SHA512校验后重试 |
| 2026-10-14 17:10 | S08-B | 用户最新日志：LLVM成功、GMP旧Autoconf多源404、整次退出1，Ceres/zlib等缓存完成；助手核查固定端口、下载官方2.71-4并验证hash | S08-B未完成；准备GMP6.3.0#2局部overlay及已校验包；待用户复制重试；未升级基线/删除缓存 |
| 2026-10-14 17:16 | S08-B | 用户复制校验后重试；本地GMP overlay/2.71-4缓存生效；autoconf-wrapper主源重试、备用源成功，automake-wrapper正在重试 | 安装进行中；Ceres/Eigen等已有installed证据；warning暂未导致整次失败；等待最终退出码 |
| 2026-10-14 18:24 | S08-B | 用户报告全部安装成功、用时1.1h、退出0；助手复核149基础端口均installed、无缺manifest依赖、五项头文件/目录True及主要lib | S08完成；保存CSV与status版本快照；当前进入S09，TBB目标目录尚未建立 |
| 2026-10-14 18:32 | S09 | 用户指出onetbb目录不存在；助手复核SDK已下载/解压、官方ZIP hash True及实际头文件/lib/DLL布局 | 补充明确的新建与复制命令；更正DLL目录为redist；最终复制/版本验收待用户执行 |
| 2026-10-14 18:37 | S09/S10 | 用户复制到onetbb，三项True、版本2021/9/0；助手复核通过并检查INI模板/验证脚本与源码 | S09完成；S10当前待执行；INI尚不存在，已准备模型加载/有限值/非零检查及操作说明 |
| 2026-10-14 18:48 | S10/S11-A | 用户模型完整输出PASS、退出0；助手复核日志/INI、输入hash、源码状态、CUDA导入及Qt关键文件 | S10完成；特征范数1.1084929019709315；准备S11-A工程加载步骤，C++尚未编译 |
| 2026-10-14 19:00–19:13 | S11-A/B | 用户CUDA四导入、两工程diff、五True、截图9项目加载；助手复核diff，准备B属性表/备份配置脚本/检查脚本，并在九份副本实际评估MSBuild | A完成；B副本9项PASS、退出0；正式B待用户应用；未执行编译 |
| 2026-10-14 19:23 | S11-B/C1 | 用户正式应用/九项检查PASS且两个退出0、Git差异符合预期；助手复核日志/JSON、属性表hash及MSBuild版本 | B完成，保存正式报告快照；C1待执行，SIFTGPU.lib及构建日志尚不存在 |
| 2026-10-14 19:29–19:32 | S11-C1/C2 | 用户SIFTGPU生成成功、36警告/0错误、退出0、文件22768192字节；助手复核日志、工具路径、lib时间/hash | C1通过；保存日志快照和警告类别；C2待执行，整个S11未完成 |
| 2026-10-14 19:37–19:38 | S11-C2/C3 | 用户VLFeat生成成功、0警告/0错误、退出0、文件3322520字节；助手复核日志/lib/hash及源码版本0.9.20 | C2通过；保存日志快照；C3待执行，Base.lib与日志尚不存在 |
| 2026-10-14 19:41–19:54 | S11-C3 | 用户Base首次75警告/7错误、退出1；助手定位glog自有弃用声明及/sdl，准备局部兼容头/属性表并做最小编译对照与警告恢复测试 | C3未通过；局部测试正向退出0、调用方错误策略保留；正式修复待复制与重试 |
| 2026-10-14 19:59–20:02 | S11-C3/C4 | 用户Base重试80警告/0错误、退出0、29458872字节；助手复核日志、/sdl及/FI、工具集/MD和产物hash | C3通过，保留失败与成功快照；C4待执行，Geometry产物及日志尚不存在 |
| 2026-10-14 20:06–20:07 | S11-C4/C5 | 用户Geometry35警告/0错误、退出0、50846562字节；助手复核日志/lib/hash及Scene工程源文件项 | C4通过；保存成功快照；C5待执行，Scene产物及日志尚不存在 |
| 2026-10-14 20:12–20:13 | S11-C5/C6 | 用户Scene741警告/0错误、退出0、178690940字节；助手复核日志/lib/hash及实际CUDA12.2命令 | C5通过；保存成功快照和警告类别；C6待执行，Estimator产物及日志尚不存在 |
| 2026-10-14 20:22–20:23 | S11-C6/C7 | 用户确认上一轮正确完成；助手复核Estimator108警告/0错误、54.49秒、185752884字节及hash | C6通过；保存成功日志快照；C7待执行，Feature产物及日志尚不存在 |
| 2026-10-14 20:25–20:26 | S11-C7/C8 | 用户确认上一轮正确完成；助手复核Feature156警告/0错误、4.07秒、29944734字节及hash | C7通过；保存成功日志快照；C8待执行，Workflow产物及日志尚不存在 |
| 2026-10-14 20:29–20:30 | S11-C8/C9 | 用户确认上一轮正确完成；助手复核Workflow1433警告/0错误、69.99秒、265410444字节及hash、moc生成源编译 | C8通过；保存成功日志快照；C9待执行，UI产物及日志尚不存在 |
| 2026-10-14 20:35–20:39 | S11-C9/S12 | 用户UI549警告/0错误、退出0、5773312字节；助手复核日志/hash/直接依赖、Qt部署版本及INI定位 | S11完成；缺python310.dll与tbb12.dll，正确来源存在；S12-A待执行，未启动正式UI |
| 2026-10-14 20:49 | S12-A/B | 用户确认部署符合通过标准；助手复核九项文件、DLL版本与两个源文件hash一致，保存Qt部署日志 | A通过；B待执行，当前无UI进程、无启动stderr日志 |
| 2026-10-14 20:56–21:01 | S12-B | 用户Python3.10.22初始化后_ctypes导入DLL失败；助手读取直接依赖、libffi元数据并做加载对照，保存失败日志 | B未通过；ffi-8.dll输出缺失，准备补充部署后用retry1日志重试 |
| 2026-10-14 21:11–21:15 | S12-B/S13准备 | 用户retry1模型成功日志及进程存活；助手复核日志、ffi hash和照片目录 | 内嵌模型加载通过；GUI待确认；仅一张测试图，试跑数据来源待用户回答 |
| 2026-10-14 21:27 | S12/S13-A | 用户确认GUI可操作并选择公开样例；助手核查COLMAP官方目录/release资产，准备ZIP校验脚本 | S12完成；South Building419421847字节，代下载未获批准，待用户下载和正式校验 |
| 2026-10-14 21:35 | S13-A | 用户提供已有South Building；助手检查128张源图解码、尺寸、唯一hash及文件清单 | 源照片通过，副本待用户创建/校验；跳过下载，不修改原COLMAP数据库与重建 |
| 2026-10-14 21:42 | S13-A/B | 用户确认副本通过；助手复核JSON、实际128文件及result/数据库状态 | A通过；B待GUI执行；128张输入不代表已完成SfM |
| 2026-10-14 21:50–21:58 | S13-B/S14 | trial02整组运行；用户提供最终日志和主窗口截图；助手复核128条特征、1次初始化、126条增量注册及8条路径规划 | S13完成：128 Images、31039 Points、相机/点云可见；S14计算通过、显示待逐项验收 |
| 2026-10-14 22:10–22:12 | S14-A | 用户勾选Show Mesh并提供截图；助手核对网格生成与过滤源码 | A通过：绿色三角网格正常显示；存在跨空白长三角形，质量问题单独记录，不影响显示功能验收 |
| 2026-10-14 22:16–22:18 | S14-B | 用户勾选Show Quality并提供截图；助手核对质量计算及颜色映射源码 | B通过：蓝—白—红质量分区清楚显示；颜色是当前模型内的相对质量分位 |
| 2026-10-14 22:22–22:24 | S14-C/S15 | 用户勾选Show Guide Path并提供截图；助手核对视点、路径与方向数据和颜色 | C通过，S14完成；准备按固定trial02路径导出并运行自动检查 |
| 2026-10-14 22:27–22:31 | S15/S16 | 用户导出并运行检查器；助手复核JSON、全部文件及数据库数量 | S15完成：PASS、退出0；进入S16冻结环境与源码差异 |
| 2026-10-14 22:35–22:38 | S16-A | 首次冻结脚本PASS但vcpkg提示无包；助手复核发现清单为空 | 部分通过；修复manifest安装根和非空检查，VS版本采集同步更正 |
| 2026-10-14 22:41–22:45 | S16-A/B | 用户重跑后vcpkg175条、32个hash、PASS/0；助手复核快照及12项Git改动 | A通过；B待在local/windows-setup建立本地提交 |
| 2026-10-14 22:48–22:50 | S16-B | 用户暂存12项、diff-check退出0并提交；助手复核完整提交号和干净状态 | B通过：f3d023b；该时点待最终冻结捕获新HEAD与clean状态 |
| 2026-10-14 22:52–22:55 | S16最终冻结 | 用户重跑脚本；助手复核报告、仓库HEAD/分支/clean状态并同步证据 | S16及本轮全流程完成：PASS/0，f3d023b，vcpkg175项，32个hash，HEAD补丁存在 |

## 七、本轮实验结果（首次运行后填写）

* 实验编号：trial02（首次GUI运行已完成，S13通过）。
* 输入照片目录、数量、尺寸：E:\sfm\data\trial02\images；South Building128张JPG，均3072x2304；总233850173字节；清单SHA256见2.13。
* 实际INI及反馈参数：E:\sfm\runtime\settings.ini；InitialImagesNum20；SIMPLE_RADIAL；useFocalLength=false/useFactor0.704；Feedback StartCreateMesh40/Step10；AutoExportModel=false。
* 成功注册影像数、三维点数：128/128张输入进入模型；31039个三维点。初始化使用2张，随后日志记录126张增量注册成功。
* 网格/质量/路径验证证据：8次Feedback路径规划均非空；末次low-quality faces=3000、clusters=20、candidates=928、selected viewpoints=60、path segments=59。Show Mesh、Show Quality和Show Guide Path均有截图并通过；网格存在明显长三角形、质量较粗。规划路径尚未在真实无人机上执行。
* 实际耗时与资源占用：GUI状态栏显示1min 38s；该值按软件自身计时口径记录。未采集CPU/GPU/RAM峰值，不补写推测值。
* 导出目录及文件检查：E:\sfm\data\trial02\result；trial02.db完整；二进制/文本COLMAP模型、Log.txt、match_matrix.bmp、images_info.txt、model_info.txt齐全；自动检查PASS、退出0。
* 失败/限制及处理：本轮未出现导致重建中止的错误；最终求解日志中的像素cost仅作为优化收敛记录，尚未与真值比较，不能据此宣称绝对位姿或几何精度。
* 使用的源码提交和本机改动：上游基线e2f511042edb93db4b0c375527ea7ecc6d0ef794；已验证本机配置提交f3d023b30e34164b840d8534fde05b9fa1988ec7；local/windows-setup分支；九个vcxproj、两个本机属性表及local.glog-compat.h已提交，算法C++源文件未改；最终工作区干净。
