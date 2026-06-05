# RViz2 可视化与初始化来源增强报告

## 本轮目标

本轮在阶段一独立包和 ROS2 topic 输出补强基础上，增加 RViz2 可视化配置、launch 自动加载 RViz2、标准 `/initialpose` 接入、外部初始位姿 service、初始化状态 topic 和 diagnostics 字段。此工作不是阶段二匹配后端重构，未引入 `RegistrationBackend`，未接入 small_gicp、fast_gicp、VGICP 或 gtsam_points，未改变默认 NDT-OMP 定位行为。

## 修改文件清单

| 文件 | 类型 | 内容 |
| --- | --- | --- |
| `docs/rviz2-and-initialization-source-audit.md` | 新增文档 | 现状审计 |
| `rviz/lightning_localization.rviz` | 新增配置 | RViz2 默认布局 |
| `launch/loc_online.launch.py` | 修改 | 增加 `use_rviz`、`rviz_config`、`use_sim_time` 等 launch 参数 |
| `srv/SetInitialPose.srv` | 新增接口 | 外部初始位姿 service |
| `src/core/system/loc_system.h` | 修改 | 初始化配置、核心初始化状态、subscription、service、publisher 成员 |
| `src/core/system/loc_system.cc` | 修改 | `/initialpose`、service、状态输出、diagnostics 集成；核心未初始化时拒绝 pose 应用 |
| `src/app/run_loc_online.cc` | 修改 | 启动初始化改为配置驱动；兼容 ROS2 launch 自动参数；`LocSystem::Init()` 失败时退出，不继续 `Spin()` |
| `src/app/run_loc_offline.cc` | 修改 | 兼容 ROS2 launch 自动参数，避免 gflags 解析 `--ros-args`、`-r`、`--params-file` |
| `CMakeLists.txt` | 修改 | 生成 `SetInitialPose.srv`，安装 `rviz/`，导出 `rosidl_default_runtime` |
| `package.xml` | 修改 | 增加 `rviz2` 运行依赖 |
| `config/default*.yaml` | 修改 | 增加 `visualization` 和 `initialization` 配置 |
| `docs/rviz2-visualization-and-initialization.md` | 新增文档 | 使用说明 |
| `docs/initialization-source-design.md` | 新增文档 | 初始化来源设计 |
| `docs/rviz2-manual-validation.md` | 新增文档 | 人工验证流程 |
| `README.md`、`docs/config-parameters.md`、`docs/ros2-output-topics.md`、`docs/manual-ros2-validation.md` | 更新 | 同步新增 topic、service、RViz2 和验证说明 |

## 新增 launch 参数

| 参数 | 默认值 | 说明 |
| --- | --- | --- |
| `use_rviz` | `false` | 为 true 时启动 RViz2 |
| `rviz_config` | package share 下 `rviz/lightning_localization.rviz` | RViz2 配置路径 |
| `use_sim_time` | `false` | 传给定位节点的 ROS 参数 |
| `initial_pose_source` | 空字符串 | 非空时覆盖 YAML `initialization.source`；空值保留 YAML |
| `initialpose_topic` | 空字符串 | 非空时覆盖 YAML `initialization.rviz_initialpose.topic`；空值保留 YAML |

`ros2 launch` 会向 C++ 可执行文件自动追加 ROS 参数，例如 `--ros-args`、`-r __node:=...` 和 `--params-file ...`。`run_loc_online` 现在使用 rclcpp 先移除 ROS 参数，再用 gflags 解析 `--config`；`run_loc_offline` 也先移除 ROS 参数，再解析 `--config`、`--input_bag` 和 `--map_path`。因此 launch 模式不应再因 `unknown command line flag 'params-file'`、`unknown command line flag 'r'` 或 `unknown command line flag 'ros-args'` 退出。

## 新增 YAML 参数

| 配置段 | 说明 |
| --- | --- |
| `visualization` | RViz2 配置路径、fixed frame、默认展示意图 |
| `initialization.source` | `default`、`external_pose`、`rviz_initialpose`、`functional_point` |
| `initialization.default_pose` | 默认启动初始位姿 |
| `initialization.external_pose` | service 名称、frame 约束、quaternion 校验、立即应用开关 |
| `initialization.rviz_initialpose` | `/initialpose` topic、frame 约束、quaternion 校验、默认行为保留 |
| `initialization.status` | `/localization/initialization_status` 输出 |

