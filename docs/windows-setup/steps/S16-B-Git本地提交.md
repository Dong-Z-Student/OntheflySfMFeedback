# S16-B：Git本地提交

当前状态：已通过。目的：把已经通过完整构建、运行、SfM、Feedback和导出验证的Windows工程配置保存为一个可回退的本地Git节点。该操作不会上传GitHub。

当前分支local/windows-setup，基线e2f511042edb93db4b0c375527ea7ecc6d0ef794。待提交内容严格限定为9个vcxproj和3个本机配置文件；绝对路径仅适用于本机，因此保留在local/windows-setup，不直接合并或推送到上游主分支。

在PowerShell执行：

```powershell
Set-Location -LiteralPath 'E:\sfm\OntheflySfMFeedback'

$taskSetupFiles = @(
    'src/Base/Base.vcxproj',
    'src/Estimator/Estimator.vcxproj',
    'src/Feature/Feature.vcxproj',
    'src/Geometry/Geometry.vcxproj',
    'src/Scene/Scene.vcxproj',
    'src/UI/UI.vcxproj',
    'src/Workflow/Workflow.vcxproj',
    'thirdparty/SIFTGPU/SIFTGPU.vcxproj',
    'thirdparty/VLFeat/VLFeat.vcxproj',
    'local.glog-compat.h',
    'local.windows.early.props',
    'local.windows.late.props'
)

git add -- $taskSetupFiles
git diff --cached --check
$taskIndexCheckExit = $LASTEXITCODE
git diff --cached --stat
git status --short
$taskIndexCheckExit
```

第一阶段通过标准：`$taskIndexCheckExit`为0；stat显示9个工程文件和3个新增文件；status中的12项都以`M `或`A `开头，表示已进入暂存区。若出现其他文件或检查不为0，不执行commit并返回输出。

检查通过后继续：

```powershell
git commit -m 'chore(windows): record verified local build configuration'
$taskCommitExit = $LASTEXITCODE

git branch --show-current
git log -1 --oneline
git status --short
$taskCommitExit
```

最终通过标准：提交成功；分支仍为local/windows-setup；最新提交信息为上述Windows配置；`git status --short`无输出；`$taskCommitExit`为0。实际提交为f3d023b30e34164b840d8534fde05b9fa1988ec7，12 files changed、115 insertions、7 deletions，工作区干净，B通过。

最终冻结也已完成：2026-10-14 22:52生成的报告记录同一提交、local/windows-setup、工作区干净、vcpkg175项、32个文件哈希、passed=true，`source-head.patch`存在，脚本退出0。S16整体完成。证据见最终冻结报告（本机归档，见相邻步骤说明）。
