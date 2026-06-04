# Input And Output Specification

## Online Inputs

Configured by `config/default*.yaml`:

| YAML key | Message type | Purpose |
|---|---|---|
| `common.imu_topic` | `sensor_msgs/msg/Imu` | IMU integration and DR prediction |
| `common.lidar_topic` | `sensor_msgs/msg/PointCloud2` | Standard point cloud input |
| `common.livox_lidar_topic` | `livox_ros_driver2/msg/CustomMsg` | Livox point cloud input |

The online node subscribes to both standard point cloud and Livox point cloud topics. A deployment should ensure only the intended source is active or that the configured topics are mutually separated.

## Offline Inputs

`run_loc_offline` requires:

```bash
--config /path/to/config.yaml
--input_bag /path/to/rosbag
--map_path /path/to/data/new_map/
```

The bag must contain the configured IMU topic and one supported point cloud topic.

## Map Inputs

The map path must contain the tiled map files produced by lightning-lm mapping:

```text
index.txt
0.pcd
1.pcd
*_dyn.pcd          optional dynamic layer
dynamic_polygon.txt optional dynamic area
global.pcd          optional display map
map.pgm             optional 2D display map
```

`index.txt` defines map origin, chunk ids, chunk grid coordinates, and functional points used for initialization.

## Outputs

Online TF output when `system.pub_tf` is enabled:

| Field | Value |
|---|---|
| `frame_id` | `map` |
| `child_frame_id` | `base_link` |
| translation | `LocalizationResult.pose_` translation |
| rotation | `LocalizationResult.pose_` quaternion |

Internal result fields are preserved in `src/core/localization/localization_result.h`, including timestamp, pose, validity, status, confidence, lidar loc validity, odometry delta checks, smoothing flags, and parking state.

## File Outputs

The inherited runtime can write:

| Path | Trigger |
|---|---|
| `./data/recover_pose.txt` | latest recovered localization pose |
| `./data/tgt.pcd` | inherited NDT target debug dump |
| `[map_path]/*_dyn.pcd` | dynamic map layer save when enabled |

The debug target dump is an inherited behavior and remains unchanged in stage one.

