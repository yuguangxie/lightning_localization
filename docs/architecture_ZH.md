# 架构

## 目标

本包在剥离为独立 ROS2 package 的同时，保持原始 lightning-lm 定位行为。

## 运行链路

```text
ROS2 在线 topic 或离线 rosbag
  -> PointCloudPreprocess
  -> LaserMapping 定位 LIO 前端
  -> LidarLoc scan-to-map 匹配
  -> TiledMap 静态和动态分块地图
  -> NDT-OMP
  -> LocalizationResult
  -> PGO 低频融合
  -> DR 外推和 PoseSmoother
  -> map -> base_link TF
```

## 在线模式

`src/app/run_loc_online.cc` 创建 `LocSystem`，读取 YAML 配置，订阅 IMU、标准 `sensor_msgs/msg/PointCloud2` 和 Livox `CustomMsg`，将初始位姿设置为单位位姿，然后进入 ROS2 节点 spin。若启用 `system.pub_tf`，`LocSystem` 会通过 `tf2_ros::TransformBroadcaster` 发布 `map -> base_link` 变换。

## 离线模式

`src/app/run_loc_offline.cc` 使用 `RosbagIO` 回放 ROS2 bag。IMU 和点云回调按确定顺序直接输入 `loc::Localization`。离线路径读取相同的 YAML topic 名称，并接受显式 `--map_path`。

## 匹配与融合

`LidarLoc` 在阶段一中保留 NDT-OMP 作为唯一 scan-to-map 后端。它会围绕预测位姿加载分块 PCD 地图，并在 chunk 变化时更新 NDT target。`PGO` 将低频激光定位、激光里程计和高频 DR 外推组合起来。

## 保留边界

阶段一不引入 `RegistrationBackend`、GICP、VGICP、diagnostics 重设计、生命周期重设计或基于 service 的重定位接线。这些属于阶段二工业化增强内容，不混入本次行为保持型剥离。

