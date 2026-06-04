# Stage One Acceptance Checklist

## Package Structure

| Item | Result | Evidence |
|---|---|---|
| Standalone ROS2 package directory exists | pass | `lightning_localization_v1/`; ROS package name `lightning_localization` |
| `package.xml` exists | pass | `package.xml` |
| `CMakeLists.txt` exists | pass | `CMakeLists.txt` |
| `src/` and `include/` exist | pass | `src/`, `include/README.md` |
| `config/`, `launch/`, `scripts/`, `docs/` exist | pass | `config/`, `launch/`, `scripts/`, `docs/` |
| `msg/` and `srv/` handled | pass | `msg/README.md`, `srv/LocCmd.srv`, `srv/SaveMap.srv`, bundled Livox `msg/*.msg` |

## Localization Chain

| Item | Result | Evidence |
|---|---|---|
| `Localization` migrated | pass | `src/core/localization/localization.*` |
| `LocSystem` migrated | pass | `src/core/system/loc_system.*` |
| `LidarLoc` migrated | pass | `src/core/localization/lidar_loc/lidar_loc.*` |
| Point cloud preprocessing migrated | pass | `src/core/lio/pointcloud_preprocess.*` |
| LIO prediction/odometry closure migrated | pass | `src/core/lio/**`, `src/core/ivox3d/**`, `src/common/**` |
| PGO/high-frequency extrapolation/smoothing migrated | pass | `src/core/localization/pose_graph/**` |
| `TiledMap` and chunk loading migrated | pass | `src/core/maps/tiled_map*`, `src/core/maps/tiled_map_chunk*` |
| NDT-OMP route preserved | pass | `src/core/localization/lidar_loc/pclomp/**`; no stage-two backend refactor |
| Dynamic map layer path preserved | pass | `src/core/maps/**`, `src/core/localization/lidar_loc/lidar_loc.*` |
| Online localization entry exists | pass | `src/app/run_loc_online.cc` |
| Offline rosbag entry exists | pass | `src/app/run_loc_offline.cc`, `src/wrapper/bag_io.*` |
| TF output preserved | pass | `src/core/system/loc_system.cc`, `src/core/localization/localization_result.*` |
| Pangolin/RViz paths migrated or documented | pass | `src/ui/**`, `docs/io-spec.md`, `docs/manual-ros2-validation.md` |
| YAML config reading preserved | pass | `src/io/yaml_io.*`, `config/default*.yaml` |

## Build And Configuration Static Audit

| Item | Result | Evidence |
|---|---|---|
| ROS2/PCL/tf2/rosbag2/pcl_conversions dependencies declared | pass | `package.xml`, `cmake/packages.cmake` |
| CMake includes C++17, targets, dependencies, srv generation, install rules | pass | `CMakeLists.txt`, `src/CMakeLists.txt`, `src/app/CMakeLists.txt` |
| Topic names statically audited | pass | `docs/io-spec.md`, `docs/stage-one-static-audit-report.md` |
| Frame names statically audited | pass | `map -> base_link` in `docs/io-spec.md` and source hash comparison |
| Map path statically audited | pass | `system.map_path`, `--map_path`, `docs/io-spec.md` |
| Config path statically audited | pass | `config/default*.yaml`, launch files |

## Documentation And Reports

| Item | Result | Evidence |
|---|---|---|
| README exists | pass | `README.md` |
| Architecture document exists | pass | `docs/architecture.md` |
| I/O specification exists | pass | `docs/io-spec.md`, `docs/input-output-spec.md` |
| Configuration parameter document exists | pass | `docs/config-parameters.md`, `docs/configuration-parameters.md` |
| Manual ROS2 validation document exists | pass | `docs/manual-ros2-validation.md` |
| Static audit report exists | pass | `docs/stage-one-static-audit-report.md` |
| Risk register exists | pass | `docs/risk-register.md` |
| Final report exists | pass | `docs/stage-one-final-report.md` |

## Prohibited-Item Checks

| Prohibited item | Result | Evidence |
|---|---|---|
| Original `lightning-lm/` files were not deleted | pass | source tree still present |
| No stage-two industrial backend refactor introduced | pass | NDT-OMP remains default and only matcher in stage-one build |
| Matching algorithm not changed first | pass | source hash comparison for `LidarLoc` and NDT-OMP path |
| Localization behavior not rewritten into another system | pass | source hash comparison for core runtime files |
| No delivery-critical placeholder comments | pass | inherited upstream source-note comments are documented warnings only |
| No stage-one goal deferral | pass | static deliverables and manual validation flow are present |
| No false ROS2/colcon runtime success claim | pass | docs explicitly mark build/run/bag/RViz/Pangolin as pending manual ROS2 validation |

## Pending Manual ROS2 Environment Validation

- `colcon build --packages-select lightning_localization`
- `ros2 run lightning_localization run_loc_online`
- `ros2 run lightning_localization run_loc_offline`
- rosbag replay with the same config/map/bag as upstream
- RViz TF inspection for `map -> base_link`
- Pangolin runtime inspection
- trajectory, confidence, status, and timing comparison against upstream lightning-lm
