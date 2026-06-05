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

