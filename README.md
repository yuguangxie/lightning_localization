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

## Documentation

- `docs/architecture.md`
- `docs/io-spec.md`
- `docs/config-parameters.md`
- `docs/input-output-spec.md`
- `docs/configuration-parameters.md`
- `docs/localization-dependency-graph.md`
- `docs/file-migration-table.md`
- `docs/manual-ros2-validation.md`
- `docs/stage-one-acceptance-checklist.md`
- `docs/stage-one-static-audit-report.md`
- `docs/risk-register.md`
- `docs/stage-one-final-report.md`

## Local Static Check

In the current Windows workspace, run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\static_check.ps1
```

The Python version, `scripts/static_check.py`, is also provided for Linux or Python-enabled environments.
