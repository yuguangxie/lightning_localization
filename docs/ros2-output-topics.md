# ROS2 Output Topics

This document describes ROS2 outputs added to the stage-one `lightning_localization` package without changing the default localization algorithm, NDT-OMP path, or existing `map -> base_link` TF behavior.

## Default Topics

| Topic | Message type | Default | Trigger |
|---|---|---|---|
| `/localization/pose` | `geometry_msgs/msg/PoseStamped` | Enabled | Every accepted localization result |
| `/localization/status` | `std_msgs/msg/String` | Enabled | Every localization result, throttled by `status_min_period_sec` |
| `/localization/diagnostics` | `diagnostic_msgs/msg/DiagnosticArray` | Enabled | Every localization result, throttled by `diagnostics_min_period_sec` |
| `/localization/odometry` | `nav_msgs/msg/Odometry` | Disabled | Every accepted localization result when enabled |
| `/localization/scan` | `sensor_msgs/msg/PointCloud2` | Enabled | Latest valid LidarLoc scan transformed with the same final pose source used by `/localization/pose`, throttled by `scan_min_period_sec` |
| `/localization/map` | `sensor_msgs/msg/PointCloud2` | Enabled | Loaded local runtime map after tiled map chunk updates, throttled by `map_min_period_sec` |
| `/localization/initialization_status` | `std_msgs/msg/String` | Enabled | Startup initialization, service/RViz requests, and localization result throttling |

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

Initialization key values are also included: `initialization_source`, `last_initialization_source`, `last_initialization_accepted`, `last_initialization_applied`, `last_initialization_message`, `waiting_for_initial_pose`, `initialpose_topic`, and `set_initial_pose_service`.

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

## Point Clouds

`/localization/scan` publishes `sensor_msgs/msg/PointCloud2` for RViz2 visualization, not raw sensor data. `Localization::LidarLocProcCloud()` caches the latest scan only after `LidarLoc` reports `lidar_loc_valid_ == true`; the actual ROS2 publication happens later from the same PGO high-frequency output path that drives `/localization/pose`. The scan is transformed into `ros_output.map_frame`, default `map`, using the final `LocalizationResult.pose_` associated with the pose topic. This keeps scan visualization aligned with `/localization/pose` while avoiding high-frequency point cloud publication.

Scan publication is resource-throttled by `ros_output.scan_min_period_sec`, defaults to `0.5` seconds. `ros_output.scan_publish_only_on_new_scan` prevents repeated publication of the same cached scan, and `ros_output.scan_timestamp_tolerance_sec`, default `0.3` seconds, rejects stale scan/pose pairings whose timestamps diverge too far.

`/localization/map` publishes `sensor_msgs/msg/PointCloud2` for the currently loaded local tiled runtime map. It is published when `LidarLoc` reloads map chunks through `TiledMap::LoadOnPose()` and the map update thread refreshes the NDT target. The map publisher uses reliable transient-local QoS so RViz2 can receive the latest local map even if the display subscribes after the first publication.

| Topic | Frame | QoS intent | Config |
|---|---|---|---|
| `/localization/scan` | `ros_output.map_frame` | Sensor data QoS, low-rate pose-aligned visualization | `ros_output.publish_scan`, `ros_output.scan_topic`, `ros_output.scan_pose_source`, `ros_output.scan_min_period_sec`, `ros_output.scan_timestamp_tolerance_sec`, `ros_output.scan_publish_only_on_new_scan` |
| `/localization/map` | `ros_output.map_frame` | Reliable transient local, chunk-update visualization | `ros_output.publish_map`, `ros_output.map_topic`, `ros_output.map_min_period_sec` |

## Initialization Status

`/localization/initialization_status` publishes `std_msgs/msg/String`. The payload is a JSON string with escaped string fields containing:

