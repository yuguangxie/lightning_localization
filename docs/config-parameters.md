# Configuration Parameters

This package preserves the upstream `config/default*.yaml` files and their key names.

## System

| Key | Purpose |
|---|---|
| `system.with_ui` | Enable Pangolin UI code path |
| `system.map_path` | Online tiled map path |
| `system.pub_tf` | Publish `map -> base_link` TF |
| `system.enable_lidar_odom_skip` | Allow lidar odometry skip behavior |
| `system.enable_lidar_loc_skip` | Allow lidar localization skip behavior |
| `system.lidar_loc_skip_num` | Localization skip interval |

## Common Topics

| Key | Purpose |
|---|---|
| `common.imu_topic` | IMU input topic |
| `common.lidar_topic` | Standard point cloud topic |
| `common.livox_lidar_topic` | Livox custom point cloud topic |

## ROS Output

| Key | Purpose |
|---|---|
| `ros_output.publish_pose` | Enable `geometry_msgs/msg/PoseStamped` pose topic |
| `ros_output.pose_topic` | Pose topic name, default `/localization/pose` |
| `ros_output.publish_status` | Enable `std_msgs/msg/String` localization status topic |
| `ros_output.status_topic` | Status topic name, default `/localization/status` |
| `ros_output.publish_diagnostics` | Enable `diagnostic_msgs/msg/DiagnosticArray` diagnostics topic |
| `ros_output.diagnostics_topic` | Diagnostics topic name, default `/localization/diagnostics` |
| `ros_output.publish_odometry` | Enable optional `nav_msgs/msg/Odometry` topic; default is false |
| `ros_output.odometry_topic` | Odometry topic name, default `/localization/odometry` |
| `ros_output.publish_invalid_result` | Publish invalid localization results to pose-like topics; default is false |
| `ros_output.map_frame` | Topic frame id, default `map` |
| `ros_output.base_frame` | Odometry child frame id and diagnostics child frame, default `base_link` |
| `ros_output.diagnostics_min_period_sec` | Minimum diagnostics publish interval |
| `ros_output.status_min_period_sec` | Minimum status publish interval |
| `ros_output.odometry_covariance_source` | Declares covariance source; default `static_config_placeholder` |
| `ros_output.odometry_position_covariance` | Static placeholder diagonal position covariance `[x, y, z]` |
| `ros_output.odometry_orientation_covariance` | Static placeholder diagonal orientation covariance `[roll, pitch, yaw]` |

`system.pub_tf` remains the original TF switch and is separate from `ros_output`. Odometry covariance is not a real estimator output in stage one.

## Visualization

| Key | Purpose |
|---|---|
| `visualization.enable_rviz` | Operator-facing switch documented for launch/RViz workflows; RViz2 is started by launch, not by C++ |
| `visualization.rviz_config` | Default RViz2 config path, `rviz/lightning_localization.rviz` |
| `visualization.fixed_frame` | RViz2 fixed frame, default `map` |
| `visualization.base_frame` | Base frame shown by TF/Pose workflows, default `base_link` |
| `visualization.show_pose_topic` | Documents that pose display should be shown by default |
| `visualization.show_odometry_topic` | Documents odometry display default; odometry publisher remains controlled by `ros_output.publish_odometry` |
| `visualization.show_tf` | Documents TF display default |
| `visualization.show_scan_topic` | Documents that RViz2 should show `/localization/scan` by default |
| `visualization.show_map_topic` | Documents that RViz2 should show `/localization/map` by default |

## Point Cloud ROS Output

| Key | Purpose |
|---|---|
| `ros_output.publish_scan` | Publish current localized scan as `sensor_msgs/msg/PointCloud2`; default true |
| `ros_output.scan_topic` | Current localized scan topic, default `/localization/scan` |
| `ros_output.scan_pose_source` | Pose source used to transform scan visualization; default `final_pose`, matching `/localization/pose` |
| `ros_output.publish_map` | Publish currently loaded local tiled runtime map as `sensor_msgs/msg/PointCloud2`; default true |
| `ros_output.map_topic` | Local runtime map topic, default `/localization/map` |
| `ros_output.scan_min_period_sec` | Minimum period between scan topic publications; default `0.5` seconds to avoid high-rate RViz point cloud load |
| `ros_output.scan_timestamp_tolerance_sec` | Maximum allowed timestamp gap between cached scan and final pose before scan publication is skipped; default `0.3` seconds |
| `ros_output.scan_publish_only_on_new_scan` | Publish each cached scan at most once when true |
| `ros_output.map_min_period_sec` | Minimum period between local map topic publications |

