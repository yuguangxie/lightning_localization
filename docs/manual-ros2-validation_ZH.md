# 人工 ROS2 验证

## 环境

使用 Ubuntu 22.04 或兼容版本、ROS2 Humble 或更新版本，以及 colcon。安装与上游 README 和 `scripts/install_dep.sh` 等价的依赖：

```bash
sudo apt update
sudo apt install -y \
  libopencv-dev libpcl-dev pcl-tools libyaml-cpp-dev \
  libgoogle-glog-dev libgflags-dev ros-humble-pcl-conversions \
  ros-humble-tf2-ros ros-humble-rosbag2-cpp
```

启用 UI 构建前，必须确保 Pangolin 和 OpenGL 依赖可用。

## 构建

将此包放入 ROS2 workspace 的 `src/` 目录：

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-select lightning_localization
source install/setup.bash
```

预期验收：

- `run_loc_online` 可执行文件存在于 `install/lightning_localization/lib/lightning_localization/`。
- `run_loc_offline` 可执行文件存在于 `install/lightning_localization/lib/lightning_localization/`。
- Livox 自定义消息头已生成。

## 离线行为对比

准备与上游 lightning-lm 相同的 config、map 和 bag：

```bash
ros2 run lightning_localization run_loc_offline \
  --config /path/to/default_nclt.yaml \
  --input_bag /path/to/bag \
  --map_path /path/to/data/new_map/
```

使用相同输入与上游包进行对比。记录：

- 初始化状态和日志
- `confidence`
- 定位状态转换
- `Timer::PrintAll()` 的耗时输出
- 失败消息
- 生成的 `recover_pose.txt`

## 在线行为对比

```bash
ros2 run lightning_localization run_loc_online --config /path/to/default_nclt.yaml
```

在传感器或 bag replay 处于活动状态时检查：

- 配置的 IMU topic 被接收
- 配置的点云 topic 被接收
- Livox topic 未与标准点云输入重复，除非这是预期行为
- 当 `system.pub_tf=true` 时发布 `map -> base_link` TF
- 当 `system.with_ui=true` 时 Pangolin UI 启动

## RViz 与 TF

```bash
ros2 run tf2_ros tf2_echo map base_link
rviz2
```

验收：

- TF 时间戳持续前进。
- 初始化后位姿连续。
- 没有相对上游行为无法解释的 frame 名称变化。

## 轨迹与指标

使用相同 bag 和地图导出上游包与剥离包的位姿。比较：

- ATE
- RPE
- confidence 趋势
- 丢失帧数量
- 初始化成功
- 平均和 P95 处理耗时

由于当前 Codex 工作区没有 ROS2、colcon、bag 或 map 数据，所有指标对比均待人工 ROS2 环境验证。

