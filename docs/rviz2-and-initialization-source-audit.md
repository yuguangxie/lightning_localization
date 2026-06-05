# RViz2 与初始化来源现状审计

## 审计范围

本审计覆盖当前 `lightning_localization` 阶段一独立 ROS2 包的 launch、配置、RViz2 可视化、ROS2 输出、初始化入口和初始化状态可观测性。当前 Codex 环境为 Windows，无 ROS2、无 colcon、无完整 rosbag/map 数据，因此本文仅记录源码、配置、构建文件和文档静态审计结论；ROS2 编译、launch、RViz2、rosbag 回放和 TF/topic 运行检查均为待人工 ROS2 环境验证。

## 当前 launch 文件清单

| 文件 | 入口 | 当前能力 | RViz2 相关能力 |
| --- | --- | --- | --- |
| `launch/loc_online.launch.py` | `run_loc_online` | 传入 `--config`，启动在线定位节点 | 未声明 `use_rviz`，未启动 `rviz2` |
| `launch/loc_offline.launch.py` | `run_loc_offline` | 传入 `--config`、`--input_bag`、`--map_path`，启动离线回放入口 | 未声明 `use_rviz`，未启动 `rviz2` |

## 当前 RViz2 配置状态

| 项 | 结果 | 证据 |
| --- | --- | --- |
| RViz2 配置目录 | 未找到 | 当前包内无 `rviz/` 目录 |
| 默认 RViz2 配置文件 | 未找到 | 未发现 `*.rviz` |
| launch 自动启动 RViz2 | 未实现 | launch 文件未引用 `rviz2`、`IfCondition`、`rviz_config` |
| RViz2 左侧 Displays/Tool Properties/Views 面板保留策略 | 未实现 | 无 RViz2 配置文件可审计 |
| `2D Pose Estimate` 工具 | 未实现 | 无 RViz2 配置文件，且代码未订阅 `/initialpose` |

## 当前可视化相关 ROS2 输出

| 输出 | 类型 | 当前是否存在 | 发布位置 | 触发条件 | 备注 |
| --- | --- | --- | --- | --- | --- |
| `map -> base_link` | TF | 存在 | `LocSystem::Init()` 设置 `SetTFCallback`，回调中调用 `sendTransform` | `system.pub_tf=true` 且定位链路输出 TF | 原阶段一主 TF 输出，必须保留 |
| `/localization/pose` | `geometry_msgs/msg/PoseStamped` | 存在 | `LocSystem::CreateRosOutputPublishers()` / `PublishLocalizationTopics()` | `LocalizationResult.valid_` 为 true，或 `publish_invalid_result=true` | topic 名称来自 `ros_output.pose_topic` |
| `/localization/status` | `std_msgs/msg/String` | 存在 | `LocSystem::CreateRosOutputPublishers()` / `PublishLocalizationTopics()` | 定位结果回调，按 `status_min_period_sec` 节流 | 输出定位状态、valid、confidence 等 |
| `/localization/diagnostics` | `diagnostic_msgs/msg/DiagnosticArray` | 存在 | `LocSystem::CreateRosOutputPublishers()` / `PublishLocalizationTopics()` | 定位结果回调，按 `diagnostics_min_period_sec` 节流 | 当前未包含初始化来源字段 |
| `/localization/odometry` | `nav_msgs/msg/Odometry` | 可选存在 | `LocSystem::CreateRosOutputPublishers()` / `PublishLocalizationTopics()` | `ros_output.publish_odometry=true` 且结果可发布 | 默认关闭，covariance 为静态配置语义 |
| scan point cloud topic | `sensor_msgs/msg/PointCloud2` | 未作为输出发布 | 当前仅订阅 `common.lidar_topic` | 无输出触发 | 可在 RViz2 中预留 disabled display，但不能声称已发布 |
| map/local map point cloud topic | `sensor_msgs/msg/PointCloud2` | 未找到输出 publisher | 未发现 map cloud publisher | 无输出触发 | Pangolin UI 路径不等同于 ROS2 map topic |
| Marker/Path | `visualization_msgs` / `nav_msgs/Path` | 未找到输出 publisher | 相关 callback 在 `Localization` 中为注释状态 | 无输出触发 | 可在 RViz2 中预留 disabled display |

