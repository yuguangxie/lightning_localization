# 配置参数

本包保留上游 `config/default*.yaml` 文件及其 key 名称。

## System

| Key | 用途 |
|---|---|
| `system.with_ui` | 启用 Pangolin UI 代码路径 |
| `system.map_path` | 在线分块地图路径 |
| `system.pub_tf` | 发布 `map -> base_link` TF |
| `system.enable_lidar_odom_skip` | 允许激光里程计跳帧行为 |
| `system.enable_lidar_loc_skip` | 允许激光定位跳帧行为 |
| `system.lidar_loc_skip_num` | 定位跳帧间隔 |

## Common Topics

| Key | 用途 |
|---|---|
| `common.imu_topic` | IMU 输入 topic |
| `common.lidar_topic` | 标准点云 topic |
| `common.livox_lidar_topic` | Livox 自定义点云 topic |

## Debug Dump

调试 PCD 写盘默认关闭，普通定位运行不会写高频调试点云。

| Key | 用途 |
|---|---|
| `debug_dump.enable_lidar_loc_target_dump` | 启用 `LidarLoc::Localize` 的 NDT target PCD dump，默认 false |
| `debug_dump.lidar_loc_target_path` | 启用 lidar localization target dump 时的输出路径 |
| `debug_dump.enable_lio_map_dump` | 启用 `LaserMapping::SaveMap` 的 LIO 全局地图调试 dump，默认 false |
| `debug_dump.lio_map_path` | 启用 LIO map dump 时的输出路径 |

## Lidar Localization

| Key | 用途 |
|---|---|
| `lidar_loc.force_2d` | 启用后强制平面输出 |
| `lidar_loc.init_with_fp` | 使用功能点初始化 |
| `lidar_loc.min_init_confidence` | 初始化所需的最低 NDT confidence |
| `lidar_loc.loc_on_kf` | 在 LIO 关键帧上执行地图匹配 |
| `lidar_loc.update_dynamic_cloud` | 启用动态地图更新 |
| `lidar_loc.update_kf_dis` | 动态层距离触发阈值 |
| `lidar_loc.update_kf_time` | 动态层时间触发阈值 |

## Maps

| Key | 用途 |
|---|---|
| `maps.load_map_size` | chunk 加载半径 |
| `maps.unload_map_size` | chunk 卸载半径 |
| `maps.load_dyn_cloud` | 加载动态层 |
| `maps.save_dyn_when_quit` | 退出时保存动态层 |
| `maps.save_dyn_when_unload` | 卸载 chunk 时保存动态层 |
| `maps.dyn_cloud_policy` | 动态层生命周期策略 |

## Point Cloud Preprocess

| Key | 用途 |
|---|---|
| `fasterlio.lidar_type` | Livox、Velodyne、Ouster 或 RoboSense 解析模式 |
| `fasterlio.time_scale` | 点时间缩放 |
| `fasterlio.blind` | 近场盲区过滤 |
| `fasterlio.point_filter_num` | 采样间隔 |
| `roi.height_min` | 保留点的最小 z |
| `roi.height_max` | 保留点的最大 z |

## 静态审计说明

`lidar_loc` 会读取继承配置 key `loop_closing.with_height`。回环运行时没有编译进这个独立定位包。
