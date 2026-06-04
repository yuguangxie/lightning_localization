# lightning_localization

`lightning_localization` 是从 `lightning-lm` 定位运行时剥离出来的独立 ROS2 包。阶段一目标聚焦于保持行为一致的剥离：在线定位、离线 rosbag 定位、LIO 预测、NDT-OMP scan-to-map 匹配、分块地图、PGO 平滑、TF 输出、YAML 配置、Pangolin UI 路径以及 Livox 自定义消息。

## 范围

包含的运行入口：

- `run_loc_online`
- `run_loc_offline`

包含的定位链路：

- `LocSystem`
- `loc::Localization`
- `LidarLoc`
- `PointCloudPreprocess`
- 作为定位 LIO 前端的 `LaserMapping`
- `TiledMap`
- NDT-OMP
- `PGO` 和高频外推
- `LocalizationResult`
- `map -> base_link` TF 输出
- 定位使用的 Pangolin UI 代码路径
- 用于离线回放的 `RosbagIO`
- 通过内置 `livox_ros_driver2` 消息子包提供的 Livox `CustomMsg` 和 `CustomPoint` 接口

排除的运行入口：

- SLAM 在线/离线应用
- 仅前端和回环应用
- `SlamSystem` 与保存地图 service 运行时接线

原始 `lightning-lm/` 目录没有被修改或删除。

## 当前 Codex 环境

该包是在 Windows 工作区中剥离完成的，当前环境没有 ROS2、没有 colcon，也没有完整 ROS2 bag/map 数据。本阶段已完成源码迁移、构建文件重组、静态审计、文档和人工验证流程设计。

ROS2 编译、ROS2 节点运行、rosbag 回放、RViz 检查和 Pangolin 运行时检查均待在真实 Ubuntu/ROS2 环境中人工验证。

## 在 ROS2 中构建

在 Ubuntu、ROS2 Humble 或更新版本中，按 `docs/manual-ros2-validation.md` 所列依赖准备环境后执行：

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-select lightning_localization
source install/setup.bash
```

## 运行

在线：

```bash
ros2 run lightning_localization run_loc_online --config ./config/default.yaml
```

离线：

```bash
ros2 run lightning_localization run_loc_offline --config ./config/default.yaml --input_bag /path/to/bag --map_path ./data/new_map/
```

Launch 包装文件位于 `launch/loc_online.launch.py` 和 `launch/loc_offline.launch.py`。

## 文档

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

中文副本采用同名加 `_ZH` 的方式命名，例如 `docs/architecture_ZH.md`。

## 本地静态检查

在当前 Windows 工作区中运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\static_check.ps1
```

同时提供 Python 版本 `scripts/static_check.py`，供 Linux 或具备 Python 的环境使用。

