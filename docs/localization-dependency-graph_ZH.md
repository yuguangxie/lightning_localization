# 定位依赖图

## 入口

```text
run_loc_online
  -> LocSystem
  -> loc::Localization

run_loc_offline
  -> RosbagIO
  -> loc::Localization
```

## 核心依赖闭包

| 模块 | 文件 |
|---|---|
| 在线封装 | `src/core/system/loc_system.*` |
| 离线 bag 封装 | `src/wrapper/bag_io.*`, `src/wrapper/ros_utils.h` |
| 定位协调器 | `src/core/localization/localization.*` |
| 结果模型 | `src/core/localization/localization_result.*` |
| Scan-to-map 匹配 | `src/core/localization/lidar_loc/**` |
| NDT-OMP | `src/core/localization/lidar_loc/pclomp/**` |
| PGO 与外推 | `src/core/localization/pose_graph/**` |
| LIO 前端 | `src/core/lio/**` |
| 增量体素地图 | `src/core/ivox3d/**` |
| 分块地图 | `src/core/maps/**` |
| 优化器 | `src/core/miao/core/**`, `src/core/miao/utils/**` |
| 通用类型 | `src/common/**` |
| YAML 和文件 IO | `src/io/**` |
| Timer 和点云辅助工具 | `src/utils/**` |
| UI 路径 | `src/ui/**` |
| 第三方头文件 | `thirdparty/Sophus/**` |
| Livox 消息 | `thirdparty/livox_ros_driver/msg/**` |

## 外部依赖

ROS2 `rclcpp`、`std_msgs`、`geometry_msgs`、`sensor_msgs`、`nav_msgs`、`std_srvs`、`tf2`、`tf2_ros`、`rosbag2_cpp`、`pcl_conversions`、`pcl_ros`、`message_filters`、`visualization_msgs`、PCL、Eigen3、yaml-cpp、Pangolin、OpenGL、OpenCV、glog、gflags、TBB 和 OpenMP。

## 排除代码

`SlamSystem`、SLAM 应用、回环应用、仅前端应用和保存地图 service 运行时均从新构建目标中排除。`SaveMap.srv` 仍作为上游接口定义被复制，但定位可执行文件不使用它。

