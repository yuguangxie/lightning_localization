# ROS2 Topic 发布审计

本文记录 `lightning_localization` 在本轮 topic 输出增强前后的 ROS2 输出。当前 Codex 运行在 Windows 环境，没有 ROS2、colcon、完整 bag/map 数据，因此本文只代表源码级静态审计。

## 增强前输出审计

| 输出名称 | 类型 | 消息类型 | 当前是否存在 | 发布位置 | 发布频率/触发条件 | 配置开关 | 备注 |
| ---- | -- | ---- | ------ | ---- | --------- | ---- | -- |
| `map -> base_link` | TF 广播 | `geometry_msgs/msg/TransformStamped` | 是 | `src/core/system/loc_system.cc`，通过 `tf2_ros::TransformBroadcaster::sendTransform`，数据来自 `LocalizationResult::ToGeoMsg()` | PGO 高频输出产生有效 `LocalizationResult` 时触发 | `system.pub_tf` | 已保留。`LocalizationResult::ToGeoMsg()` 固定使用 `map` 与 `base_link`。 |
| pose topic | ROS2 topic | 增强前无 | 否 | 增强前不存在 | 不适用 | 不适用 | 增强前未发现 pose 的 `create_publisher`。 |
| odometry topic | ROS2 topic | 增强前无 | 否 | 增强前不存在 | 不适用 | 不适用 | `bag_io` 中有 `nav_msgs/msg/Odometry` 序列化成员，但没有定位 odometry publisher。 |
| status topic | ROS2 topic | 增强前无 | 否 | 增强前不存在 | 不适用 | 不适用 | `Localization` 内部有 `LocStateCallback` 类型，但 `LocSystem` 未注册为 ROS topic。 |
| diagnostics topic | ROS2 topic | 增强前无 | 否 | 增强前不存在 | 不适用 | 不适用 | 增强前没有 `diagnostic_msgs` 依赖和 diagnostics publisher。 |
| `/localization/scan` | ROS2 topic | `sensor_msgs/msg/PointCloud2` | RViz2 可视化增强后新增 | `LocSystem::PublishScanCloud()`，经 `Localization::SetScanCloudCallback()` 回调 | LidarLoc 产生有效定位结果后；受 `ros_output.scan_min_period_sec` 节流 | `ros_output.publish_scan`、`ros_output.scan_topic` | 当前 scan 按定位 pose 变换到 `ros_output.map_frame`，用于 RViz2 显示。 |
| `/localization/map` | ROS2 topic | `sensor_msgs/msg/PointCloud2` | RViz2 可视化增强后新增 | `LocSystem::PublishMapCloud()`，经 `LidarLoc::UpdateMapThread()` 地图回调 | tiled map chunk 更新后；受 `ros_output.map_min_period_sec` 节流 | `ros_output.publish_map`、`ros_output.map_topic` | 当前已加载的局部 runtime map，不等同于完整全局地图。 |
| 离线运行输出 topic | ROS2 topic | 增强前无 | 否 | `src/app/run_loc_offline.cc` 直接构造 `loc::Localization`，不使用 `LocSystem` | 不适用 | 不适用 | 离线模式仍为进程内回放，不 spin publisher node。 |
| 结果到 ROS helper | helper | `geometry_msgs/msg/TransformStamped` | 是 | `src/core/localization/localization_result.cc` | TF callback 在结果有效时调用 | 不可配置 | 现有 helper 只转换 TF transform，不转换 `PoseStamped`、`Odometry`、status 或 diagnostics。 |

已搜索关键字：`create_publisher`、`publish(`、`TransformBroadcaster`、`sendTransform`、`SetTFCallback`、`LocalizationResult`、`ToGeoMsg`、`status_callback`、`Register`、`LocSystem`、`rclcpp::Publisher`、`diagnostic`、`nav_msgs`、`geometry_msgs`、`std_msgs`。

## 增强后输出审计

| 输出名称 | 类型 | 消息类型 | 当前是否存在 | 发布位置 | 发布频率/触发条件 | 配置开关 | 备注 |
| ---- | -- | ---- | ------ | ---- | --------- | ---- | -- |
| `/localization/pose` | ROS2 topic | `geometry_msgs/msg/PoseStamped` | 新增 | `LocSystem::PublishLocalizationTopics()` | 每个被接受的定位结果 | `ros_output.publish_pose`、`ros_output.pose_topic`、`ros_output.publish_invalid_result` | 默认只发布有效结果。 |
| `/localization/status` | ROS2 topic | `std_msgs/msg/String` | 新增 | `LocSystem::PublishLocalizationTopics()` | 每个定位结果，受 `ros_output.status_min_period_sec` 节流 | `ros_output.publish_status`、`ros_output.status_topic` | 包含状态名、valid、confidence、timestamp 和 health 文本。 |
| `/localization/diagnostics` | ROS2 topic | `diagnostic_msgs/msg/DiagnosticArray` | 新增 | `LocSystem::PublishLocalizationTopics()` | 每个定位结果，受 `ros_output.diagnostics_min_period_sec` 节流 | `ros_output.publish_diagnostics`、`ros_output.diagnostics_topic` | 包含名为 `lightning_localization/localization` 的 `DiagnosticStatus`。 |
| `/localization/odometry` | ROS2 topic | `nav_msgs/msg/Odometry` | 新增，默认关闭 | `LocSystem::PublishLocalizationTopics()` | 启用后每个被接受的定位结果 | `ros_output.publish_odometry`、`ros_output.odometry_topic` | pose 与定位结果一致；twist 不估计；covariance 来自静态配置占位值。 |
| `map -> base_link` | TF 广播 | `geometry_msgs/msg/TransformStamped` | 保留 | 原有 TF callback 和 `sendTransform` 路径 | 有效定位结果 | `system.pub_tf` | topic 发布与 TF 广播相互独立。 |

## 边界

- 本轮不新增 scan/map 点云 topic。
- 离线 `run_loc_offline` 仍不发布 ROS2 topic，因为它不构造 `LocSystem` 或 spin publisher node。
- 本轮没有新增自定义 msg，优先使用 ROS2 标准消息。
- ROS2 编译和运行仍待 Ubuntu/ROS2/colcon 环境人工验证。

