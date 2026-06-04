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
    "docs/ros2-topic-publication-audit.md",
    "docs/ros2-output-topics.md",
    "docs/manual-ros2-validation.md",
    "docs/stage-one-acceptance-checklist.md",
    "docs/stage-one-static-audit-report.md",
    "docs/stage-one-topic-output-enhancement-report.md",
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

$RequiredPatterns = @{
    "package.xml" = @("builtin_interfaces", "diagnostic_msgs", "nav_msgs", "geometry_msgs", "std_msgs")
    "cmake/packages.cmake" = @("find_package(builtin_interfaces REQUIRED)", "find_package(diagnostic_msgs REQUIRED)", "find_package(nav_msgs REQUIRED)")
    "src/CMakeLists.txt" = @('"builtin_interfaces"', '"diagnostic_msgs"', '"nav_msgs"')
    "src/core/system/loc_system.h" = @("rclcpp::Publisher<geometry_msgs::msg::PoseStamped>", "rclcpp::Publisher<std_msgs::msg::String>", "rclcpp::Publisher<diagnostic_msgs::msg::DiagnosticArray>", "rclcpp::Publisher<nav_msgs::msg::Odometry>")
    "src/core/system/loc_system.cc" = @("create_publisher<geometry_msgs::msg::PoseStamped>", "create_publisher<std_msgs::msg::String>", "create_publisher<diagnostic_msgs::msg::DiagnosticArray>", "create_publisher<nav_msgs::msg::Odometry>", "PublishLocalizationTopics")
    "config/default.yaml" = @("ros_output:", "publish_pose:", "publish_status:", "publish_diagnostics:", "publish_odometry:")
}

foreach ($Rel in $RequiredPatterns.Keys) {
    $Path = Join-Path $Root $Rel
    if (-not (Test-Path $Path)) {
        continue
    }
    $Text = Get-Content -Raw -Encoding UTF8 $Path
    foreach ($Pattern in $RequiredPatterns[$Rel]) {
        if (-not $Text.Contains($Pattern)) {
            $Failures.Add("missing required pattern in ${Rel}: $Pattern")
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
