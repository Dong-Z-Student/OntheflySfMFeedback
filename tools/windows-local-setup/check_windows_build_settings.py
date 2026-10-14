"""Evaluate Release/x64 properties for nine projects without invoking build targets."""
import argparse
import json
import os
from pathlib import Path
import subprocess

from configure_windows_projects import PROJECTS

MSBUILD = Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe")
PROPERTY_NAMES = (
    "Configuration", "Platform", "PlatformToolset", "VCToolsVersion", "VCToolsInstallDir",
    "WindowsTargetPlatformVersion", "WindowsSDK_IncludePath", "OutDir", "IntDir",
    "VcpkgInstalledDir", "_ZVcpkgCurrentInstalledDir", "VcpkgManifestInstall",
    "UseInteloneTBB", "UseInteloneMKL", "QtMsBuild", "QtInstallDir",
    "CudaToolkitCustomDir", "IncludePath", "LibraryPath",
)


def normalized(path):
    return os.path.normcase(os.path.normpath(path)).rstrip("\\/")


def inspect_properties(name, properties):
    errors = []
    expected = {
        "Configuration": "Release", "Platform": "x64", "PlatformToolset": "v143",
        "VCToolsVersion": "14.36.32532", "WindowsTargetPlatformVersion": "10.0.26100.0",
        "VcpkgManifestInstall": "false", "UseInteloneTBB": "false", "UseInteloneMKL": "No",
    }
    for key, value in expected.items():
        if properties.get(key, "").lower() != value.lower():
            errors.append(f"{key}={properties.get(key)!r}, expected {value!r}")
    paths = {
        "VCToolsInstallDir": r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.36.32532",
        "OutDir": r"E:\sfm\runtime\Release",
        "IntDir": fr"E:\sfm\build\obj\{name}\x64\Release",
        "VcpkgInstalledDir": r"E:\sfm\OntheflySfMFeedback\vcpkg_installed",
        "_ZVcpkgCurrentInstalledDir": r"E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows",
        "CudaToolkitCustomDir": r"D:\CUDA\v12.2",
    }
    if name in ("UI", "Workflow"):
        paths["QtInstallDir"] = r"E:\Qt\5.12.12\msvc2017_64"
        paths["QtMsBuild"] = str(Path(os.environ["LOCALAPPDATA"]) / "QtMsBuild")
    for key, value in paths.items():
        if normalized(properties.get(key, "")) != normalized(value):
            errors.append(f"{key}={properties.get(key)!r}, expected {value!r}")
    if "10.0.26100.0" not in properties.get("WindowsSDK_IncludePath", ""):
        errors.append("WindowsSDK_IncludePath does not select SDK 26100")
    include_dirs = {normalized(p) for p in properties.get("IncludePath", "").split(";") if p}
    required_includes = (
        r"E:\sfm\envs\feedback310\include",
        r"E:\sfm\envs\feedback310\Lib\site-packages\numpy\core\include",
        r"E:\sfm\onetbb\include", r"D:\CUDA\v12.2\include",
        r"E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\include",
        r"E:\sfm\OntheflySfMFeedback\vcpkg_installed\x64-windows\include\eigen3",
    )
    for directory in required_includes:
        if normalized(directory) not in include_dirs:
            errors.append(f"Missing include directory: {directory}")
    library_dirs = {normalized(p) for p in properties.get("LibraryPath", "").split(";") if p}
    for directory in (r"E:\sfm\envs\feedback310\libs", r"E:\sfm\onetbb\lib\intel64\vc14",
                      r"D:\CUDA\v12.2\lib\x64", r"E:\sfm\runtime\Release"):
        if normalized(directory) not in library_dirs:
            errors.append(f"Missing library directory: {directory}")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--report-dir", type=Path, required=True)
    args = parser.parse_args()
    args.report_dir.mkdir(parents=True, exist_ok=True)
    failures = []
    for relative in PROJECTS:
        project = args.repo / relative
        name = project.stem
        command = [str(MSBUILD), str(project), "/nologo", "/p:Configuration=Release", "/p:Platform=x64",
                   "/getProperty:" + ",".join(PROPERTY_NAMES)]
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                encoding="utf-8-sig", errors="replace", timeout=120)
        (args.report_dir / f"{name}-properties.json").write_text(result.stdout, encoding="utf-8")
        if result.returncode:
            print(f"[FAIL] {name}: MSBuild exit {result.returncode}\n{result.stdout}")
            failures.append(name)
            continue
        try:
            data = json.loads(result.stdout)
            errors = inspect_properties(name, data["Properties"])
        except (ValueError, KeyError) as exc:
            errors = [f"Unable to read MSBuild JSON: {exc}"]
        if errors:
            print(f"[FAIL] {name}: " + "; ".join(errors))
            failures.append(name)
        else:
            print(f"[OK] {name}: Release/x64, MSVC 14.36.32532, SDK 26100, paths verified")
    print("Result:", "FAIL" if failures else "PASS (9 projects)")
    print("Reports:", args.report_dir)
    print("No build targets executed; C++ compilation and linking remain to be verified.")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