- `current_initialization_source`
- `last_request_source`
- `last_request_time`
- `last_request_frame`
- `last_request_accepted`
- `last_request_applied`
- `last_message`
- `localization_status`
- `waiting_for_initial_pose`
- `external_pose_service_enabled`
- `rviz_initialpose_enabled`
- `initialpose_topic`
- `set_initial_pose_service`
- `core_initialized`

Example payload:

```json
{
  "current_initialization_source": "external_pose",
  "last_request_source": "operator",
  "last_request_time": 1710000000.100000,
  "last_request_frame": "map",
  "last_request_accepted": true,
  "last_request_applied": true,
  "last_message": "initial pose accepted and applied as initial guess; localization success still depends on later matching",
  "localization_status": "INITIALIZING",
  "waiting_for_initial_pose": false,
  "external_pose_service_enabled": true,
  "rviz_initialpose_enabled": true,
  "initialpose_topic": "/initialpose",
  "set_initial_pose_service": "/lightning_localization/set_initial_pose",
  "core_initialized": true,
  "timestamp": 1710000000.100000
}
```

The status distinguishes pose acceptance/application from localization success. A service or RViz request can be accepted and applied as an initial guess while the localization state remains `INITIALIZING` until later scan-to-map matching succeeds.

If the localization core fails to initialize, `run_loc_online` exits instead of spinning normal ROS2 input interfaces. `ApplyInitialPose()` also rejects internal calls while `core_initialized=false`, so a missing map or failed `loc_->Init()` cannot be reported as pose applied or localization success.

## Initial Pose Inputs

| Interface | Type | Default | Purpose |
|---|---|---|---|
| `/initialpose` | `geometry_msgs/msg/PoseWithCovarianceStamped` subscription | Enabled | RViz2 `2D Pose Estimate` input |
| `/lightning_localization/set_initial_pose` | `lightning_localization/srv/SetInitialPose` service | Enabled | External/operator initial pose injection |

Both inputs require finite pose values and a valid quaternion. `frame_id` must match the configured accepted frame, default `map`.

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
  publish_scan: true
  scan_topic: "/localization/scan"
  scan_pose_source: "final_pose"
  publish_map: true
  map_topic: "/localization/map"
  publish_invalid_result: false
  map_frame: "map"
  base_frame: "base_link"
  diagnostics_min_period_sec: 0.1
  status_min_period_sec: 0.1
  scan_min_period_sec: 0.5
  scan_timestamp_tolerance_sec: 0.3
  scan_publish_only_on_new_scan: true
  map_min_period_sec: 1.0
  odometry_covariance_source: "static_config_placeholder"
  odometry_position_covariance: [1.0, 1.0, 1.0]
  odometry_orientation_covariance: [0.5, 0.5, 0.5]

initialization:
  source: "default"
  default_pose:
    enabled: true
    pose: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0]
  external_pose:
    enabled: true
    service_name: "/lightning_localization/set_initial_pose"
    accept_frame_id: "map"
    require_valid_quaternion: true
    apply_immediately: true
  rviz_initialpose:
    enabled: true
    topic: "/initialpose"
    accept_frame_id: "map"
    require_valid_quaternion: true
    apply_immediately: true
    preserve_default_behavior: true
  status:
    publish_initialization_status: true
    topic: "/localization/initialization_status"
```

`system.pub_tf` remains the switch for the original TF broadcaster. Topic output does not replace or remove TF output.

## Online And Offline Boundary

Online mode (`run_loc_online`) creates `LocSystem`, ROS2 subscribers, TF broadcaster, and the new publishers.

Offline mode (`run_loc_offline`) constructs `loc::Localization` directly for bag replay and does not spin a publisher node. Its ROS2 topic output remains out of scope for this stage-one enhancement and must not be assumed available.

## Manual Validation Status

ROS2 compilation, node execution, rosbag replay, RViz2 inspection, Pangolin inspection, and topic echo/hz checks are pending manual validation in a real Ubuntu/ROS2/colcon environment.

