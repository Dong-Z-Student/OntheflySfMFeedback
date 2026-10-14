"""Read-only checks for the default OntheflySfMFeedback pipeline.

Does not install packages, download weights, change source, or build the solution.
Run with the project's Python 3.10 interpreter, not the Anaconda base interpreter.
--model-test loads local weights and extracts a descriptor from --image.
"""
import argparse
import configparser
import importlib
import importlib.metadata
import json
from pathlib import Path
import struct
import sys

WEIGHTS = (
    "vgg16-397923af.pth", "VGG16_64_desc_cen.hdf5",
    "VGG16_NetVlad_NoSplit.pth.tar", "PCA_dims32768to2048.model",
)
EXPECTED = {
    "torch": "2.2.2", "torchvision": "0.17.2", "torchaudio": "2.2.2",
    "numpy": "1.26.4", "scipy": "1.12.0", "scikit-learn": "1.4.2",
    "h5py": "3.10.0", "Pillow": "10.2.0", "joblib": "1.3.2",
}
KEYS = {
    "SfM": ("useFocalLength", "useFactor", "cameraModel"),
    "Pipeline": ("AutoRunImages", "AutoExportModel", "AutoExportFrame", "InitialImagesNum"),
    "Python": ("PythonHome", "PythonWorkspace", "GlobalFeatureModel"),
    "Feedback": ("StartCreateMesh", "Step"),
    "Test": ("ModelFolder", "Start", "Step", "End"),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, required=True)
    parser.add_argument("--ini", type=Path, required=True)
    parser.add_argument("--model-test", action="store_true")
    parser.add_argument("--image", type=Path)
    args = parser.parse_args()
    failures = []

    def check(ok, message):
        print(("[OK] " if ok else "[FAIL] ") + message)
        if not ok:
            failures.append(message)

    print("Interpreter:", sys.executable)
    check(sys.version_info[:2] == (3, 10), "Python must be 3.10 for python310.lib")
    check(struct.calcsize("P") == 8, "64-bit interpreter")
    repo = args.repo.resolve()
    check((repo / "OntheFlySfM.sln").is_file(), "Solution exists")
    manifest = repo / "vcpkg.json"
    check(manifest.is_file(), "vcpkg manifest exists")
    if manifest.is_file():
        print("Manifest baseline:", json.loads(manifest.read_text(encoding="utf-8"))["builtin-baseline"])
    prefix = Path(sys.prefix)
    for part in ("include/Python.h", "libs/python310.lib", "python310.dll"):
        check((prefix / part).is_file(), str(prefix / part))

    for name, version in EXPECTED.items():
        try:
            actual = importlib.metadata.version(name)
            check(actual.split("+")[0] == version, f"{name}: {actual} (reference {version})")
        except importlib.metadata.PackageNotFoundError:
            check(False, f"Missing distribution: {name}")

    torch_module = None
    for name in ("numpy", "scipy", "sklearn", "h5py", "PIL", "joblib", "torch", "torchvision"):
        try:
            module = importlib.import_module(name)
            check(True, f"import {name}")
            if name == "numpy":
                print("NumPy include directory:", module.get_include())
            if name == "torch":
                torch_module = module
        except Exception as exc:
            check(False, f"import {name}: {exc}")
    if torch_module is not None:
        cuda_ok = torch_module.cuda.is_available()
        check(cuda_ok, "torch.cuda.is_available()")
        print("Torch CUDA runtime:", torch_module.version.cuda)
        if cuda_ok:
            try:
                print("GPU:", torch_module.cuda.get_device_name(0))
                result = (torch_module.ones((8, 8), device="cuda") @
                          torch_module.ones((8, 8), device="cuda")).sum().item()
                check(result == 512, "CUDA matrix multiplication")
            except Exception as exc:
                check(False, f"CUDA execution: {exc}")

    check(args.ini.is_file(), "INI exists")
    if not args.ini.is_file():
        return 1
    cfg = configparser.ConfigParser(interpolation=None)
    cfg.optionxform = str
    cfg.read(args.ini, encoding="utf-8-sig")
    for section, keys in KEYS.items():
        for key in keys:
            check(cfg.has_option(section, key), f"INI {section}.{key}")
    if not all(cfg.has_option("Python", key) for key in KEYS["Python"]):
        return 1
    home = Path(cfg["Python"]["PythonHome"]).resolve()
    workspace = Path(cfg["Python"]["PythonWorkspace"]).resolve()
    models = Path(cfg["Python"]["GlobalFeatureModel"]).resolve()
    check(home == prefix.resolve(), "INI PythonHome matches current environment")
    for name in ("GlobalFeatureExtractor.py", "NetVlad.py"):
        check((workspace / name).is_file(), str(workspace / name))
    for name in WEIGHTS:
        path = models / name
        check(path.is_file() and path.stat().st_size > 0, str(path))
    try:
        start, step = cfg.getint("Feedback", "StartCreateMesh"), cfg.getint("Feedback", "Step")
        check(start >= 0 and step > 0, "Feedback start >= 0 and Step > 0")
        print("First feedback scheduling threshold:", start + step, "registered images")
    except (ValueError, KeyError, configparser.Error) as exc:
        check(False, f"Feedback values: {exc}")

    if args.model_test:
        if not args.image or not args.image.is_file():
            check(False, "--model-test requires an existing --image")
        elif failures:
            print("[SKIP] Model test: fix the failures above first.")
        else:
            try:
                sys.path.insert(0, str(workspace))
                from GlobalFeatureExtractor import GlobalFeatureExtractor
                print("Loading local NetVLAD/PCA models...", flush=True)
                extractor = GlobalFeatureExtractor(*(str(models / name) for name in WEIGHTS))
                print("Models loaded; extracting the test image...", flush=True)
                feature = extractor.Extract(str(args.image.resolve()))
                check(tuple(feature.shape) == (1, 2048), f"NetVLAD descriptor shape: {feature.shape}")
                numpy_module = importlib.import_module("numpy")
                feature_array = numpy_module.asarray(feature)
                check(bool(numpy_module.isfinite(feature_array).all()),
                      "NetVLAD descriptor contains only finite values")
                feature_norm = float(numpy_module.linalg.norm(feature_array))
                check(bool(numpy_module.isfinite(feature_norm)) and feature_norm > 0,
                      f"NetVLAD descriptor norm: {feature_norm}")
            except Exception as exc:
                check(False, f"Model initialization/extraction: {exc}")
    print("\nResult:", "FAIL" if failures else "PASS")
    print("This does not verify MSVC, Qt, TBB, C++ linking, embedded Python, or SfM accuracy.")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
