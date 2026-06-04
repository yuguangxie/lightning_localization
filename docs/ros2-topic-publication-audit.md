# ROS2 Topic Publication Audit

This audit records the ROS2 outputs found in `lightning_localization` before this topic-output enhancement, plus the outputs added by this change. The current Windows Codex environment has no ROS2, no colcon, and no bag/map data, so this document is a source-level audit only.

## Pre-Enhancement Output Audit

| Output name | Type | Message type | Currently existed | Publication location | Publication frequency / trigger | Config switch | Notes |
| ---- | -- | ---- | ------ | ---- | --------- | ---- | -- |
| `map -> base_link` | TF broadcast | `geometry_msgs/msg/TransformStamped` | Yes | `src/core/system/loc_system.cc`, through `tf2_ros::TransformBroadcaster::sendTransform`; data from `LocalizationResult::ToGeoMsg()` | Triggered when PGO high-frequency output produces a valid `LocalizationResult` | `system.pub_tf` | Preserved unchanged. `LocalizationResult::ToGeoMsg()` hard-codes `map` and `base_link`. |
| Pose topic | ROS2 topic | None before enhancement | No | Not present before enhancement | Not applicable | Not applicable | No `create_publisher` existed for pose before this change. |
| Odometry topic | ROS2 topic | None before enhancement | No | Not present before enhancement | Not applicable | Not applicable | `nav_msgs/msg/Odometry` is present in bag IO serialization, but no localization odometry publisher existed. |
| Status topic | ROS2 topic | None before enhancement | No | Not present before enhancement | Not applicable | Not applicable | `Localization` had an internal `LocStateCallback` type, but `LocSystem` did not register or publish it as a ROS topic. |
| Diagnostics topic | ROS2 topic | None before enhancement | No | Not present before enhancement | Not applicable | Not applicable | No `diagnostic_msgs` dependency or diagnostics publisher existed. |
| Current scan/map topic | ROS2 topic | None before enhancement | No | Not present in `LocSystem`; Pangolin UI receives in-process scan updates | Not applicable | `system.with_ui` only affects Pangolin UI | Stage one keeps scan/map visualization as inherited UI behavior; no scan/map ROS topic is added in this enhancement. |
| Offline run output topic | ROS2 topic | None before enhancement | No | `src/app/run_loc_offline.cc` constructs `loc::Localization` directly, not `LocSystem` | Not applicable | Not applicable | Offline mode replays bag data in-process and writes inherited outputs such as logs/recover pose; it does not spin a publisher node. |
| Result-to-ROS helper | Helper | `geometry_msgs/msg/TransformStamped` | Yes | `src/core/localization/localization_result.cc` | Called by TF callback when result is valid | Not configurable | Existing helper converts only to TF transform, not `PoseStamped`, `Odometry`, status, or diagnostics. |

Search terms reviewed: `create_publisher`, `publish(`, `TransformBroadcaster`, `sendTransform`, `SetTFCallback`, `LocalizationResult`, `ToGeoMsg`, `status_callback`, `Register`, `LocSystem`, `rclcpp::Publisher`, `diagnostic`, `nav_msgs`, `geometry_msgs`, and `std_msgs`.

## Post-Enhancement Output Audit

| Output name | Type | Message type | Currently existed | Publication location | Publication frequency / trigger | Config switch | Notes |
| ---- | -- | ---- | ------ | ---- | --------- | ---- | -- |
| `/localization/pose` | ROS2 topic | `geometry_msgs/msg/PoseStamped` | Added | `LocSystem::PublishLocalizationTopics()` | Every accepted localization result | `ros_output.publish_pose`, `ros_output.pose_topic`, `ros_output.publish_invalid_result` | Defaults to publishing valid results only. |
| `/localization/status` | ROS2 topic | `std_msgs/msg/String` | Added | `LocSystem::PublishLocalizationTopics()` | Every localization result, throttled by `ros_output.status_min_period_sec` | `ros_output.publish_status`, `ros_output.status_topic` | Includes status enum name, valid flags, confidence, timestamp, and health text. |
| `/localization/diagnostics` | ROS2 topic | `diagnostic_msgs/msg/DiagnosticArray` | Added | `LocSystem::PublishLocalizationTopics()` | Every localization result, throttled by `ros_output.diagnostics_min_period_sec` | `ros_output.publish_diagnostics`, `ros_output.diagnostics_topic` | Includes one `DiagnosticStatus` named `lightning_localization/localization`. |
| `/localization/odometry` | ROS2 topic | `nav_msgs/msg/Odometry` | Added, default off | `LocSystem::PublishLocalizationTopics()` | Every accepted localization result when enabled | `ros_output.publish_odometry`, `ros_output.odometry_topic` | Pose mirrors localization pose. Twist is not estimated. Pose covariance comes from static config placeholders. |
| `map -> base_link` | TF broadcast | `geometry_msgs/msg/TransformStamped` | Preserved | Existing TF callback and `sendTransform` path | Valid localization result | `system.pub_tf` | New topic publishing is separate from TF broadcasting. |

## Boundaries

- No scan/map point cloud topic was added in this stage-one enhancement.
- Offline `run_loc_offline` still does not publish ROS2 topics because it does not construct `LocSystem` or spin a publisher node.
- No custom message was added; this keeps the build surface smaller and uses ROS2 standard messages.
- ROS2 build and runtime behavior are pending manual validation in Ubuntu/ROS2/colcon.