## 当前 RViz2 能显示和不能显示的内容

当前如果用户手工启动 RViz2 并手工添加显示项，理论上可观察 `map -> base_link` TF、`/localization/pose`、可选 `/localization/odometry`。但当前包没有安装 RViz2 配置文件，也没有 launch 自动启动 RViz2，因此无法提供开箱即用的可视化布局。

当前不能直接显示由本包发布的 map cloud、scan cloud、path 或 marker，因为静态审计未发现这些输出 publisher。RViz2 配置可以预留这些 display 并默认关闭，待后续确实发布对应 topic 后再启用。

## 当前 `/initialpose` 与外部 pose 注入状态

| 项 | 当前状态 | 证据 |
| --- | --- | --- |
| `/initialpose` 订阅 | 未实现 | 未发现 `create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>` 或 `initialpose` 关键词 |
| 标准 RViz2 `2D Pose Estimate` 接入 | 未实现 | 未订阅 `/initialpose` |
| 外部 pose service | 未实现 | 未发现 `create_service`；`srv/LocCmd.srv` 存在但未在在线入口注册 service |
| 外部 pose action | 未实现 | 未发现 `create_action_server` |
| `SetInitPose` 在线默认接入 | 已接入 | `run_loc_online.cc` 在 `Init()` 后调用 `LocSystem::SetInitPose(SE3())` |
| `SetExternalPose` 内部能力 | 已存在 | `Localization::SetExternalPose()` 调用 `LidarLoc::SetInitialPose()` |
| 初始化来源可配置 | 未实现 | 默认启动位姿硬编码在 `run_loc_online.cc` |
| 初始化状态 topic | 未实现 | 仅有定位 status topic，无 initialization status topic |
| diagnostics 初始化字段 | 未实现 | 当前 diagnostics key-value 未包含 initialization source/request 字段 |

## `SetInitPose` / `SetExternalPose` 语义

`LocSystem::SetInitPose()` 当前会调用 `Localization::SetExternalPose()` 并将 `loc_started_` 设为 true。`Localization::SetExternalPose()` 继续调用 `LidarLoc::SetInitialPose()`，其语义是设置后续 scan-to-map 定位的初始位姿/初始猜测，并不等价于定位已经成功。真实定位是否进入 `GOOD` 仍取决于后续 LiDAR 输入、地图加载和匹配结果。

## 当前初始化来源和状态可观测性

当前可观测性不足。用户可以从 `/localization/status` 和 `/localization/diagnostics` 看到定位状态、valid、confidence、lidar_loc_valid 等信息，但无法区分初始化来源是默认位姿、functional point、外部服务还是 RViz2 `/initialpose`。最近一次初始化请求的来源、frame、是否接受、是否已应用和失败原因也没有独立输出。

## 需要新增的文件和修改点

| 类别 | 需要新增或修改 | 目标 |
| --- | --- | --- |
| RViz2 配置 | 新增 `rviz/lightning_localization.rviz` | 提供保留左侧面板和工具栏的默认 RViz2 布局 |
| launch | 更新 `launch/loc_online.launch.py` | 增加 `use_rviz`、`rviz_config`、`use_sim_time`、`initial_pose_source`、`initialpose_topic` 等参数，并在 `use_rviz=true` 时启动 RViz2 |
| CMake | 更新 `CMakeLists.txt` | 安装 `rviz/`，新增 service 时生成接口 |
| package.xml | 更新依赖 | 增加 RViz2 运行依赖，保持 geometry/std/diagnostic/nav/rosidl 依赖闭合 |
| YAML | 更新所有默认 YAML | 增加 `visualization` 和 `initialization` 配置段，保持缺省值安全 |
| service | 新增 `srv/SetInitialPose.srv` | 提供外部 pose 注入入口 |
| C++ | 更新 `LocSystem` 和在线入口 | 订阅 `/initialpose`、创建 service、发布 initialization status、扩展 diagnostics，按配置选择启动初始化来源 |
| 文档 | 新增和更新 docs | 说明 RViz2、初始化来源、人工验证和静态审计结果 |