## 新增 RViz2 配置

`rviz/lightning_localization.rviz` 包含：

- Global Fixed Frame: `map`
- TF display
- Pose display: `/localization/pose`
- Odometry display: `/localization/odometry`，默认关闭
- PointCloud2 scan/map display，当前包未发布对应 topic，默认关闭
- MarkerArray/Path display，当前包未发布对应 topic，默认关闭
- `2D Pose Estimate` 工具发布 `/initialpose`
- Displays、Tool Properties、Views、Time、Selection 面板保留

## 新增 topic 与 service

| 名称 | 类型 | 默认状态 | 说明 |
| --- | --- | --- | --- |
| `/localization/initialization_status` | `std_msgs/msg/String` | 开启 | 输出初始化来源和最近请求状态 |
| `/initialpose` | `geometry_msgs/msg/PoseWithCovarianceStamped` | 订阅开启 | RViz2 `2D Pose Estimate` 标准 topic |
| `/lightning_localization/set_initial_pose` | `lightning_localization/srv/SetInitialPose` | service 开启 | 外部 pose 注入 |

## 初始化来源说明

默认 `initialization.source=default` 会应用 identity pose，保持原在线入口启动行为。`external_pose` 可以等待外部 service 请求；`rviz_initialpose` 可以接收 RViz2 手动初始位姿，默认 `preserve_default_behavior=true`，因此不会因为等待 RViz 操作而破坏默认启动行为；`functional_point` 保留原 LidarLoc 功能点初始化路线。

service 或 `/initialpose` 请求通过 frame、NaN/Inf、全零四元数和单位四元数检查后，才会应用为初始猜测。请求成功不等于定位成功，定位是否成功仍由后续 LiDAR、地图和 NDT-OMP 匹配决定。

`LocSystem::Init()` 只有在 `loc_->Init()` 成功后才创建 `/initialpose` subscription 和 `/lightning_localization/set_initial_pose` service。若地图、配置或定位核心初始化失败，在线入口会调用 `rclcpp::shutdown()` 并返回非零，不继续正常 `Spin()`。`ApplyInitialPose()` 也会检查 `core_initialized_`，核心未初始化时返回失败并记录 `localization core is not initialized; initial pose was rejected`，不会报告 `applied=true` 或 localization success。

`/localization/initialization_status` 使用 `std_msgs/msg/String` 承载 JSON 字符串。`last_request_source`、`last_request_frame`、`last_message`、topic 和 service 名称等字符串字段会进行 JSON 转义，避免 service 自由字符串中的空格、引号或换行破坏解析。

## 本轮审查 finding 修复

| Finding | 修复 |
| --- | --- |
| P1：核心初始化失败后仍可能保留初始化接口且在线入口继续 spin | `CreateInitializationInterfaces()` 移到 `loc_->Init()` 成功之后；新增 `core_initialized_`；`ApplyInitialPose()` 在核心未初始化时拒绝；`run_loc_online` 初始化失败后 shutdown 并返回 `1` |
| P2：缺少 `ament_export_dependencies(rosidl_default_runtime)` | `CMakeLists.txt` 在 `ament_package()` 前增加 `ament_export_dependencies(rosidl_default_runtime)` |
| P2：`initialization_status` 自由字符串未转义 | `MakeInitializationStatusString()` 改为 JSON 输出并对字符串字段进行转义 |

## 静态审计结果

