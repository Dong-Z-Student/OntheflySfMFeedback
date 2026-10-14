param(
    [string]$OutputDir = 'E:\sfm\setup\S16-freeze-20261014'
)

$ErrorActionPreference = 'Stop'

$taskRepo = 'E:\sfm\OntheflySfMFeedback'
$taskPythonEnv = 'E:\sfm\envs\feedback310'
$taskPython = Join-Path $taskPythonEnv 'python.exe'
$taskConda = 'D:\Anaconda\Scripts\conda.exe'
$taskVcpkg = 'E:\sfm\vcpkg\vcpkg.exe'
$taskVcpkgInstalled = Join-Path $taskRepo 'vcpkg_installed'
$taskQmake = 'E:\Qt\5.12.12\msvc2017_64\bin\qmake.exe'
$taskNvcc = 'D:\CUDA\v12.2\bin\nvcc.exe'
$taskVswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
$taskResult = 'E:\sfm\data\trial02\result'
$taskModels = 'E:\sfm\models\netvlad'
$taskUtf8 = New-Object System.Text.UTF8Encoding($false)

function Write-Utf8Lines {
    param([string]$Path, [object[]]$Lines)
    $taskTextLines = @($Lines | ForEach-Object { [string]$_ })
    [IO.File]::WriteAllLines($Path, [string[]]$taskTextLines, $taskUtf8)
}

