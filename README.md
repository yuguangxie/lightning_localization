# lightning_localization

`lightning_localization` is a standalone ROS2 package extracted from the `lightning-lm` localization runtime. It keeps the stage-one goal focused on behavior-preserving extraction: online localization, offline rosbag localization, LIO prediction, NDT-OMP scan-to-map matching, tiled maps, PGO smoothing, TF output, YAML configuration, Pangolin UI hooks, and Livox custom messages.

## Scope

Included runtime entry points:

- `run_loc_online`
- `run_loc_offline`

Included localization chain:

- `LocSystem`
- `loc::Localization`
- `LidarLoc`
- `PointCloudPreprocess`
- `LaserMapping` as the localization LIO frontend
- `TiledMap`
- NDT-OMP
- `PGO` and high-frequency extrapolation
- `LocalizationResult`
- `map -> base_link` TF output
- `/localization/pose` pose topic
- `/localization/status` status topic
- `/localization/diagnostics` diagnostics topic
- optional `/localization/odometry` odometry topic, disabled by default
- RViz2 visualization config in `rviz/lightning_localization.rviz`
- RViz2 `/initialpose` initial pose injection
- `/localization/initialization_status` initialization status topic
- `/lightning_localization/set_initial_pose` external initial pose service
- Pangolin UI code paths used by localization
- `RosbagIO` for offline replay
- Livox `CustomMsg` and `CustomPoint` interfaces through the bundled `livox_ros_driver2` message subpackage

Excluded runtime entry points:

- SLAM online/offline applications
- frontend-only and loop-closing applications
- `SlamSystem` and save-map service wiring

The original `lightning-lm/` directory is not modified or deleted.

## Current Codex Environment

This package was extracted in a Windows workspace without ROS2, without colcon, and without complete ROS2 bag/map data. The work completed here is source migration, build-file restructuring, static audit, documentation, and manual validation design.

ROS2 compilation, ROS2 node execution, rosbag replay, RViz inspection, and Pangolin runtime inspection are pending manual validation in a real Ubuntu/ROS2 environment.

## Build In ROS2

Use these commands in Ubuntu with ROS2 Humble or newer and the dependencies listed in `docs/manual-ros2-validation.md`:

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-select lightning_localization
source install/setup.bash
```

## Run

Online:

```bash
ros2 run lightning_localization run_loc_online --config ./config/default.yaml
```

Offline:

```bash
ros2 run lightning_localization run_loc_offline --config ./config/default.yaml --input_bag /path/to/bag --map_path ./data/new_map/
```

Launch wrappers are provided in `launch/loc_online.launch.py` and `launch/loc_offline.launch.py`.

Online with RViz2:

```bash
ros2 launch lightning_localization loc_online.launch.py \
  config:=/path/to/default.yaml \
  use_rviz:=true
```

`use_rviz:=true` starts `rviz2 -d` with `rviz/lightning_localization.rviz`. The RViz2 config keeps the left Displays panel and toolbar visible, shows TF and `/localization/pose` by default, and includes the standard `2D Pose Estimate` tool publishing `/initialpose`.

## ROS2 Outputs

Online mode publishes the original `map -> base_link` TF when `system.pub_tf=true`. It also publishes standard ROS2 topics configured under `ros_output`:

- `/localization/pose` as `geometry_msgs/msg/PoseStamped`
- `/localization/status` as `std_msgs/msg/String`
- `/localization/diagnostics` as `diagnostic_msgs/msg/DiagnosticArray`
- `/localization/odometry` as `nav_msgs/msg/Odometry` when explicitly enabled
- `/localization/initialization_status` as `std_msgs/msg/String`

Pose is enabled by default for valid localization results. Status and diagnostics are enabled by default for every localization result so initializing, degraded, failed, or invalid states remain visible. Odometry is disabled by default because twist is not estimated and covariance is a static config placeholder. See `docs/ros2-output-topics.md`.

Initial pose can be injected through:

- RViz2 `2D Pose Estimate` on `/initialpose`
- service `/lightning_localization/set_initial_pose` with type `lightning_localization/srv/SetInitialPose`

Both paths apply the pose only as an initial guess. Localization success must still be verified through `/localization/status`, `/localization/diagnostics`, TF, and later matching output.

Offline mode constructs `loc::Localization` directly and does not spin the `LocSystem` publisher node, so the new ROS2 topic outputs apply to online mode unless the offline entry is refactored in a later validated ROS2 environment.

## Documentation

- `docs/architecture.md`
- `docs/io-spec.md`
- `docs/config-parameters.md`
- `docs/ros2-topic-publication-audit.md`
- `docs/ros2-output-topics.md`
- `docs/input-output-spec.md`
- `docs/configuration-parameters.md`
- `docs/localization-dependency-graph.md`
- `docs/file-migration-table.md`
- `docs/manual-ros2-validation.md`
- `docs/rviz2-and-initialization-source-audit.md`
- `docs/rviz2-visualization-and-initialization.md`
- `docs/initialization-source-design.md`
- `docs/rviz2-manual-validation.md`
- `docs/rviz2-initialization-enhancement-report.md`
- `docs/stage-one-acceptance-checklist.md`
- `docs/stage-one-static-audit-report.md`
- `docs/stage-one-topic-output-enhancement-report.md`
- `docs/risk-register.md`
- `docs/stage-one-final-report.md`

## Local Static Check

In the current Windows workspace, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\static_check.ps1
```

The Python version, `scripts/static_check.py`, is also provided for Linux or Python-enabled environments.