## Initialization

| Key | Purpose |
|---|---|
| `initialization.source` | Startup source: `default`, `external_pose`, `rviz_initialpose`, or `functional_point` |
| `initialization.default_pose.enabled` | Enable default startup pose |
| `initialization.default_pose.pose` | Startup pose `[x, y, z, qx, qy, qz, qw]`; default identity pose preserves stage-one behavior |
| `initialization.external_pose.enabled` | Create set initial pose service |
| `initialization.external_pose.service_name` | Service name, default `/lightning_localization/set_initial_pose` |
| `initialization.external_pose.accept_frame_id` | Required service request frame, default `map` |
| `initialization.external_pose.require_valid_quaternion` | Reject non-unit quaternion when true |
| `initialization.external_pose.apply_immediately` | Allow immediate service pose application |
| `initialization.rviz_initialpose.enabled` | Subscribe to RViz2 `/initialpose` |
| `initialization.rviz_initialpose.topic` | RViz2 initial pose topic, default `/initialpose` |
| `initialization.rviz_initialpose.accept_frame_id` | Required RViz message frame, default `map` |
| `initialization.rviz_initialpose.require_valid_quaternion` | Reject non-unit quaternion when true |
| `initialization.rviz_initialpose.apply_immediately` | Apply RViz pose immediately when true |
| `initialization.rviz_initialpose.preserve_default_behavior` | With source `rviz_initialpose`, still apply default pose first when true |
| `initialization.status.publish_initialization_status` | Publish initialization status topic |
| `initialization.status.topic` | Initialization status topic, default `/localization/initialization_status` |

Initial pose injection only applies an initial guess. It does not mean localization has succeeded; use localization status, diagnostics, TF, and pose output after sensor/map input to judge success.

## Debug Dump

Debug PCD dump is disabled by default so normal localization does not write high-frequency debug point clouds.

| Key | Purpose |
|---|---|
| `debug_dump.enable_lidar_loc_target_dump` | Enable NDT target PCD dump from `LidarLoc::Localize`; default is false |
| `debug_dump.lidar_loc_target_path` | Output path used when lidar localization target dump is enabled |
| `debug_dump.enable_lio_map_dump` | Enable LIO global map debug dump from `LaserMapping::SaveMap`; default is false |
| `debug_dump.lio_map_path` | Output path used when LIO map dump is enabled |

## Lidar Localization

| Key | Purpose |
|---|---|
| `lidar_loc.force_2d` | Force planar output when enabled |
| `lidar_loc.init_with_fp` | Initialize against functional points |
| `lidar_loc.min_init_confidence` | Minimum NDT confidence for initialization |
| `lidar_loc.loc_on_kf` | Run map matching on LIO keyframes |
| `lidar_loc.update_dynamic_cloud` | Enable dynamic map update |
| `lidar_loc.update_kf_dis` | Dynamic layer distance trigger |
| `lidar_loc.update_kf_time` | Dynamic layer time trigger |

## Maps

| Key | Purpose |
|---|---|
| `maps.load_map_size` | Chunk load radius |
| `maps.unload_map_size` | Chunk unload radius |
| `maps.load_dyn_cloud` | Load dynamic layer |
| `maps.save_dyn_when_quit` | Save dynamic layer on quit |
| `maps.save_dyn_when_unload` | Save dynamic layer when chunks unload |
| `maps.dyn_cloud_policy` | Dynamic layer lifetime policy |

## Point Cloud Preprocess

| Key | Purpose |
|---|---|
| `fasterlio.lidar_type` | Livox, Velodyne, Ouster, or RoboSense parser mode |
| `fasterlio.time_scale` | Point time scaling |
| `fasterlio.blind` | Near-field blind filter |
| `fasterlio.point_filter_num` | Sampling interval |
| `roi.height_min` | Minimum retained z |
| `roi.height_max` | Maximum retained z |

## Static Audit Note

`lidar_loc` reads `loop_closing.with_height` as an inherited configuration key. The loop-closing runtime is not compiled into this standalone localization package.

