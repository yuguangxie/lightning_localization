# 输入输出规范

## 在线输入

由 `config/default*.yaml` 配置：

| YAML key | 消息类型 | 用途 |
|---|---|---|
| `common.imu_topic` | `sensor_msgs/msg/Imu` | IMU 积分和 DR 预测 |
| `common.lidar_topic` | `sensor_msgs/msg/PointCloud2` | 标准点云输入 |
| `common.livox_lidar_topic` | `livox_ros_driver2/msg/CustomMsg` | Livox 点云输入 |

在线节点会同时订阅标准点云 topic 和 Livox 点云 topic。部署时应确保只有预期的数据源处于活动状态，或确保配置的 topic 彼此分离。

## 离线输入

`run_loc_offline` 需要：

```bash
--config /path/to/config.yaml
--input_bag /path/to/rosbag
--map_path /path/to/data/new_map/
```

bag 必须包含配置的 IMU topic 和一个受支持的点云 topic。

## 地图输入

地图路径必须包含由 lightning-lm 建图生成的分块地图文件：

```text
index.txt
0.pcd
1.pcd
*_dyn.pcd           可选动态层
dynamic_polygon.txt 可选动态区域
global.pcd          可选展示地图
map.pgm             可选 2D 展示地图
```

`index.txt` 定义地图原点、chunk id、chunk 栅格坐标，以及用于初始化的功能点。

## 输出

当启用 `system.pub_tf` 时，在线 TF 输出如下：

| 字段 | 值 |
|---|---|
| `frame_id` | `map` |
| `child_frame_id` | `base_link` |
| translation | `LocalizationResult.pose_` 的平移 |
| rotation | `LocalizationResult.pose_` 的四元数 |

内部结果字段保留在 `src/core/localization/localization_result.h` 中，包括 timestamp、pose、validity、status、confidence、lidar loc validity、里程计 delta 检查、平滑标志和停车状态。

## 文件输出

继承运行时可能写入：

| 路径 | 触发条件 |
|---|---|
| `./data/recover_pose.txt` | 最近恢复的定位位姿 |
| `./data/tgt.pcd` | 继承的 NDT target 调试 dump |
| `[map_path]/*_dyn.pcd` | 启用时保存动态地图层 |

调试 target dump 是继承行为，阶段一保持不变。

