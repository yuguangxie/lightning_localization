# Stage One Static Audit Report

## Environment

Audit was performed in a Windows workspace without ROS2, without colcon, and without complete ROS2 bag/map data. ROS2 build and runtime validation were not executed locally.

## Completed Static Checks

| Item | Result | Evidence |
|---|---|---|
| Standalone package directory | pass | filesystem directory `lightning_localization_v1/` exists; ROS package name is `lightning_localization` |
| Online entry | pass | `src/app/run_loc_online.cc` |
| Offline entry | pass | `src/app/run_loc_offline.cc` |
| ROS2 package manifest | pass | `package.xml` |
| CMake root | pass | `CMakeLists.txt` |
| Source build target | pass | `src/CMakeLists.txt` |
| App build target | pass | `src/app/CMakeLists.txt` |
| Config migration | pass | `config/default*.yaml` |
| Launch wrappers | pass | `launch/loc_online.launch.py`, `launch/loc_offline.launch.py` |
| Livox messages | pass | `thirdparty/livox_ros_driver/msg/*.msg` |
| Upstream project preserved | pass | original `lightning-lm/` files were not edited or deleted |
| SLAM runtime excluded | pass | no `run_slam_*`, `SlamSystem`, or save-map runtime in new build target |
| Private upstream deps removed | pass | no `scrubber_common` or `agibot_robot` in new manifest |
| Local PowerShell static check | pass | `scripts/static_check.ps1` completed with inherited-source warnings only |
| Local quoted include closure | pass | local quoted includes resolve inside the package or to declared external/ROS dependencies |

## Dependency Closure

The static dependency closure contains online/offline entry points, `LocSystem`, `Localization`, `LidarLoc`, NDT-OMP, `TiledMap`, LIO, PGO, `miao`, common types, YAML/file IO, wrappers, UI, Sophus, and Livox messages. See `docs/localization-dependency-graph.md`.

The referenced prompt file `plans/stage-one-extraction-plan.md` was missing in the initial workspace snapshot and has been created from the master prompt, agents, skills, validation procedure, and current package state.

## CMake And Package Audit

The new CMake structure declares:

- C++17
- ROS2 interface generation for copied services
- bundled Livox message subpackage
- localization shared library
- `run_loc_online`
- `run_loc_offline`
- config, launch, docs, and scripts install rules

The manifest declares ROS2, TF, rosbag2, PCL conversion, and visualization dependencies without upstream private dependencies.

## Config, Topic, Frame, And Map Audit

The copied YAML files preserve upstream keys. Online mode still reads `system.map_path`, `common.imu_topic`, `common.lidar_topic`, `common.livox_lidar_topic`, and `system.pub_tf`. TF output remains `map -> base_link` through `LocalizationResult::ToGeoMsg()`.

## Inherited Source Notes

The extracted code preserves several upstream comments that contain待办-style wording in source and third-party headers. These comments are inherited source annotations, not stage-one delivery placeholders, and they do not replace any required migration, build-file, documentation, or validation artifact.

The local static check reported these as warnings, not failures, because the package migration, build files, documentation, and validation flow do not depend on placeholder delivery.

## Local Tool Limits

The Python static check script is included for Linux or Python-enabled environments, but `python --version` returned no usable output in the current Windows shell. The PowerShell static check was executed successfully and is the local script evidence for this run.

## Not Executed Locally

The following remain pending manual ROS2 environment validation:

- `colcon build --packages-select lightning_localization`
- `ros2 run lightning_localization run_loc_online`
- `ros2 run lightning_localization run_loc_offline`
- rosbag replay
- RViz TF inspection
- Pangolin runtime inspection
- trajectory metric comparison
