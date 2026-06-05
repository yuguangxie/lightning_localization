# ROS2 输出 Topic

本文说明阶段一 `lightning_localization` 包新增的 ROS2 输出 topic。本轮不改变默认定位算法、不改 NDT-OMP 路线，也不替换原有 `map -> base_link` TF 输出。

## 默认 Topic

| Topic | 消息类型 | 默认状态 | 触发条件 |
|---|---|---|---|
| `/localization/pose` | `geometry_msgs/msg/PoseStamped` | 开启 | 每个被接受的定位结果 |
| `/localization/status` | `std_msgs/msg/String` | 开启 | 每个定位结果，受 `status_min_period_sec` 节流 |
| `/localization/diagnostics` | `diagnostic_msgs/msg/DiagnosticArray` | 开启 | 每个定位结果，受 `diagnostics_min_period_sec` 节流 |
| `/localization/odometry` | `nav_msgs/msg/Odometry` | 关闭 | 启用后每个被接受的定位结果 |
| `/localization/scan` | `sensor_msgs/msg/PointCloud2` | 开启 | 当前 scan 按定位 pose 变换到 `ros_output.map_frame` 后发布，受 `scan_min_period_sec` 节流 |
| `/localization/map` | `sensor_msgs/msg/PointCloud2` | 开启 | tiled runtime map chunk 更新后发布当前已加载局部地图，受 `map_min_period_sec` 节流 |

对于 pose 和 odometry，被接受默认表示 `LocalizationResult.valid_ == true`。只有下游确实需要 invalid 位姿类消息时，才建议设置 `ros_output.publish_invalid_result: true`。status 和 diagnostics 仍会报告 invalid 或初始化阶段结果，以便操作者观察失败状态。

## Pose

`/localization/pose` 发布 `geometry_msgs/msg/PoseStamped`。

| 字段 | 来源 |
|---|---|
| `header.stamp` | `LocalizationResult.timestamp_`，保留秒和纳秒 |
| `header.frame_id` | `ros_output.map_frame`，默认 `map` |
| `pose.position` | `LocalizationResult.pose_.translation()` |
| `pose.orientation` | `LocalizationResult.pose_.so3().unit_quaternion()` |

## Status

`/localization/status` 发布 `std_msgs/msg/String`。内容为紧凑 key-value 字符串，至少包含：

- `status`: `IDLE`、`INITIALIZING`、`GOOD`、`FOLLOWING_DR`、`FAIL` 或 `UNKNOWN`
- `valid`
- `lidar_loc_valid`
- `confidence`
- `timestamp`
- `health`

本轮使用字符串 topic 是为了降低阶段一构建和接口风险。长期产品化时可再设计版本化的结构化自定义消息。

## Diagnostics

`/localization/diagnostics` 发布 `diagnostic_msgs/msg/DiagnosticArray`，其中包含一个 `DiagnosticStatus`：

- `name`: `lightning_localization/localization`
- `hardware_id`: `lightning_localization`

level 映射：

| 定位状态 | diagnostics level |
|---|---|
| `GOOD` 且 valid | `OK` |
| `INITIALIZING` | `WARN` |
| `FOLLOWING_DR` | `WARN` |
| `FAIL` | `ERROR` |
| invalid result | `WARN` |

KeyValue 包含 `status`、`valid`、`lidar_loc_valid`、`confidence`、`lidar_loc_odom_delta`、`lidar_loc_odom_error_normal`、`lidar_loc_smooth_flag`、`is_parking`、`timestamp`、`frame_id`、`child_frame_id`、`map_path`、`odometry_covariance_source` 和 `note`。

`note` 明确说明：当前 `confidence` 继承自 NDT transformation probability，不是统一工业级 score。

## Odometry

`/localization/odometry` 在 `ros_output.publish_odometry: true` 时发布 `nav_msgs/msg/Odometry`。

| 字段 | 来源 |
|---|---|
| `header.stamp` | `LocalizationResult.timestamp_` |
| `header.frame_id` | `ros_output.map_frame`，默认 `map` |
| `child_frame_id` | `ros_output.base_frame`，默认 `base_link` |
| `pose.pose` | 与 `/localization/pose` 相同 |
| `pose.covariance` | 静态配置占位值 |
| `twist` | 保持默认 0，本轮不估计速度 |

Odometry 默认关闭，因为阶段一没有给全局定位结果提供已验证的协方差或 twist 估计。启用后，下游必须把 covariance 理解为 `static_config_placeholder`。

## PointCloud2 可视化输出

`/localization/scan` 发布当前 LiDAR scan 的可视化点云。该点云不是原始传感器 topic，而是在 `LidarLoc` 报告 `lidar_loc_valid_ == true` 后，使用当前 scan-to-map pose 变换到 `ros_output.map_frame`，默认 `map`。这里故意使用 `LidarLoc` 的有效标志，而不是后续 PGO 融合结果的 `LocalizationResult.valid_`，因为 PGO 会在更晚阶段才标记融合结果有效。

`/localization/map` 发布当前已加载的局部 tiled runtime map。该 topic 在地图 chunk 重新加载或更新 NDT target 后发布，使用 reliable transient-local QoS，便于 RViz2 display 在稍后订阅时仍能收到最近一次局部地图。

| Topic | Frame | 配置 |
|---|---|---|
| `/localization/scan` | `ros_output.map_frame` | `ros_output.publish_scan`、`ros_output.scan_topic`、`ros_output.scan_min_period_sec` |
| `/localization/map` | `ros_output.map_frame` | `ros_output.publish_map`、`ros_output.map_topic`、`ros_output.map_min_period_sec` |

## YAML 配置

所有 `config/default*.yaml` 均包含 `ros_output` 配置段。`system.pub_tf` 仍只控制原 TF 广播，topic 输出不会替换或删除 TF。

## 在线和离线边界

在线模式 `run_loc_online` 创建 `LocSystem`、ROS2 subscribers、TF broadcaster 和新增 publishers。

离线模式 `run_loc_offline` 直接构造 `loc::Localization` 做 bag 回放，不 spin publisher node，因此本轮新增 topic 不适用于当前离线入口。

## 验证状态

ROS2 编译、节点运行、rosbag 回放、RViz2 检查、Pangolin 检查以及 topic echo/hz 均待真实 Ubuntu/ROS2/colcon 环境人工验证。