function Invoke-Capture {
    param([string]$Name, [string]$Path, [scriptblock]$Command)
    # Keep stderr on the console so warnings cannot corrupt YAML/patch/text outputs.
    $taskOutput = & $Command
    $taskCode = $LASTEXITCODE
    Write-Utf8Lines -Path $Path -Lines $taskOutput
    if ($taskCode -ne 0) {
        throw "$Name failed with exit code $taskCode. See $Path"
    }
    Write-Host "[OK] $Name"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$taskConfigDir = Join-Path $OutputDir 'config'
$taskSourceDir = Join-Path $OutputDir 'source-local-files'
New-Item -ItemType Directory -Force -Path $taskConfigDir,$taskSourceDir | Out-Null

foreach ($taskRequired in @($taskRepo,$taskPython,$taskConda,$taskVcpkg,$taskQmake,$taskNvcc,$taskVswhere,$taskResult,$taskModels)) {
    if (-not (Test-Path -LiteralPath $taskRequired)) {
        throw "Required path missing: $taskRequired"
    }
}

Invoke-Capture 'pip freeze' (Join-Path $OutputDir 'requirements-tested.txt') {
    & $taskPython -m pip freeze
}
Invoke-Capture 'pip check' (Join-Path $OutputDir 'pip-check.txt') {
    & $taskPython -m pip check
}
Invoke-Capture 'conda environment export' (Join-Path $OutputDir 'environment-tested.yml') {
    & $taskConda env export --prefix $taskPythonEnv
}
$taskVcpkgListPath = Join-Path $OutputDir 'vcpkg-installed.txt'
Invoke-Capture 'vcpkg installed list' $taskVcpkgListPath {
    & $taskVcpkg list "--x-install-root=$taskVcpkgInstalled"
}
$taskVcpkgEntryCount = @(
    Get-Content -LiteralPath $taskVcpkgListPath |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
).Count
if ($taskVcpkgEntryCount -le 0) {
    throw "vcpkg list returned no installed entries. See $taskVcpkgListPath"
}
Write-Host "[OK] vcpkg entries: $taskVcpkgEntryCount"

Invoke-Capture 'Git HEAD' (Join-Path $OutputDir 'source-commit.txt') {
    git -C $taskRepo rev-parse HEAD
}
Invoke-Capture 'Git branch' (Join-Path $OutputDir 'source-branch.txt') {
    git -C $taskRepo branch --show-current
}
Invoke-Capture 'Git remotes' (Join-Path $OutputDir 'source-remotes.txt') {
    git -C $taskRepo remote -v
}
Invoke-Capture 'Git status' (Join-Path $OutputDir 'source-status.txt') {
    git -C $taskRepo status --short
}
Invoke-Capture 'Git diff stat' (Join-Path $OutputDir 'source-diff-stat.txt') {
    git -C $taskRepo diff --stat
}
Invoke-Capture 'Git tracked patch' (Join-Path $OutputDir 'source-tracked-changes.patch') {
    git -C $taskRepo diff --binary
}
Invoke-Capture 'Git HEAD summary' (Join-Path $OutputDir 'source-head-summary.txt') {
    git -C $taskRepo log -1 --decorate --stat --oneline
}
Invoke-Capture 'Git HEAD patch' (Join-Path $OutputDir 'source-head.patch') {
    git -C $taskRepo show --binary --format=fuller HEAD
}

$taskVersions = [ordered]@{
    captured_at = (Get-Date).ToString('o')
    os = (Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,BuildNumber)
    git = ((git --version) -join "`n")
    visual_studio_2022_installation = ((& $taskVswhere -products '*' -version '[17.0,18.0)' -property installationVersion) -join "`n")
    visual_studio_latest_detected = ((& $taskVswhere -latest -products '*' -property installationVersion) -join "`n")
    msvc_directories = @(Get-ChildItem 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC' -Directory | Select-Object -ExpandProperty Name)
    windows_sdk_directories = @(Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\Include' -Directory | Select-Object -ExpandProperty Name)
    qt = ((& $taskQmake -v) -join "`n")
    cuda_toolkit = ((& $taskNvcc --version) -join "`n")
    gpu = ((nvidia-smi --query-gpu=name,driver_version --format=csv,noheader) -join "`n")
    python = ((& $taskPython -VV) -join "`n")
    vcpkg = ((& $taskVcpkg version) -join "`n")
    repo = $taskRepo
    result = $taskResult
}
[IO.File]::WriteAllText(
    (Join-Path $OutputDir 'versions.json'),
    ($taskVersions | ConvertTo-Json -Depth 5),
    $taskUtf8)
Write-Host '[OK] tool and OS versions'

$taskCopies = @(
    @{ Source='E:\sfm\runtime\settings.ini'; Destination=(Join-Path $taskConfigDir 'settings.ini') },
    @{ Source=(Join-Path $taskRepo 'vcpkg.json'); Destination=(Join-Path $taskConfigDir 'vcpkg.json') },
    @{ Source=(Join-Path $taskRepo 'local.windows.early.props'); Destination=(Join-Path $taskSourceDir 'local.windows.early.props') },
    @{ Source=(Join-Path $taskRepo 'local.windows.late.props'); Destination=(Join-Path $taskSourceDir 'local.windows.late.props') },
    @{ Source=(Join-Path $taskRepo 'local.glog-compat.h'); Destination=(Join-Path $taskSourceDir 'local.glog-compat.h') },
    @{ Source='E:\sfm\triplets\x64-windows.cmake'; Destination=(Join-Path $taskConfigDir 'x64-windows.cmake') },
    @{ Source='E:\sfm\setup\S15-trial02-export.json'; Destination=(Join-Path $OutputDir 'S15-trial02-export.json') }
)
foreach ($taskCopy in $taskCopies) {
    if (-not (Test-Path -LiteralPath $taskCopy.Source)) {
        throw "Snapshot source missing: $($taskCopy.Source)"
    }
    Copy-Item -LiteralPath $taskCopy.Source -Destination $taskCopy.Destination -Force
}
if (Test-Path -LiteralPath 'E:\sfm\overlay-ports\gmp') {
    $taskOverlayDest = Join-Path $taskConfigDir 'overlay-ports\gmp'
    New-Item -ItemType Directory -Force -Path $taskOverlayDest | Out-Null
    Copy-Item -Path 'E:\sfm\overlay-ports\gmp\*' -Destination $taskOverlayDest -Recurse -Force
}
Write-Host '[OK] configuration and local source files copied'

$taskHashRoots = @($taskModels,$taskResult,$taskConfigDir,$taskSourceDir)
$taskHashRows = foreach ($taskHashRoot in $taskHashRoots) {
    Get-ChildItem -LiteralPath $taskHashRoot -Recurse -File | Sort-Object FullName | ForEach-Object {
        $taskHash = Get-FileHash -Algorithm SHA256 -LiteralPath $_.FullName
        [pscustomobject]@{
            Path = $_.FullName
            Length = $_.Length
            SHA256 = $taskHash.Hash
        }
    }
}
$taskHashRows | Export-Csv -LiteralPath (Join-Path $OutputDir 'file-hashes.csv') -NoTypeInformation -Encoding UTF8
Write-Host "[OK] file hashes: $($taskHashRows.Count) files"

$taskSourceStatusCount = @(
    Get-Content -LiteralPath (Join-Path $OutputDir 'source-status.txt') |
        Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
).Count
$taskReport = [ordered]@{
    output_directory = $OutputDir
    source_commit = ((git -C $taskRepo rev-parse HEAD) -join '').Trim()
    source_branch = ((git -C $taskRepo branch --show-current) -join '').Trim()
    requirements_file = (Join-Path $OutputDir 'requirements-tested.txt')
    conda_environment_file = (Join-Path $OutputDir 'environment-tested.yml')
    vcpkg_file = (Join-Path $OutputDir 'vcpkg-installed.txt')
    vcpkg_entry_count = $taskVcpkgEntryCount
    versions_file = (Join-Path $OutputDir 'versions.json')
    hash_file = (Join-Path $OutputDir 'file-hashes.csv')
    hashed_file_count = $taskHashRows.Count
    source_status_entry_count = $taskSourceStatusCount
    source_worktree_clean = ($taskSourceStatusCount -eq 0)
    passed = $true
}
[IO.File]::WriteAllText(
    (Join-Path $OutputDir 'S16-freeze-report.json'),
    ($taskReport | ConvertTo-Json -Depth 4),
    $taskUtf8)

Write-Host "Snapshot: $OutputDir"
Write-Host 'Result: PASS'
exit 0
