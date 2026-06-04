# Architecture

## Goal

This package preserves the original lightning-lm localization behavior while isolating it into a standalone ROS2 package.

## Runtime Chain

```text
ROS2 online topics or offline rosbag
  -> PointCloudPreprocess
  -> LaserMapping localization LIO frontend
  -> LidarLoc scan-to-map matching
  -> TiledMap static and dynamic chunk map
  -> NDT-OMP
  -> LocalizationResult
  -> PGO low-frequency fusion
  -> DR extrapolation and PoseSmoother
  -> map -> base_link TF
```

## Online Mode

`src/app/run_loc_online.cc` creates `LocSystem`, reads the YAML config, subscribes to IMU, standard `sensor_msgs/msg/PointCloud2`, and Livox `CustomMsg`, sets the initial pose to identity, and spins the ROS2 node. If `system.pub_tf` is enabled, `LocSystem` publishes `map -> base_link` transforms through `tf2_ros::TransformBroadcaster`.

## Offline Mode

`src/app/run_loc_offline.cc` uses `RosbagIO` to replay a ROS2 bag. IMU and point cloud callbacks feed `loc::Localization` directly in deterministic order. The offline path reads the same YAML topic names and accepts an explicit `--map_path`.

## Matching And Fusion

`LidarLoc` keeps NDT-OMP as the only stage-one scan-to-map backend. It loads tiled PCD maps around the predicted pose and updates the NDT target when chunks change. `PGO` combines low-frequency lidar localization with lidar odometry and high-frequency DR extrapolation.

## Preserved Boundaries

Stage one does not introduce `RegistrationBackend`, GICP, VGICP, diagnostics redesign, lifecycle redesign, or service-based relocalization wiring. Those are stage-two industrial hardening topics and are not mixed into this behavior-preserving extraction.

