# ROS2 Output Topics

This document describes ROS2 outputs added to the stage-one `lightning_localization` package without changing the default localization algorithm, NDT-OMP path, or existing `map -> base_link` TF behavior.

## Default Topics

| Topic | Message type | Default | Trigger |
|---|---|---|---|
| `/localization/pose` | `geometry_msgs/msg/PoseStamped` | Enabled | Every accepted localization result |
| `/localization/status` | `std_msgs/msg/String` | Enabled | Every localization result, throttled by `status_min_period_sec` |
| `/localization/diagnostics` | `diagnostic_msgs/msg/DiagnosticArray` | Enabled | Every localization result, throttled by `diagnostics_min_period_sec` |
| `/localization/odometry` | `nav_msgs/msg/Odometry` | Disabled | Every accepted localization result when enabled |

For pose and odometry, accepted means `LocalizationResult.valid_ == true` by default. Set `ros_output.publish_invalid_result: true` only when downstream tools need invalid pose-like messages. Status and diagnostics still report invalid or initializing results so operators can see failure state.

## Pose

`/localization/pose` publishes `geometry_msgs/msg/PoseStamped`.

| Field | Source |
|---|---|
| `header.stamp` | `LocalizationResult.timestamp_`, converted with seconds and nanoseconds |
| `header.frame_id` | `ros_output.map_frame`, default `map` |
| `pose.position` | `LocalizationResult.pose_.translation()` |
| `pose.orientation` | `LocalizationResult.pose_.so3().unit_quaternion()` |

## Status

`/localization/status` publishes `std_msgs/msg/String`. The payload is a compact key-value string containing:

- `status`: `IDLE`, `INITIALIZING`, `GOOD`, `FOLLOWING_DR`, `FAIL`, or `UNKNOWN`
- `valid`
- `lidar_loc_valid`
- `confidence`
- `timestamp`
- `health`: short health/failure text

The string topic is intentionally simple for stage one. A future productization pass can introduce a structured custom message after the interface is designed and versioned.

## Diagnostics

`/localization/diagnostics` publishes `diagnostic_msgs/msg/DiagnosticArray` with one `DiagnosticStatus`:

- `name`: `lightning_localization/localization`
- `hardware_id`: `lightning_localization`

Level mapping:

| Localization state | Diagnostic level |
|---|---|
| `GOOD` and valid | `OK` |
| `INITIALIZING` | `WARN` |
| `FOLLOWING_DR` | `WARN` |
| `FAIL` | `ERROR` |
| invalid result | `WARN` |

Key values include `status`, `valid`, `lidar_loc_valid`, `confidence`, `lidar_loc_odom_delta`, `lidar_loc_odom_error_normal`, `lidar_loc_smooth_flag`, `is_parking`, `timestamp`, `frame_id`, `child_frame_id`, `map_path`, `odometry_covariance_source`, and `note`.

The `note` field states that `confidence` is inherited from NDT transformation probability and is not a unified industrial score.

## Odometry

`/localization/odometry` publishes `nav_msgs/msg/Odometry` when `ros_output.publish_odometry: true`.

| Field | Source |
|---|---|
| `header.stamp` | `LocalizationResult.timestamp_` |
| `header.frame_id` | `ros_output.map_frame`, default `map` |
| `child_frame_id` | `ros_output.base_frame`, default `base_link` |
| `pose.pose` | Same pose as `/localization/pose` |
| `pose.covariance` | Static config placeholder values |
| `twist` | Left at default zero because this enhancement does not estimate velocity |

Odometry is disabled by default because this stage-one package does not provide a validated covariance or twist estimate for the published global localization result. If enabled, downstream consumers must treat covariance as `static_config_placeholder`, not a real estimator output.

## YAML Configuration

All default YAML files under `config/` include:

```yaml
ros_output:
  publish_pose: true
  pose_topic: "/localization/pose"
  publish_status: true
  status_topic: "/localization/status"
  publish_diagnostics: true
  diagnostics_topic: "/localization/diagnostics"
  publish_odometry: false
  odometry_topic: "/localization/odometry"
  publish_invalid_result: false
  map_frame: "map"
  base_frame: "base_link"
  diagnostics_min_period_sec: 0.1
  status_min_period_sec: 0.1
  odometry_covariance_source: "static_config_placeholder"
  odometry_position_covariance: [1.0, 1.0, 1.0]
  odometry_orientation_covariance: [0.5, 0.5, 0.5]
```

`system.pub_tf` remains the switch for the original TF broadcaster. Topic output does not replace or remove TF output.

## Online And Offline Boundary

Online mode (`run_loc_online`) creates `LocSystem`, ROS2 subscribers, TF broadcaster, and the new publishers.

Offline mode (`run_loc_offline`) constructs `loc::Localization` directly for bag replay and does not spin a publisher node. Its ROS2 topic output remains out of scope for this stage-one enhancement and must not be assumed available.

## Manual Validation Status

ROS2 compilation, node execution, rosbag replay, RViz2 inspection, Pangolin inspection, and topic echo/hz checks are pending manual validation in a real Ubuntu/ROS2/colcon environment.

