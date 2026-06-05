# RViz2 与初始化来源人工验证流程

## 环境前提

本流程必须在 Ubuntu 22.04 + ROS2 Humble 或兼容环境中执行。当前 Windows Codex 环境没有 ROS2、colcon、RViz2、完整 bag/map 数据，因此本文命令未在当前环境执行。

## 构建

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-select lightning_localization
source install/setup.bash
```

确认接口生成：

```bash
ros2 interface show lightning_localization/srv/SetInitialPose
```

## 启动在线定位和 RViz2

```bash
ros2 launch lightning_localization loc_online.launch.py \
  config:=/path/to/default.yaml \
  use_rviz:=true
```

若需要指定 RViz2 配置：

```bash
ros2 launch lightning_localization loc_online.launch.py \
  config:=/path/to/default.yaml \
  use_rviz:=true \
  rviz_config:=/path/to/lightning_localization.rviz
```

启动时不应再出现以下 gflags 错误：

```text
unknown command line flag 'params-file'
unknown command line flag 'r'
unknown command line flag 'ros-args'
```

这些参数由 ROS2 launch 自动追加，`run_loc_online` 应在解析 gflags 前移除它们。若仍出现该错误，说明运行的 install 空间不是最新构建结果，需要重新执行 `colcon build --packages-select lightning_localization` 并重新 `source install/setup.bash`。

## ROS2 节点、topic 和 service 检查

```bash
ros2 node list
ros2 topic list
ros2 service list
```

检查定位输出：

```bash
ros2 topic echo /localization/pose
ros2 topic echo /localization/status
ros2 topic echo /localization/diagnostics
ros2 topic echo /localization/initialization_status
ros2 topic echo /localization/scan
ros2 topic echo /localization/map
ros2 run tf2_ros tf2_echo map base_link
```

检查 `/initialpose`：

```bash
ros2 topic echo /initialpose
```

如果启用 odometry：

```bash
ros2 topic echo /localization/odometry
```

检查频率：

```bash
ros2 topic hz /localization/pose
ros2 topic hz /localization/status
ros2 topic hz /localization/diagnostics
ros2 topic hz /localization/initialization_status
ros2 topic hz /localization/scan
ros2 topic hz /localization/map
```

`/localization/scan` 默认是低频 RViz2 可视化输出，使用最近一次有效 LidarLoc scan 和 `/localization/pose` 同源 final pose 配对发布；默认 `scan_min_period_sec=0.5`，因此 `ros2 topic hz /localization/scan` 预期不会接近原始 LiDAR 频率。

## Service 调用验证

```bash
ros2 service call /lightning_localization/set_initial_pose lightning_localization/srv/SetInitialPose "{
  header: {frame_id: 'map'},
  pose: {
    position: {x: 0.0, y: 0.0, z: 0.0},
    orientation: {x: 0.0, y: 0.0, z: 0.0, w: 1.0}
  },
  source: 'operator',
  apply_immediately: true
}"
```

预期：

- response `success=true`。
- `/localization/initialization_status` 是 JSON 字符串。
- JSON 字段 `"last_request_source":"operator"`。
- JSON 字段 `"last_request_accepted":true`。
- JSON 字段 `"last_request_applied":true`。
- JSON 字段 `"core_initialized":true`。
- JSON 字段 `"last_message"` 说明 pose 已作为 initial guess 应用。
- `/localization/diagnostics` 中出现 initialization 相关 key。

异常输入验证：

```bash
ros2 service call /lightning_localization/set_initial_pose lightning_localization/srv/SetInitialPose "{
  header: {frame_id: 'odom'},
  pose: {
    position: {x: 0.0, y: 0.0, z: 0.0},
    orientation: {x: 0.0, y: 0.0, z: 0.0, w: 1.0}
  },
  source: 'operator_bad_frame',
  apply_immediately: true
}"
```

预期：请求被拒绝，message 指出 frame_id 不匹配。

自由字符串转义验证：

```bash
ros2 service call /lightning_localization/set_initial_pose lightning_localization/srv/SetInitialPose "{
  header: {frame_id: 'map'},
  pose: {
    position: {x: 0.0, y: 0.0, z: 0.0},
    orientation: {x: 0.0, y: 0.0, z: 0.0, w: 1.0}
  },
  source: 'operator \"quoted\" source',
  apply_immediately: true
}"
```

预期：`/localization/initialization_status` 仍是可解析 JSON，`last_request_source` 中的引号被正确转义。

## `/initialpose` 命令行验证

```bash
ros2 topic pub --once /initialpose geometry_msgs/msg/PoseWithCovarianceStamped "{
  header: {frame_id: 'map'},
  pose: {
    pose: {
      position: {x: 1.0, y: 2.0, z: 0.0},
      orientation: {x: 0.0, y: 0.0, z: 0.0, w: 1.0}
    }
  }
}"
```

预期：

- `/initialpose` 有一次消息。
- `/localization/initialization_status` JSON 更新为 `"last_request_source":"rviz_initialpose_topic"`。
- JSON 字段 `"last_request_accepted":true`。
- JSON 字段 `"last_request_applied":true`。

## RViz2 手工步骤

1. 确认 RViz2 正常打开。
2. 确认左侧 Displays 面板可见。
3. 确认 Tool Properties、Views、Time 面板可见。
4. 确认 Global Options 的 Fixed Frame 是 `map`。
5. 确认 TF display 已开启。
6. 确认 Pose display 指向 `/localization/pose`。
7. 确认 Current Scan display 指向 `/localization/scan`，并可调整 Size、Style、Color Transformer、Decay Time。
8. 确认 Local Map display 指向 `/localization/map`，并可调整 Size、Style、Color Transformer、Decay Time。
9. 如果启用 odometry，打开 Odometry display 并确认 topic 为 `/localization/odometry`。
10. 用鼠标拖动左侧 Displays 面板边界，确认可以手动收窄。
11. 如果右侧 Views 面板未显示，通过 `Panels -> Views` 打开并拖到右侧 dock，确认可以切换 Orbit/TopDownOrtho 视角。
12. 点击 `2D Pose Estimate`。
13. 在地图区域拖动设置 x/y/yaw。
14. 检查 `/initialpose` 是否收到消息。
15. 检查 `/localization/initialization_status` 是否更新。
16. 检查 `/localization/diagnostics` 是否包含 initialization 字段。
17. 回放 bag 或接入真实传感器后，观察 `/localization/status` 是否从 `INITIALIZING` 进入 `GOOD`，并确认 `/localization/scan` 与 `/localization/map` 在 RViz2 中同时可见。

## 成功判据

| 项 | 成功标准 |
| --- | --- |
| RViz2 自动启动 | `use_rviz:=true` 后 RViz2 打开并加载指定配置 |
| 左侧边栏 | Displays、Tool Properties、Views、Time 可见 |
| TF | `map -> base_link` 可观察，且 `system.pub_tf=true` 时未被破坏 |
| Pose | `/localization/pose` 可显示 |
| Scan | `/localization/scan` 可显示，点云位于 map frame 下 |
| Local Map | `/localization/map` 可显示当前已加载局部 runtime map |
| Service | 合法请求被接受并更新 initialization status |
| `/initialpose` | RViz2 和命令行发布均能触发初始化状态更新 |
| Diagnostics | initialization key-value 存在 |
| 定位成功 | 真实点云和地图输入后由后续 `GOOD` 状态确认，不由 pose 注入直接确认 |

## 核心初始化失败验证

使用一个不存在的地图路径或无效配置启动：

```bash
ros2 launch lightning_localization loc_online.launch.py \
  config:=/path/to/invalid-map-config.yaml \
  use_rviz:=false
```

预期：

- `LocSystem::Init()` 返回失败后进程退出，不进入正常 `Spin()`。
- `/lightning_localization/set_initial_pose` 不应作为可用 service 长时间存在。
- `/initialpose` subscription 不应作为可用输入链路存在。
- 不应出现 `success=true`、`last_request_applied=true` 或 localization success 的状态输出。
- 日志应能看到定位核心初始化失败相关信息。
