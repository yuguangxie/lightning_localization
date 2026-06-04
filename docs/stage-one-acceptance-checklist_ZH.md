# 阶段一验收清单

## 包结构

| 项 | 结果 | 证据 |
|---|---|---|
| 独立 ROS2 package 目录存在 | 通过 | `lightning_localization_v1/`；ROS package 名为 `lightning_localization` |
| `package.xml` 存在 | 通过 | `package.xml` |
| `CMakeLists.txt` 存在 | 通过 | `CMakeLists.txt` |
| `src/` 和 `include/` 存在 | 通过 | `src/`, `include/README.md` |
| `config/`、`launch/`、`scripts/`、`docs/` 存在 | 通过 | `config/`, `launch/`, `scripts/`, `docs/` |
| `msg/` 和 `srv/` 已处理 | 通过 | `msg/README.md`, `srv/LocCmd.srv`, `srv/SaveMap.srv`, 内置 Livox `msg/*.msg` |

## 定位链路

| 项 | 结果 | 证据 |
|---|---|---|
| `Localization` 已迁移 | 通过 | `src/core/localization/localization.*` |
| `LocSystem` 已迁移 | 通过 | `src/core/system/loc_system.*` |
| `LidarLoc` 已迁移 | 通过 | `src/core/localization/lidar_loc/lidar_loc.*` |
| 点云预处理已迁移 | 通过 | `src/core/lio/pointcloud_preprocess.*` |
| LIO 预测/里程计闭包已迁移 | 通过 | `src/core/lio/**`, `src/core/ivox3d/**`, `src/common/**` |
| PGO/高频外推/平滑已迁移 | 通过 | `src/core/localization/pose_graph/**` |
| `TiledMap` 和 chunk 加载已迁移 | 通过 | `src/core/maps/tiled_map*`, `src/core/maps/tiled_map_chunk*` |
| NDT-OMP 路线保留 | 通过 | `src/core/localization/lidar_loc/pclomp/**`；无阶段二后端重构 |
| 动态地图层路径保留 | 通过 | `src/core/maps/**`, `src/core/localization/lidar_loc/lidar_loc.*` |
| 在线定位入口存在 | 通过 | `src/app/run_loc_online.cc` |
| 离线 rosbag 入口存在 | 通过 | `src/app/run_loc_offline.cc`, `src/wrapper/bag_io.*` |
| TF 输出保留 | 通过 | `src/core/system/loc_system.cc`, `src/core/localization/localization_result.*` |
| Pangolin/RViz 路径已迁移或文档化 | 通过 | `src/ui/**`, `docs/io-spec.md`, `docs/manual-ros2-validation.md` |
| YAML 配置读取保留 | 通过 | `src/io/yaml_io.*`, `config/default*.yaml` |

## 构建与配置静态审计

| 项 | 结果 | 证据 |
|---|---|---|
| ROS2/PCL/tf2/rosbag2/pcl_conversions 依赖已声明 | 通过 | `package.xml`, `cmake/packages.cmake` |
| CMake 包含 C++17、目标、依赖、srv 生成和 install 规则 | 通过 | `CMakeLists.txt`, `src/CMakeLists.txt`, `src/app/CMakeLists.txt` |
| topic 名称已静态审计 | 通过 | `docs/io-spec.md`, `docs/stage-one-static-audit-report.md` |
| frame 名称已静态审计 | 通过 | `docs/io-spec.md` 中的 `map -> base_link` 和源码哈希对比 |
| map path 已静态审计 | 通过 | `system.map_path`, `--map_path`, `docs/io-spec.md` |
| config path 已静态审计 | 通过 | `config/default*.yaml`, launch 文件 |

## 文档与报告

| 项 | 结果 | 证据 |
|---|---|---|
| README 存在 | 通过 | `README.md` |
| 架构文档存在 | 通过 | `docs/architecture.md` |
| I/O 规范存在 | 通过 | `docs/io-spec.md`, `docs/input-output-spec.md` |
| 配置参数文档存在 | 通过 | `docs/config-parameters.md`, `docs/configuration-parameters.md` |
| 人工 ROS2 验证文档存在 | 通过 | `docs/manual-ros2-validation.md` |
| 静态审计报告存在 | 通过 | `docs/stage-one-static-audit-report.md` |
| 风险登记存在 | 通过 | `docs/risk-register.md` |
| 最终报告存在 | 通过 | `docs/stage-one-final-report.md` |

## 禁止项检查

| 禁止项 | 结果 | 证据 |
|---|---|---|
| 未删除原始 `lightning-lm/` 文件 | 通过 | 源码树仍存在 |
| 未引入阶段二工业化后端重构 | 通过 | NDT-OMP 仍为阶段一构建中的默认且唯一 matcher |
| 未优先改变匹配算法 | 通过 | `LidarLoc` 和 NDT-OMP 路径的源码哈希对比 |
| 未把定位行为改写为另一个系统 | 通过 | 核心运行时文件源码哈希对比 |
| 无交付关键占位注释 | 通过 | 继承的上游源码注释已记录为警告 |
| 无阶段一 goal 延期 | 通过 | 静态交付物和人工验证流程已存在 |
| 无虚假 ROS2/colcon 运行成功声明 | 通过 | 文档明确将 build/run/bag/RViz/Pangolin 标记为待人工 ROS2 验证 |

## 待人工 ROS2 环境验证

- `colcon build --packages-select lightning_localization`
- `ros2 run lightning_localization run_loc_online`
- `ros2 run lightning_localization run_loc_offline`
- 使用与上游相同的 config/map/bag 进行 rosbag 回放
- RViz 检查 `map -> base_link` TF
- Pangolin 运行时检查
- 与上游 lightning-lm 对比轨迹、confidence、状态和耗时

