# RViz2 可视化与初始化操作说明

## 功能目标

本轮增强为 `lightning_localization` 增加 RViz2 可视化配置、launch 自动加载能力、标准 `/initialpose` 接入、外部初始位姿 service，以及初始化状态输出。默认 NDT-OMP 定位链路、`map -> base_link` TF、`/localization/pose`、`/localization/status` 和 `/localization/diagnostics` 保持原有行为。

当前 Codex 环境为 Windows，无 ROS2、无 colcon、无 RViz2 和完整 bag/map 数据。本文所有 launch、RViz2、topic echo 和 service call 步骤均为待人工 ROS2 环境验证。

## RViz2 启动方式

在线 launch 支持按需启动 RViz2：

```bash
ros2 launch lightning_localization loc_online.launch.py \
  config:=/path/to/default.yaml \
  use_rviz:=true
```

默认 RViz2 配置文件安装路径：

```text
share/lightning_localization/rviz/lightning_localization.rviz
```

源码路径：

```text
rviz/lightning_localization.rviz
```

可通过 launch 参数覆盖：

```bash
ros2 launch lightning_localization loc_online.launch.py \
  config:=/path/to/default.yaml \
  use_rviz:=true \
  rviz_config:=/path/to/custom.rviz
```

`ros2 launch` 会自动给定位进程追加 ROS 参数，例如 `--ros-args`、`-r __node:=...` 和 `--params-file ...`。`run_loc_online` 会先通过 rclcpp 移除这些 ROS 参数，再把剩余的 `--config` 交给 gflags 解析，因此 launch 启动不应再出现 `unknown command line flag 'params-file'`、`unknown command line flag 'r'` 或 `unknown command line flag 'ros-args'`。

## Launch 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `config` | `./config/default.yaml` | 定位 YAML 配置文件路径 |
| `use_rviz` | `false` | 为 true 时启动 `rviz2 -d <rviz_config>` |
| `rviz_config` | package share 下的 `rviz/lightning_localization.rviz` | RViz2 配置文件 |
| `use_sim_time` | `false` | 传给定位节点的 ROS 参数，用于真实 ROS2 环境验证 |
| `initial_pose_source` | 空字符串 | 非空时覆盖 YAML `initialization.source`；空值保留 YAML，默认有效值为 `default` |
| `initialpose_topic` | 空字符串 | 非空时覆盖 YAML `initialization.rviz_initialpose.topic`；空值保留 YAML，默认有效值为 `/initialpose` |

## RViz2 面板和工具栏

`rviz/lightning_localization.rviz` 保留完整左侧面板和工具栏，不使用 kiosk/fullscreen 模式。配置包含：

| 面板 | 用途 |
| --- | --- |
| Displays | 添加、删除和调整 TF、Pose、PointCloud2、Path、Marker 等显示 |
| Tool Properties | 查看和调整 `2D Pose Estimate` 等工具的 topic 与 covariance |
| Views | 调整 Orbit 视角、目标 frame 和距离 |
| Time | 查看 ROS time / wall time |
| Selection | 查看选中对象属性 |

## Displays 列表

| Display | 默认状态 | Topic / Frame | 说明 |
| --- | --- | --- | --- |
| Grid | 开启 | fixed frame `map` | 提供地图平面参考 |
| TF | 开启 | `map -> base_link` | 显示定位 TF 树 |
| Localization Pose | 开启 | `/localization/pose` | 显示 `geometry_msgs/msg/PoseStamped` 定位位姿 |
| Localization Odometry | 关闭 | `/localization/odometry` | odometry 默认不发布，启用 `ros_output.publish_odometry` 后再打开 |
| Current Scan | 开启 | `/localization/scan` | 显示最近一次有效 LidarLoc scan 按 `/localization/pose` 同源 final pose 变换到 `map` frame 后的低频点云 |
| Local Map | 开启 | `/localization/map` | 显示当前已加载的局部 tiled runtime map，使用 transient local QoS |
| Localization Markers | 关闭 | `/localization/markers` | 当前包未发布 marker topic |
| Localization Path | 关闭 | `/localization/path` | 当前包未发布 path topic |

`/localization/scan` 不是原始 LiDAR topic，而是定位模块处理后的 scan 可视化输出。系统会缓存最近一次 `LidarLoc` 有效 scan，并在 `/localization/pose` 同源的 PGO final pose 输出路径中低频发布，因此 RViz2 中 scan 的位置与 pose 输出保持一致。`/localization/map` 是当前加载的局部 runtime map，不一定是完整全局地图；它会随 `TiledMap::LoadOnPose()` 的 chunk 加载更新。

左侧 Displays 面板不会隐藏，可以用鼠标拖动面板边界手动收窄到较窄宽度。右侧 dock 不隐藏，`Views` 面板存在；如果你的 RViz2 启动后 `Views` 未停靠在右侧，可在菜单 `Panels -> Views` 打开，并把面板拖到右侧 dock，用于 Orbit、TopDownOrtho 等视角切换。

Diagnostics 没有 RViz2 原生 display，本轮通过 topic echo 或诊断面板工具检查：

```bash
ros2 topic echo /localization/diagnostics
```

## 使用 2D Pose Estimate 设置初始位姿

RViz2 工具栏中的 `2D Pose Estimate` 会发布标准 topic：

```text
/initialpose
```

消息类型：

