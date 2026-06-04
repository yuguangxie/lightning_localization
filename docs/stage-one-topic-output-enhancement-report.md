# Stage-One Topic Output Enhancement Report

## Objective

Enhance the stage-one `lightning_localization` standalone ROS2 localization package with source-audited ROS2 topic outputs: localization pose, localization status, diagnostics, optional odometry, configuration, build dependencies, documentation, static audit, and manual ROS2 validation steps.

## Modified Files

- `src/core/localization/localization.h`
- `src/core/localization/localization.cpp`
- `src/core/system/loc_system.h`
- `src/core/system/loc_system.cc`
- `config/default.yaml`
- `config/default_livox.yaml`
- `config/default_nclt.yaml`
- `config/default_robosense.yaml`
- `config/default_utbm.yaml`
- `config/default_vbr.yaml`
- `package.xml`
- `cmake/packages.cmake`
- `src/CMakeLists.txt`
- `scripts/static_check.ps1`
- `scripts/static_check.py`
- `README.md`
- `docs/config-parameters.md`
- `docs/manual-ros2-validation.md`
- `docs/ros2-topic-publication-audit.md`
- `docs/ros2-topic-publication-audit_ZH.md`
- `docs/ros2-output-topics.md`
- `docs/ros2-output-topics_ZH.md`
- `docs/stage-one-topic-output-enhancement-report.md`
- `docs/stage-one-topic-output-enhancement-report_ZH.md`
- `docs/stage-one-acceptance-checklist.md`
- `docs/stage-one-acceptance-checklist_ZH.md`
- `docs/stage-one-static-audit-report.md`
- `docs/stage-one-static-audit-report_ZH.md`
- `docs/stage-one-final-report.md`
- `docs/stage-one-final-report_ZH.md`
- `docs/risk-register.md`
- `docs/risk-register_ZH.md`
- `../codex-localization-industrialization/plans/stage-one-extraction-plan.md`

The stage-one docs above were updated only to correct the obsolete `lightning_localization_v1/` directory name to the actual `lightning_localization/` directory.

## Added Topics

| Topic | Type | Default | Notes |
|---|---|---|---|
| `/localization/pose` | `geometry_msgs/msg/PoseStamped` | On | Publishes valid localization pose by default. |
| `/localization/status` | `std_msgs/msg/String` | On | Publishes every localization result after throttle; includes status name, valid flags, confidence, timestamp, and health text. |
| `/localization/diagnostics` | `diagnostic_msgs/msg/DiagnosticArray` | On | Publishes every localization result after throttle; includes one `DiagnosticStatus` named `lightning_localization/localization`. |
| `/localization/odometry` | `nav_msgs/msg/Odometry` | Off | Uses pose from localization; twist is not estimated; covariance is static placeholder config. |

The existing `map -> base_link` TF broadcaster remains controlled by `system.pub_tf` and is not replaced by topic output.

## Implementation Summary

- Added `Localization::ResultCallback` so `LocSystem` can receive the full `LocalizationResult` while the existing TF callback remains unchanged.
- Added ROS2 publishers in `LocSystem` for pose, status, diagnostics, and optional odometry.
- Added helpers for status-name conversion, nanosecond-preserving timestamp conversion, `PoseStamped`, status string, `DiagnosticArray`, and `Odometry` message construction.
- Added `ros_output` YAML configuration to all default YAML files.
- Added `builtin_interfaces` and `diagnostic_msgs` to `package.xml`, `cmake/packages.cmake`, and `src/CMakeLists.txt`.
- Did not add custom messages in this enhancement.
- Did not introduce `RegistrationBackend`, GICP/VGICP/small_gicp, or any matcher/backend refactor.

## Static Audit Results

| Check | Result |
|---|---|
| `create_publisher` exists and uses configurable topic names | Pass |
| `PoseStamped` uses configurable `map_frame`, default `map` | Pass |
| Optional `Odometry` uses configurable `map_frame` and `base_frame`, defaults `map` and `base_link` | Pass |
| `DiagnosticArray` includes status, valid, confidence, `lidar_loc_valid`, and related keys | Pass |
| `std_msgs/String` status includes status enum name | Pass |
| Original `system.pub_tf` and `sendTransform` path remains | Pass |
| `package.xml` and CMake include `builtin_interfaces` and `diagnostic_msgs`; existing `nav_msgs` remains | Pass |
| All default YAML files include `ros_output` | Pass |
| Topic docs match code defaults | Pass |
| No ROS2/colcon success claim was added | Pass |
| No original `lightning-lm/` project files were deleted or modified by this enhancement | Pass |

Executed local checks:

- `powershell -NoProfile -ExecutionPolicy Bypass -File .\lightning_localization\scripts\static_check.ps1`: passed, with inherited upstream/third-party source-note markers reported as warnings only.
- Text audit for all `config/*.yaml` `ros_output` keys: passed.
- Text audit for publishers, TF path, message types, and dependencies: passed.
- Python static script was not executed because this Windows environment only exposes the non-functional WindowsApps `python.exe` launcher and has no `py` launcher.

## Not Executed In Current Environment

The current Codex workspace is Windows-based and has no ROS2, no colcon, and no complete ROS2 bag/map data. The following were not executed:

- `colcon build`
- ROS2 node startup
- rosbag replay
- `ros2 topic echo`
- `ros2 topic hz`
- `tf2_echo`
- RViz2 inspection
- Pangolin runtime inspection

## Manual ROS2 Validation Required

Run these checks in Ubuntu 22.04 with ROS2 Humble and colcon:

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-select lightning_localization
source install/setup.bash
ros2 run lightning_localization run_loc_online --config /path/to/default.yaml
ros2 run tf2_ros tf2_echo map base_link
ros2 topic echo /localization/pose
ros2 topic echo /localization/status
ros2 topic echo /localization/diagnostics
ros2 topic hz /localization/pose
ros2 topic hz /localization/status
ros2 topic hz /localization/diagnostics
```

If odometry is enabled:

```bash
ros2 topic echo /localization/odometry
```

Use the same config, map, and bag or live sensor input used for the original stage-one TF validation. In RViz2, inspect TF and pose together. Record confidence, status, diagnostics level, and timing behavior as localization state changes.

## Risks And Notes

- Odometry covariance is a static placeholder when odometry output is enabled; it is not a real estimator covariance.
- Odometry twist is not estimated by this enhancement and remains default zero.
- Status is a simple `std_msgs/String` for stage one; a structured product interface should be designed later before adding a stable custom message.
- Offline `run_loc_offline` still does not publish ROS2 topics because it bypasses `LocSystem`.
- The topic publishing path is source-audited only until ROS2 manual validation is completed.

