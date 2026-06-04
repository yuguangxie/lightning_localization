# Stage One Final Report

## Result

Stage one uses `lightning_localization/` as the standalone ROS2 package directory for the original lightning-lm localization runtime. Its ROS package name is `lightning_localization`. The extraction keeps the default NDT-OMP behavior and does not introduce stage-two industrial matching backend refactors.

## Created Package

Key package files:

- `package.xml`
- `CMakeLists.txt`
- `cmake/packages.cmake`
- `src/CMakeLists.txt`
- `src/app/CMakeLists.txt`
- `launch/loc_online.launch.py`
- `launch/loc_offline.launch.py`

Key runtime entries:

- `src/app/run_loc_online.cc`
- `src/app/run_loc_offline.cc`

Key documents:

- `README.md`
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

## Preserved Behavior

The package preserves:

- `LocSystem`
- `loc::Localization`
- `LidarLoc`
- point cloud preprocessing
- LIO prediction
- PGO and high-frequency extrapolation
- `TiledMap`
- NDT-OMP
- dynamic map layer code path
- online and offline localization entry points
- TF output
- Pangolin UI code path
- YAML configuration reading
- Livox custom message support

## Excluded From Build Target

The new build target excludes SLAM applications, frontend-only applications, loop-closing applications, `SlamSystem`, and save-map runtime wiring. The upstream project remains unchanged.

## Static Audit

Static audit completed for:

- file presence
- migration scope
- CMake/package.xml structure
- topic/frame/map-path preservation
- default NDT-OMP preservation
- ROS2 manual validation procedure
- risk register
- local PowerShell static check
- local quoted include closure check

## Manual Validation Required

ROS2 and colcon were not available in the current Windows workspace, and no complete ROS2 bag/map data was available. Therefore the following are pending manual ROS2 environment validation:

- ROS2 build
- online node execution
- offline rosbag replay
- RViz TF inspection
- Pangolin runtime inspection
- trajectory and timing comparison against upstream lightning-lm

## Completion Criteria

The stage-one source migration, build-file restructuring, documentation, static audit, and manual validation flow are complete. Runtime acceptance must be confirmed in a real ROS2 environment using `docs/manual-ros2-validation.md`.
