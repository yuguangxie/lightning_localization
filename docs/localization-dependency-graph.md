# Localization Dependency Graph

## Entry Points

```text
run_loc_online
  -> LocSystem
  -> loc::Localization

run_loc_offline
  -> RosbagIO
  -> loc::Localization
```

## Core Dependency Closure

| Module | Files |
|---|---|
| Online wrapper | `src/core/system/loc_system.*` |
| Offline bag wrapper | `src/wrapper/bag_io.*`, `src/wrapper/ros_utils.h` |
| Localization coordinator | `src/core/localization/localization.*` |
| Result model | `src/core/localization/localization_result.*` |
| Scan-to-map matching | `src/core/localization/lidar_loc/**` |
| NDT-OMP | `src/core/localization/lidar_loc/pclomp/**` |
| PGO and extrapolation | `src/core/localization/pose_graph/**` |
| LIO frontend | `src/core/lio/**` |
| Incremental voxel map | `src/core/ivox3d/**` |
| Tiled map | `src/core/maps/**` |
| Optimizer | `src/core/miao/core/**`, `src/core/miao/utils/**` |
| Common types | `src/common/**` |
| YAML and file IO | `src/io/**` |
| Timers and point cloud helpers | `src/utils/**` |
| UI path | `src/ui/**` |
| Third party headers | `thirdparty/Sophus/**` |
| Livox messages | `thirdparty/livox_ros_driver/msg/**` |

## External Dependencies

ROS2 `rclcpp`, `std_msgs`, `geometry_msgs`, `sensor_msgs`, `nav_msgs`, `std_srvs`, `tf2`, `tf2_ros`, `rosbag2_cpp`, `pcl_conversions`, `pcl_ros`, `message_filters`, `visualization_msgs`, PCL, Eigen3, yaml-cpp, Pangolin, OpenGL, OpenCV, glog, gflags, TBB, and OpenMP.

## Excluded Code

`SlamSystem`, SLAM applications, loop-closing applications, frontend-only applications, and map-save service runtime are excluded from the new build target. `SaveMap.srv` remains copied as an upstream interface definition, but it is not used by the localization executables.

