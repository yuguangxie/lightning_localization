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

