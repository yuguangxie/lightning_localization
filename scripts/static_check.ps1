$ErrorActionPreference = "Stop"

$Root = Resolve-Path (Join-Path $PSScriptRoot "..")

$Required = @(
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
    "docs/stage-one-final-report.md"
)

$ForbiddenRuntimePatterns = @(
    "run_slam_online",
    "run_slam_offline",
    "run_frontend_offline",
    "run_loop_offline",
    "SlamSystem",
    "SaveMapService",
    "scrubber_common",
    "agibot_robot"
)

$Failures = New-Object System.Collections.Generic.List[string]
$Warnings = New-Object System.Collections.Generic.List[string]

foreach ($Rel in $Required) {
    if (-not (Test-Path (Join-Path $Root $Rel))) {
        $Failures.Add("missing required file: $Rel")
    }
}

$Searchable = @(
    "package.xml",
    "CMakeLists.txt",
    "src/CMakeLists.txt",
    "src/app/CMakeLists.txt"
)

foreach ($Rel in $Searchable) {
    $Path = Join-Path $Root $Rel
    if (-not (Test-Path $Path)) {
        continue
    }
    $Text = Get-Content -Raw -Encoding UTF8 $Path
    foreach ($Pattern in $ForbiddenRuntimePatterns) {
        if ($Text.Contains($Pattern)) {
            $Failures.Add("forbidden runtime pattern in ${Rel}: $Pattern")
        }
    }
}

Get-ChildItem -Path $Root -Recurse -File -Include *.cc,*.h,*.hpp,*.md |
    ForEach-Object {
        $Text = Get-Content -Raw -Encoding UTF8 $_.FullName
        if ($Text.Contains("TODO")) {
            $Warnings.Add("inherited source note marker in $($_.FullName.Substring($Root.Path.Length + 1)): TODO")
        }
    }

if ($Warnings.Count -gt 0) {
    Write-Output "Warnings:"
    foreach ($Item in $Warnings) {
        Write-Output "  - $Item"
    }
}

if ($Failures.Count -gt 0) {
    Write-Output "Failures:"
    foreach ($Item in $Failures) {
        Write-Output "  - $Item"
    }
    exit 1
}

Write-Output "Static package check passed. ROS2 build and runtime remain pending manual validation."
