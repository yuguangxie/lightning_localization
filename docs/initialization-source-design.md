# 初始化来源设计

## 当前初始化逻辑审计

阶段一在线入口原先在 `run_loc_online.cc` 中固定调用：

```cpp
loc.SetInitPose(SE3());
```

`LocSystem::SetInitPose()` 会调用 `Localization::SetExternalPose()`，后者继续调用 `LidarLoc::SetInitialPose()`。该调用设置的是后续 scan-to-map 定位的初始位姿或初始猜测，并不代表 NDT-OMP 已经匹配成功。

本轮增强把启动初始化入口改为：

```cpp
loc.ApplyConfiguredInitialPose();
```

默认 YAML 中 `initialization.source: "default"`，且 `default_pose.pose` 为 identity pose，因此默认行为仍等价于原来的 `SetInitPose(SE3())`。

`run_loc_online` 现在要求 `LocSystem::Init()` 成功后才进入启动初始化和 `Spin()`。如果 `loc_->Init()` 因地图、配置或依赖错误失败，进程会调用 `rclcpp::shutdown()` 并返回非零，避免在定位核心不可用时仍暴露可调用的初始化接口。

## 新增初始化来源

| 来源 | 配置值 | 行为 | 默认启用 |
| --- | --- | --- | --- |
| 默认启动初始位姿 | `default` | 使用 `initialization.default_pose.pose` 调用 `SetExternalPose()` | 是 |
| 外部 service pose | `external_pose` | 等待 `/lightning_localization/set_initial_pose` 请求 | service 默认创建 |
| RViz initialpose | `rviz_initialpose` | 订阅 `/initialpose`，接收 RViz2 `2D Pose Estimate` | 订阅默认创建 |
| functional point | `functional_point` | 不注入 pose，允许 LidarLoc 继续使用地图功能点初始化逻辑 | 保留 |

## 状态机语义

| 状态字段 | 语义 |
| --- | --- |
| `current_initialization_source` | 当前配置或最近应用的初始化来源 |
| `waiting_for_initial_pose` | 选择外部/RViz 且未应用 pose 时为 true |
| `last_request_accepted` | 最近一次请求通过 frame、数值和 quaternion 校验 |
| `last_request_applied` | 最近一次请求已调用 `Localization::SetExternalPose()` |
| `last_message` | 最近一次请求的说明或失败原因 |
| `core_initialized` | 定位核心 `loc_->Init()` 是否已经成功 |

关键语义边界：

- `accepted=true` 只表示输入合法。
- `applied=true` 只表示 pose 已应用为初始猜测。
- 真实定位成功仍以 `/localization/status`、`/localization/diagnostics` 和后续 `LocalizationResult` 为准。
- 缺少地图、点云或匹配失败时，service/RViz 请求不能被解释为定位成功。
- 核心未初始化时，`/initialpose` subscription 和 set initial pose service 不会创建；即使内部路径被调用，`ApplyInitialPose()` 也会返回失败，并设置 `accepted=false`、`applied=false`。
- `/localization/initialization_status` 的 payload 是 JSON 字符串，字符串字段会转义，service 请求中的 `source` 即使包含空格、引号或换行也不会破坏解析格式。

## 默认初始位姿

配置：

```yaml
initialization:
  source: "default"
  default_pose:
    enabled: true
    pose: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0]
```

pose 数组格式为：

```text
x, y, z, qx, qy, qz, qw
```

默认四元数为单位四元数。若配置中默认四元数非法，代码会回退为 identity quaternion，避免启动时产生 NaN pose。

## 外部 service pose

service：

```text
/lightning_localization/set_initial_pose
```

类型：

```text
lightning_localization/srv/SetInitialPose
```

service 请求会被转换为内部 `SE3`，再调用 `Localization::SetExternalPose()`。如果 `apply_immediately=false`，系统只记录请求，不执行延迟队列；状态会明确显示 `accepted but not applied`。

service 只在定位核心初始化成功后创建。若核心初始化失败，在线入口不会继续 spin，因此外部系统不会得到“pose 已应用”的误导性响应。`ApplyInitialPose()` 仍保留核心状态检查，作为防御式保护。

## RViz initialpose

topic：

```text
/initialpose
```

类型：

```text
geometry_msgs/msg/PoseWithCovarianceStamped
```

`LocSystem` 订阅该 topic 后执行同一套校验与应用逻辑。默认 `preserve_default_behavior=true`，当 `initialization.source=rviz_initialpose` 时仍可先应用默认位姿，之后允许操作员通过 RViz2 手动重设初始猜测。

## functional point

`functional_point` 来源保留原 `LidarLoc` 基于地图功能点的初始化路线。本轮不修改功能点格式、不修改地图加载、不改变 NDT-OMP 匹配逻辑。选择该来源时，`LocSystem` 会允许定位输入继续进入 `Localization`，实际初始化成功与否由后续地图功能点和匹配结果决定。

## 安全检查

所有来自 service 或 `/initialpose` 的 pose 均执行：

| 检查 | 规则 |
| --- | --- |
| frame_id | 默认必须等于 `map`，由 `accept_frame_id` 配置 |
| NaN/Inf | position 和 orientation 任一字段非有限数时拒绝 |
| 全零 quaternion | 拒绝 |
| 非单位 quaternion | `require_valid_quaternion=true` 时拒绝；否则归一化后使用 |
| apply_immediately | 为 false 时记录请求但不调用定位初始化 |

## 与定位模块函数的关系

| 函数 | 本轮关系 |
| --- | --- |
| `LocSystem::ApplyConfiguredInitialPose()` | 启动时读取 YAML，决定默认、等待外部、等待 RViz 或 functional point |
| `LocSystem::SetInitPose()` | 保留公开接口，内部复用统一 `ApplyInitialPose()` |
| `LocSystem::ApplyInitialPose()` | 统一应用入口，先检查 `core_initialized_`，再区分 accepted、applied 和 localization success |
| `Localization::SetExternalPose()` | 继续作为外部 pose 注入到定位核心的入口 |
| `LidarLoc::SetInitialPose()` | 保持原语义，设置 scan-to-map 初始位姿 |

## 后续阶段二可扩展项

阶段二可以在不破坏默认 NDT-OMP 的前提下扩展：

| 扩展项 | 建议 |
| --- | --- |
| 初始化请求队列 | 为 `apply_immediately=false` 增加明确队列和触发条件 |
| structured status msg | 将 `std_msgs/String` 迁移为版本化自定义状态消息 |
| 多候选重定位 | 增加候选 pose 列表和质量评分 |
| 初始化成功判定 | 将 pose accepted、initial guess applied、NDT converged、localization GOOD 分成独立状态 |
| lifecycle | 将等待初始位姿、运行、失效、关闭纳入 ROS2 lifecycle |
