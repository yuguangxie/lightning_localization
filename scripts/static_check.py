#!/usr/bin/env python3
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]

REQUIRED = [
    "package.xml",
    "CMakeLists.txt",
    "cmake/packages.cmake",
    "src/CMakeLists.txt",
    "src/app/CMakeLists.txt",
    "src/app/run_loc_online.cc",
    "src/app/run_loc_offline.cc",
    "src/core/system/loc_system.h",
    "src/core/system/loc_system.cc",
    "src/core/localization/localization.h",
    "src/core/localization/localization.cpp",
    "src/core/localization/lidar_loc/lidar_loc.h",
    "src/core/localization/lidar_loc/lidar_loc.cc",
    "src/core/localization/lidar_loc/pclomp/ndt_omp.h",
    "src/core/maps/tiled_map.h",
    "src/core/lio/pointcloud_preprocess.h",
    "src/wrapper/bag_io.h",
    "config/default.yaml",
    "launch/loc_online.launch.py",
    "launch/loc_offline.launch.py",
    "thirdparty/livox_ros_driver/msg/CustomMsg.msg",
    "thirdparty/livox_ros_driver/msg/CustomPoint.msg",
    "docs/architecture.md",
    "docs/io-spec.md",
    "docs/config-parameters.md",
    "docs/manual-ros2-validation.md",
    "docs/stage-one-acceptance-checklist.md",
    "docs/stage-one-static-audit-report.md",
    "docs/stage-one-final-report.md",
]

FORBIDDEN_RUNTIME_PATTERNS = [
    "run_slam_online",
    "run_slam_offline",
    "run_frontend_offline",
    "run_loop_offline",
    "SlamSystem",
    "SaveMapService",
    "scrubber_common",
    "agibot_robot",
]

SOURCE_NOTE_MARKERS = ["TODO"]


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def main() -> int:
    failures = []
    warnings = []

    for rel in REQUIRED:
        if not (ROOT / rel).exists():
            failures.append(f"missing required file: {rel}")

    searchable = [
        ROOT / "package.xml",
        ROOT / "CMakeLists.txt",
        ROOT / "src" / "CMakeLists.txt",
        ROOT / "src" / "app" / "CMakeLists.txt",
    ]

    for path in searchable:
        if not path.exists():
            continue
        text = read_text(path)
        for pattern in FORBIDDEN_RUNTIME_PATTERNS:
            if pattern in text:
                failures.append(f"forbidden runtime pattern in {path.relative_to(ROOT)}: {pattern}")

    for path in ROOT.rglob("*"):
        if path.is_file() and path.suffix in {".cc", ".h", ".hpp", ".md"}:
            text = read_text(path)
            for marker in SOURCE_NOTE_MARKERS:
                if marker in text:
                    warnings.append(f"inherited source note marker in {path.relative_to(ROOT)}: {marker}")
                    break

    if warnings:
        print("Warnings:")
        for item in warnings:
            print(f"  - {item}")

    if failures:
        print("Failures:")
        for item in failures:
            print(f"  - {item}")
        return 1

    print("Static package check passed. ROS2 build and runtime remain pending manual validation.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
