#!/usr/bin/env python3
"""Validate one OntheflySfMFeedback Export Project result directory."""

from __future__ import annotations

import argparse
import json
import re
import sqlite3
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--result", type=Path, required=True)
    parser.add_argument("--database-name", default="trial02.db")
    parser.add_argument("--expected-images", type=int, required=True)
    parser.add_argument("--expected-points", type=int, required=True)
    parser.add_argument("--report", type=Path)
    return parser.parse_args()


def read_prefix(path: Path, max_lines: int = 8) -> str:
    with path.open("r", encoding="utf-8", errors="replace") as handle:
        return "".join(handle.readline() for _ in range(max_lines))


def main() -> int:
    args = parse_args()
    root = args.result.resolve()
    model = root / "models" / "1"
    database = root / args.database_name
    required = [
        database,
        root / "Log.txt",
        root / "match_matrix.bmp",
        model / "bin" / "cameras.bin",
        model / "bin" / "images.bin",
        model / "bin" / "points3D.bin",
        model / "text" / "cameras.txt",
        model / "text" / "images.txt",
        model / "text" / "points3D.txt",
        model / "images_info.txt",
        model / "model_info.txt",
    ]

    checks: list[dict[str, object]] = []

    def check(name: str, passed: bool, detail: object) -> None:
        checks.append({"name": name, "passed": passed, "detail": detail})
        print(f"[{'OK' if passed else 'FAIL'}] {name}: {detail}")

    check("result directory", root.is_dir(), str(root))
    for path in required:
        exists = path.is_file()
        size = path.stat().st_size if exists else 0
        check(str(path.relative_to(root)), exists and size > 0, f"{size} bytes")

    images_text = model / "text" / "images.txt"
    if images_text.is_file():
        prefix = read_prefix(images_text)
        match = re.search(r"# Number of images:\s*(\d+)", prefix)
        actual = int(match.group(1)) if match else None
        check("text model registered images", actual == args.expected_images, actual)

    points_text = model / "text" / "points3D.txt"
    if points_text.is_file():
        prefix = read_prefix(points_text)
        match = re.search(r"# Number of points:\s*(\d+)", prefix)
        actual = int(match.group(1)) if match else None
        check("text model 3D points", actual == args.expected_points, actual)

    images_info = model / "images_info.txt"
    if images_info.is_file():
        first_line = read_prefix(images_info, 1).strip()
        match = re.search(
            r"Number of images\s*=\s*(\d+), number of registered images\s*=\s*(\d+)",
            first_line,
        )
        values = tuple(map(int, match.groups())) if match else None
        expected = (args.expected_images, args.expected_images)
        check("debug image counts", values == expected, values)

    if database.is_file() and database.stat().st_size > 0:
        try:
            # immutable=1 prevents SQLite from creating -wal/-shm sidecars during validation.
            with sqlite3.connect(
                f"file:{database.as_posix()}?mode=ro&immutable=1", uri=True
            ) as connection:
                integrity = connection.execute("PRAGMA integrity_check").fetchone()[0]
                check("SQLite integrity", integrity == "ok", integrity)
                table_names = {
                    row[0]
                    for row in connection.execute(
                        "SELECT name FROM sqlite_master WHERE type='table'"
                    )
                }
                for table in ("images", "keypoints", "descriptors"):
                    if table in table_names:
                        count = connection.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0]
                        check(f"database {table} rows", count == args.expected_images, count)
                    else:
                        check(f"database table {table}", False, "missing")
                for table in ("matches", "two_view_geometries"):
                    if table in table_names:
                        count = connection.execute(f"SELECT COUNT(*) FROM {table}").fetchone()[0]
                        check(f"database {table} nonempty", count > 0, count)
                    else:
                        check(f"database table {table}", False, "missing")
        except sqlite3.Error as exc:
            check("open exported SQLite database", False, str(exc))

    passed = all(bool(item["passed"]) for item in checks)
    report = {
        "result_directory": str(root),
        "expected_images": args.expected_images,
        "expected_points": args.expected_points,
        "checks": checks,
        "passed": passed,
    }
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
        print(f"Report: {args.report}")
    print(f"Result: {'PASS' if passed else 'FAIL'}")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
