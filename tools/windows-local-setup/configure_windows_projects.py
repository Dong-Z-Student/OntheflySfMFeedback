"""Back up and attach local Release/x64 property sheets to the nine known projects.

Default mode previews the change. --apply writes only the two property sheets and
nine .vcxproj files plus a local glog compatibility header; it does not install
dependencies, build, or change algorithm sources.
"""
import argparse
from pathlib import Path
import shutil
import xml.etree.ElementTree as ET

PROJECTS = (
    "thirdparty/SIFTGPU/SIFTGPU.vcxproj", "thirdparty/VLFeat/VLFeat.vcxproj",
    "src/Base/Base.vcxproj", "src/Geometry/Geometry.vcxproj",
    "src/Scene/Scene.vcxproj", "src/Estimator/Estimator.vcxproj",
    "src/Feature/Feature.vcxproj", "src/Workflow/Workflow.vcxproj", "src/UI/UI.vcxproj",
)
PROPS = ("local.windows.early.props", "local.windows.late.props")
SUPPORT_FILES = PROPS + ("local.glog-compat.h",)


def attach(text, anchor, name):
    marker = f'Label="LocalWindowsSetup-{name}"'
    if marker in text:
        return text
    if text.count(anchor) != 1:
        raise ValueError(f"Expected one import anchor for {name}")
    newline = "\r\n" if "\r\n" in text else "\n"
    local_import = (
        '<Import Project="$(MSBuildProjectDirectory)\\..\\..\\local.windows.' + name
        + '.props" Condition="\'$(Configuration)|$(Platform)\'==\'Release|x64\'" '
        + marker + ' />'
    )
    return text.replace(anchor, local_import + newline + "  " + anchor)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--backup-dir", type=Path, required=True)
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    repo, backup = args.repo.resolve(), args.backup_dir.resolve()
    helper = Path(__file__).resolve().parent
    if backup == repo or repo in backup.parents:
        raise ValueError("Keep the backup outside the repository")
    planned = []
    for filename in SUPPORT_FILES:
        source, destination = helper / filename, repo / filename
        payload = source.read_bytes()
        if filename.endswith(".props"):
            ET.fromstring(payload)
        if destination.exists() and destination.read_bytes() != payload:
            raise ValueError(f"Existing property sheet differs: {destination}; review before overwriting")
        planned.append((destination, payload))
    for relative in PROJECTS:
        path = repo / relative
        original = path.read_bytes()
        bom = original.startswith(b"\xef\xbb\xbf")
        text = original.decode("utf-8-sig")
        text = attach(text, '<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />', "early")
        text = attach(text, '<Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />', "late")
        if relative.startswith("thirdparty/SIFTGPU/"):
            text = text.replace('"C:/vcpkg/installed/x64-windows/include"',
                                '"E:/sfm/OntheflySfMFeedback/vcpkg_installed/x64-windows/include"')
            text = text.replace('"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.2/include"',
                                '"D:/CUDA/v12.2/include"')
        ET.fromstring(text)
        payload = text.encode("utf-8-sig" if bom else "utf-8")
        planned.append((path, payload))
        print(f"[READY] {relative}: early/late Release-x64 imports")
    if not args.apply:
        print("Preview only. Add --apply after reviewing the two property sheets.")
        return 0
    # Preserve the state immediately before S11-B; never overwrite an earlier backup.
    for path, _ in planned:
        if path.exists():
            destination = backup / path.relative_to(repo)
            destination.parent.mkdir(parents=True, exist_ok=True)
            if not destination.exists():
                shutil.copy2(path, destination)
    for path, payload in planned:
        path.write_bytes(payload)
    print("Result: PASS; configured 9 projects and 2 local property sheets.")
    print("Local glog compatibility header is included in the setup.")
    print("Backup:", backup)
    print("Build has NOT been executed; evaluate properties before compiling.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