| 检查项 | 结果 | 证据 |
| --- | --- | --- |
| RViz2 配置存在 | OK | `rviz/lightning_localization.rviz` |
| launch 支持 `use_rviz` | OK | `launch/loc_online.launch.py` |
| launch 启动 `rviz2 -d` | OK | `Node(package="rviz2", executable="rviz2", arguments=["-d", rviz_config])` |
| 在线入口兼容 launch 自动 ROS 参数 | OK | `run_loc_online` 使用 `rclcpp::init_and_remove_ros_arguments()` 后再调用 gflags |
| 离线入口兼容 launch 自动 ROS 参数 | OK | `run_loc_offline` 使用 `rclcpp::remove_ros_arguments()` 后再调用 gflags |
| CMake 安装 `rviz/` | OK | `install(DIRECTORY ... rviz ...)` |
| YAML 有 `visualization` | OK | 所有 `config/default*.yaml` |
| YAML 有 `initialization` | OK | 所有 `config/default*.yaml` |
| `/initialpose` 订阅代码存在 | OK | `create_subscription<geometry_msgs::msg::PoseWithCovarianceStamped>` |
| `SetInitialPose` service 存在 | OK | `srv/SetInitialPose.srv` 和 `create_service` |
| 核心初始化失败后不继续 spin | OK | `run_loc_online.cc` 在 `LocSystem::Init()` 失败时 `rclcpp::shutdown(); return 1;` |
| 核心未初始化时不应用 pose | OK | `ApplyInitialPose()` 检查 `core_initialized_`，失败时 `accepted=false`、`applied=false` |
| initialization status topic | OK | `/localization/initialization_status` publisher，payload 为 JSON 字符串 |
| initialization status 字符串转义 | OK | `JsonEscape()` 覆盖引号、反斜杠、换行、制表符和控制字符 |
| `rosidl_default_runtime` 导出 | OK | `ament_export_dependencies(rosidl_default_runtime)` 位于 `ament_package()` 前 |
| diagnostics 初始化字段 | OK | `initialization_source`、`last_initialization_*`、`waiting_for_initial_pose` |
| 默认初始化行为 | OK | `initialization.source=default` + identity pose |
| 原 TF 输出保留 | OK | `SetTFCallback` 与 `sendTransform` 未删除 |
| 原 pose/status/diagnostics topic 保留 | OK | 原 publisher 和 topic 配置保留 |
| 阶段二后端重构 | OK | 未引入 RegistrationBackend 或新匹配后端 |
| 虚假 ROS2/RViz2 验证声明 | OK | 文档均标记待人工 ROS2 环境验证 |

## 当前 Windows 环境未执行项

- 未执行 `colcon build --packages-select lightning_localization`。
- 未运行 `ros2 launch lightning_localization loc_online.launch.py use_rviz:=true`。
- 未打开 RViz2。
- 未验证 `2D Pose Estimate` 实际发布 `/initialpose`。
- 未执行 service call。
- 未回放 rosbag。
- 未验证 RViz2 中 TF、pose、odometry、scan/map display 的实际显示效果。

## 待人工 ROS2/RViz2 验证步骤

1. 在 Ubuntu 22.04 + ROS2 Humble 环境中构建包。
2. 确认 `SetInitialPose.srv` 接口生成。
3. 使用 `use_rviz:=true` 启动在线定位和 RViz2。
4. 检查 RViz2 面板和工具栏是否完整。
5. 检查 TF 和 `/localization/pose`。
6. 使用 service 注入初始位姿。
7. 使用 RViz2 `2D Pose Estimate` 发布 `/initialpose`。
8. 检查 `/localization/initialization_status` 和 `/localization/diagnostics`。
9. 接入真实 bag/map 后确认定位状态是否能进入 `GOOD`。

## 风险和限制

| 风险 | 说明 | 处理建议 |
| --- | --- | --- |
| launch 参数与 YAML 覆盖关系 | `initial_pose_source` 和 `initialpose_topic` 仅在非空时覆盖 YAML 对应值，其效果仍需 ROS2 launch 实测 | 人工验证时同时检查 YAML 默认和 launch 覆盖两种启动方式 |
| scan/map display 默认关闭 | 当前包未发布 `/localization/scan` 或 `/localization/map` | 不将 RViz2 display 视作实际发布能力 |
| pose 注入不等于定位成功 | service/RViz 请求只设置初始猜测 | 使用后续 `/localization/status` 和 diagnostics 判断定位是否成功 |
| RViz2 配置兼容性 | RViz2 配置文件格式需在真实 RViz2 版本中加载验证 | 待人工 ROS2/RViz2 验证 |

## 是否可以进入人工 ROS2 验证

可以进入人工 ROS2/RViz2 验证。当前静态审计未发现阶段二后端重构、TF 删除、topic 删除或虚假验证声明，但真实编译和运行仍必须在 Ubuntu/ROS2/colcon 环境确认。