```text
geometry_msgs/msg/PoseWithCovarianceStamped
```

处理链路：

1. 操作员在 RViz2 中点击 `2D Pose Estimate`。
2. 在地图区域拖动设置 x/y/yaw。
3. RViz2 发布 `/initialpose`。
4. `LocSystem` 检查 `frame_id`、pose 数值和 quaternion。
5. 检查通过后调用 `Localization::SetExternalPose()`，最终进入 `LidarLoc::SetInitialPose()`。
6. `/localization/initialization_status` 更新最近一次请求来源、frame、接受/应用结果和说明。
7. `/localization/diagnostics` 增加 initialization 相关 key-value。

注意：RViz2 pose 被应用后只是设置定位初始猜测，不代表定位已经成功。是否进入 `GOOD` 取决于后续点云、地图加载和 NDT-OMP 匹配结果。

## 外部 Set Initial Pose Service

service 名称：

```text
/lightning_localization/set_initial_pose
```

service 类型：

```text
lightning_localization/srv/SetInitialPose
```

请求字段：

| 字段 | 说明 |
| --- | --- |
| `header.frame_id` | 默认要求为 `map`，由 `initialization.external_pose.accept_frame_id` 配置 |
| `pose` | 初始位姿，quaternion 默认要求为单位四元数 |
| `source` | 来源描述，例如 `operator`、`remote_system`、`external_pose` |
| `apply_immediately` | true 时立即应用为初始猜测 |

调用示例：

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

返回语义：

| 字段 | 说明 |
| --- | --- |
| `success` | 请求是否被接受；当 `apply_immediately=false` 时表示请求被接受但未立即应用 |
| `message` | frame/quaternion 校验结果或应用说明 |
| `status` | `0` 拒绝，`1` 接受并已应用，`2` 接受但未立即应用 |

## 初始化状态 topic

topic 名称：

```text
/localization/initialization_status
```

消息类型：

```text
std_msgs/msg/String
```

内容为稳定 JSON 字符串，字符串字段会进行 JSON 转义，包含：

| 字段 | 含义 |
| --- | --- |
| `current_initialization_source` | 当前初始化来源 |
| `last_request_source` | 最近一次请求来源 |
| `last_request_time` | 最近一次请求时间 |
| `last_request_frame` | 最近一次请求 frame |
| `last_request_accepted` | 最近一次请求是否通过输入校验 |
| `last_request_applied` | 最近一次请求是否已应用为初始猜测 |
| `last_message` | 最近一次请求的说明或失败原因 |
| `localization_status` | 最近一次定位状态 |
| `waiting_for_initial_pose` | 是否等待外部/RViz 初始位姿 |
| `external_pose_service_enabled` | service 是否启用 |
| `rviz_initialpose_enabled` | `/initialpose` 是否启用 |
| `initialpose_topic` | RViz initialpose topic |
| `set_initial_pose_service` | service 名称 |
| `core_initialized` | 定位核心是否初始化成功 |

示例：

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

如果定位核心 `loc_->Init()` 因地图或配置错误失败，在线入口不会继续 `Spin()`；`/initialpose` subscription 和 `/lightning_localization/set_initial_pose` service 不会在核心失败后保留为可正常使用的初始化入口。内部 `ApplyInitialPose()` 也会检查 `core_initialized`，核心未初始化时拒绝请求，并且不会报告 `last_request_applied=true`。

## Diagnostics 新增字段

`/localization/diagnostics` 的 `DiagnosticStatus` 新增：

| Key | 含义 |
| --- | --- |
| `initialization_source` | 当前初始化来源 |
| `last_initialization_source` | 最近一次初始化请求来源 |
| `last_initialization_accepted` | 最近一次请求是否通过校验 |
| `last_initialization_applied` | 最近一次请求是否应用 |
| `last_initialization_message` | 最近一次请求说明 |
| `waiting_for_initial_pose` | 是否等待初始位姿 |
| `initialpose_topic` | RViz initialpose topic |
| `set_initial_pose_service` | 外部初始化 service |

## YAML 配置

默认 YAML 增加：

```yaml
ros_output:
  publish_scan: true
  scan_topic: "/localization/scan"
  scan_pose_source: "final_pose"
  publish_map: true
  map_topic: "/localization/map"
  scan_min_period_sec: 0.5
  scan_timestamp_tolerance_sec: 0.3
  scan_publish_only_on_new_scan: true
  map_min_period_sec: 1.0

visualization:
  enable_rviz: false
  rviz_config: "rviz/lightning_localization.rviz"
  fixed_frame: "map"
  base_frame: "base_link"
  show_pose_topic: true
  show_odometry_topic: false
  show_tf: true
  show_scan_topic: true
  show_map_topic: true

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

`visualization.enable_rviz` 供文档和 launch 使用；C++ 节点不会自行启动 RViz2。RViz2 由 launch 的 `use_rviz` 控制。`initial_pose_source` 和 `initialpose_topic` launch 参数会作为 ROS 参数传给节点；只有非空时才覆盖 YAML 中的初始化来源和 RViz initialpose topic。

## 人工 ROS2/RViz2 验证入口

完整验证步骤见 `docs/rviz2-manual-validation.md`。当前 Windows Codex 环境未执行 ROS2 编译、launch、RViz2 打开、`2D Pose Estimate`、rosbag 回放或 topic/service 实测。
