# lightning_localization 定位模块深度审计

检索日期：2026-06-04  
审计环境：Windows Codex 客户端，当前环境无 ROS2、无 colcon、无完整 rosbag/map 数据。本文只基于源码、配置、构建文件、既有文档和官方公开资料做静态审计；ROS2 编译、节点运行、rosbag 回放、RViz2/Pangolin 观察均未在本机执行，均列为“待人工 ROS2 环境验证”。

## 审计范围

本轮审计对象是阶段一剥离后的独立 ROS2 定位包 `lightning_localization`。审计覆盖项目级文件、定位主链路、地图加载、NDT-OMP scan-to-map、LIO/DR 输入、PGO 高频输出、TF、ROS2 topic、service 定义、YAML 配置、线程和资源释放、debug 写盘以及阶段二工业化风险。

已复核的项目级文件和目录：

| 路径 | 静态审计结论 |
| --- | --- |
| `README.md` | 存在，说明阶段一独立包、在线/离线入口、topic 输出和未执行 ROS2 实测限制。 |
| `package.xml` | 存在，声明 ROS2、消息、PCL、tf2、rosbag2、rosidl 等依赖。 |
| `CMakeLists.txt` | 存在，生成 `LocCmd.srv`、`SaveMap.srv` 和 Livox 消息，安装 config/launch/docs/scripts。 |
| `cmake/` | 存在，`packages.cmake` 集中 `find_package`，`packages-link.cmake` 管理第三方链接。 |
| `config/` | 存在，多套默认 YAML 均包含 `ros_output` 段。 |
| `launch/` | 存在，用于 ROS2 launch 静态组织，运行仍需人工 ROS2 环境验证。 |
| `msg/` | 存在，保留 Livox 自定义消息定义和 README。 |
| `srv/` | 存在，保留 `LocCmd.srv`、`SaveMap.srv`。 |
| `scripts/` | 存在，包含静态检查脚本和依赖安装脚本。 |
| `docs/` | 存在，已有阶段一、topic 输出和人工验证文档。 |
| `codex-localization-industrialization/HARD_REQUIREMENTS.md` | 未找到。 |
| `AGENTS.md` | 存在，要求静态审计不能冒充 ROS2 实测，阶段一不得破坏 NDT-OMP 默认链路。 |

## 项目目录结构

| 目录 | 作用 |
| --- | --- |
| `src/app/` | 在线和离线定位入口，例如 `run_loc_online.cc`、`run_loc_offline.cc`。 |
| `src/core/system/` | `LocSystem` ROS2 node 持有者，负责订阅、TF、topic publisher 和调用 `Localization`。 |
| `src/core/localization/` | 定位核心，包括 `Localization`、`LocalizationResult`、`LidarLoc` 和 pose graph。 |
| `src/core/lio/` | AA-FasterLIO 风格前端和 LiDAR odom 输出。 |
| `src/core/maps/` | `TiledMap`、chunk 地图读取、动态层更新和 NDT target 设置。 |
| `src/core/common/` | 点云、IMU、SE3、YAML、timer 等通用工具。 |
| `src/ui/` | Pangolin 可视化窗口，依赖 `system.with_ui`。 |
| `thirdparty/` | 阶段一保留的依赖代码，例如 `pclomp`、`livox_ros_driver2` 等。 |
| `config/` | 多数据集/传感器默认配置。 |
| `docs/` | 阶段一、topic 增强和本轮阶段二规划文档。 |

## 定位相关文件清单

| 模块 | 关键文件 | 静态结论 |
| --- | --- | --- |
| 在线入口 | `src/app/run_loc_online.cc` | 创建 `LocSystem`，调用 `Init()`、`SetInitPose(SE3())`、`Spin()`，依赖 ROS2 runtime。 |
| 离线入口 | `src/app/run_loc_offline.cc` | 直接创建 `loc::Localization`，读取 bag，处理 IMU/PointCloud2/Livox，结束时 `Finish()`；不经过 `LocSystem`，因此不发布新增 ROS2 topic。 |
| ROS2 系统层 | `src/core/system/loc_system.h/.cc` | 订阅 IMU、标准 LiDAR、Livox；保留 TF；新增 pose/status/diagnostics/odometry publisher。 |
| 核心调度 | `src/core/localization/localization.h/.cpp` | 管理 LIO、LidarLoc、PGO、异步队列、UI、TF callback 和 result callback。 |
| 结果结构 | `src/core/localization/localization_result.h` | 结果字段包括 pose、valid、status、confidence、lidar_loc_valid、平滑标志、parking 等。 |
| scan-to-map | `src/core/localization/lidar_loc/lidar_loc.h/.cc` | 使用 `pclomp::NormalDistributionsTransform`，读取地图 chunk，执行 NDT-OMP 匹配。 |
| PGO/平滑 | `src/core/localization/pose_graph/pgo.h/.cc`、`smoother.h` | 融合 DR/LIO 与 lidar loc，输出高频定位结果。 |
| 点云预处理 | `src/core/common/point_cloud_utils.h`、`src/core/lio/*` | 完成 ROS 点云转换、Livox 点转换、LIO 前端预处理。 |
| 地图 | `src/core/maps/tiled_map.h/.cc` | 读取 `index.txt` 和 chunk bin/pcd，维护静态层和动态层。 |
| 消息/服务 | `msg/`、`srv/LocCmd.srv`、`srv/SaveMap.srv` | 消息/服务定义已迁移，但阶段一在线运行未静态确认已注册定位命令 service。 |
| 构建 | `CMakeLists.txt`、`src/CMakeLists.txt`、`cmake/packages.cmake` | ROS2、PCL、yaml-cpp、Pangolin、tf2、diagnostic/nav/geometry/std messages 均有静态声明。 |

## 在线运行链路

在线入口 `src/app/run_loc_online.cc` 调用 `rclcpp::init()` 后构造 `LocSystem`。`LocSystem::Init()` 读取 YAML，创建 ROS2 node、订阅器、TF broadcaster 和 topic publisher，然后初始化 `Localization`。入口随后设置单位初始位姿并进入 `Spin()`。

在线数据链路如下：

1. `LocSystem` 订阅 `common.imu_topic`、`common.lidar_topic`、`common.livox_lidar_topic`。
2. IMU 回调进入 `Localization::ProcessIMUMsg()`。
3. 标准点云回调进入 `Localization::ProcessLidarMsg()`。
4. Livox 回调进入 `Localization::ProcessLivoxLidarMsg()`。
5. `Localization` 根据配置使用同步或异步处理，将点云送入 LIO 和 LidarLoc。
6. LIO 产生 DR/Lidar odom 预测，LidarLoc 执行 scan-to-map。
7. PGO/平滑器融合并通过高频回调输出 `LocalizationResult`。
8. `Localization` 在 valid 结果上触发 TF callback，同时触发 result callback。
9. `LocSystem` 保留 `map -> base_link` TF，同时发布 `/localization/pose`、`/localization/status`、`/localization/diagnostics`，可选发布 `/localization/odometry`。

## 离线运行链路

离线入口 `src/app/run_loc_offline.cc` 直接创建 `loc::Localization`，使用 `rosbag2_cpp::Reader` 遍历 bag 消息。它会处理 IMU、标准 PointCloud2 和 Livox CustomMsg，并调用同一套 `Localization` 主链路，最后调用 `Finish()`。

静态审计结论：

| 项 | 结论 |
| --- | --- |
| 是否使用 `LocSystem` | 否。 |
| 是否创建 ROS2 publisher | 未静态发现。 |
| 是否发布新增 topic | 不发布新增 topic，因为 topic publisher 位于 `LocSystem`。 |
| 是否可能输出 TF | 未静态发现离线入口创建 TF broadcaster。 |
| 运行验证状态 | 待人工 ROS2 环境验证。 |

## IMU 输入链路

IMU 输入通过 `LocSystem::IMUCallBack()` 转换为内部 `IMUPtr` 后进入 `Localization::ProcessIMUMsg()`。`Localization` 将 IMU 数据交给 LIO 前端，供运动预测和高频外推使用。当前静态审计可确认数据入口和函数调用存在，但无法在 Windows 环境验证 IMU 时间同步、坐标外参、丢包和异常序列处理。

## LiDAR 输入链路

标准 LiDAR 使用 `sensor_msgs/msg/PointCloud2`，Livox 使用迁移后的 `livox_ros_driver2::msg::CustomMsg`。两类输入分别进入 `Localization::ProcessLidarMsg()` 和 `Localization::ProcessLivoxLidarMsg()`，随后转换为内部点云并分发给 LIO 与 LidarLoc。

配置字段包括：

| 字段 | 作用 |
| --- | --- |
| `common.lidar_topic` | 标准 `PointCloud2` topic。 |
| `common.livox_lidar_topic` | Livox 自定义点云 topic。 |
| `common.imu_topic` | IMU topic。 |
| `system.map_path` | 定位地图根目录。 |

## 点云预处理链路

点云预处理分布在公共点云工具和 LIO/LidarLoc 内部。标准点云和 Livox 点云会转换为内部点类型；LidarLoc 中会对输入执行 voxel filter 和近远距离裁剪，之后进入 NDT 匹配。当前实现更接近原 lightning-lm 主链路迁移，尚未形成工业级统一点云预处理策略配置、输入点字段校验、强度/时间字段诊断和传感器 profile 管理。

## LIO / lidar odom 链路

`src/core/lio/` 保留 AA-FasterLIO 风格前端。`Localization` 使用 LIO 输出作为 DR 和 lidar odom 预测输入，PGO 用其和 LidarLoc 结果共同构造高频输出。LIO map debug dump 已由 `debug_dump.enable_lio_map_dump` 控制，默认关闭。

## LidarLoc scan-to-map 链路

`LidarLoc` 是定位主匹配模块。其初始化会读取地图 index、构造 `TiledMap`、读取恢复位姿、初始化 NDT-OMP matcher，并启动地图更新线程。定位时通过 LIO/DR 预测作为初值，调用 `Localize()` 执行 scan-to-map。

关键静态结论：

| 项 | 结论 |
| --- | --- |
| 默认 matcher | `pclomp::NormalDistributionsTransform<PointType, PointType>`。 |
| 粗匹配 matcher | 另一个 NDT-OMP 实例，分辨率更大、迭代更少。 |
| confidence | 来自 NDT transformation probability，不是统一工业级 score。 |
| 多候选路径 | 代码中保留 LO/DR/self 等尝试痕迹，但多候选策略和 failure reason 不完整。 |
| debug 写盘 | `Localize()` 中的 NDT target dump 已由 `debug_dump.enable_lidar_loc_target_dump` 控制，默认关闭。 |

## NDT-OMP 匹配实现

`LidarLoc` 构造函数中配置 NDT-OMP：

| 参数 | 静态值或来源 |
| --- | --- |
| resolution | 默认 1.0，可由 YAML 覆盖。 |
| neighborhood search | `pclomp::DIRECT7`。 |
| outlier ratio | 默认 0.45。 |
| step size | 默认 0.1。 |
| transformation epsilon | 默认 0.01。 |
| max iterations | 默认 20。 |
| num threads | 默认 4。 |
| 粗匹配 resolution | 默认 5.0。 |

当前阶段一补强没有替换 NDT-OMP，也没有引入 `RegistrationBackend`、small_gicp、fast_gicp、VGICP 或 gtsam_points，这符合“阶段一后补强不是阶段二”的边界。

## TiledMap 地图加载和 chunk 切换

`TiledMap::LoadMapIndex()` 从地图目录读取 `index.txt`，加载原点、静态 chunk 列表和功能点。运行时根据当前 pose 调用 `LoadOnPose()`、`GetStaticCloud()`、`GetDynamicCloud()` 和 `SetNewTargetForNDT()`，将当前局部地图设置为 NDT target。

工业化风险：

| 风险 | 说明 |
| --- | --- |
| 缺少 metadata | 未发现地图 version、schema、frame、hash、chunk 数量摘要等强校验。 |
| chunk 缺失处理不足 | 缺失或损坏 chunk 的降级策略需要人工验证和补强。 |
| 坐标一致性 | index 原点、map frame、base frame 和数据集配置之间缺少集中审计工具。 |
| 并发安全 | 地图更新线程和定位线程共享地图对象，需进一步审计锁粒度和生命周期。 |

## 动态地图层

动态地图层由 `TiledMap` 和 `LidarLoc` 管理，配置字段包括 `update_dynamic_cloud`、dynamic layer 策略、保存路径等。`Finish()` 中会根据持久化/保存条件写回动态地图。当前实现保留了原 lightning-lm 的动态层路线，但产品化上缺少地图版本治理、写回事务、失败恢复、空间配额和数据安全策略。

## PGO 融合和平滑输出

`PGO` 接收 DR、LIO 和 LidarLoc 结果，内部使用 `PoseSmoother` 执行平滑和高频外推。`Localization` 注册 PGO 高频输出 callback，在 callback 中：

1. 复制 `LocalizationResult`。
2. 对 valid 结果触发 TF callback。
3. 触发 result callback，供 `LocSystem` 发布 topic。
4. 如启用 UI，更新 Pangolin 显示。

当前 PGO 更偏工程平滑输出，不提供工业级协方差估计、质量分解、退化分类或完整 failure reason。

## TF 输出

原 TF 输出保留在 `LocSystem`：

| 项 | 结论 |
| --- | --- |
| 开关 | `system.pub_tf`。 |
| 发布对象 | `tf2_ros::TransformBroadcaster`。 |
| 发布调用 | `sendTransform()`。 |
| 数据来源 | `LocalizationResult::ToGeoMsg()`。 |
| frame | `map -> base_link`。 |
| 触发条件 | PGO 高频输出产生 valid `LocalizationResult` 时。 |

注意：`LocalizationResult::ToGeoMsg()` 中 frame 仍硬编码为 `map` 和 `base_link`；topic 输出的 `ros_output.map_frame/base_frame` 可配置不等于 TF frame 可配置。

## ROS2 topic 输出

阶段一补强后的 topic 输出位于 `LocSystem`：

| Topic | Message | 默认状态 | 数据来源 | 触发条件 |
| --- | --- | --- | --- | --- |
| `/localization/pose` | `geometry_msgs/msg/PoseStamped` | 开启 | `LocalizationResult.pose_` | 默认只发布 valid result。 |
| `/localization/status` | `std_msgs/msg/String` | 开启 | `LocalizationResult` 状态字段 | 每个结果按 `status_min_period_sec` 节流。 |
| `/localization/diagnostics` | `diagnostic_msgs/msg/DiagnosticArray` | 开启 | `LocalizationResult` 诊断字段 | 每个结果按 `diagnostics_min_period_sec` 节流。 |
| `/localization/odometry` | `nav_msgs/msg/Odometry` | 关闭 | `LocalizationResult.pose_` | 启用后默认只发布 valid result。 |

静态审计可确认 publisher 创建和 result callback 接入存在，但 topic 实际发布频率、QoS、订阅端兼容性、sim time 行为需要人工 ROS2 环境验证。

## service / command 接口

`srv/LocCmd.srv` 和 `srv/SaveMap.srv` 已保留并在 CMake 中生成。`msg/README.md` 明确记录 `LocCmd.srv` 是继承接口定义，但阶段一在线 runtime 未注册完整重定位 service。静态搜索未确认 `LocSystem` 中注册定位命令 service，因此产品级远程初始化、外部重定位、暂停/恢复等接口仍是阶段二工作。

## YAML 配置读取

配置读取分为两类：

| 层级 | 特点 |
| --- | --- |
| `LocSystem` 的 `ros_output` | 使用 fallback helper，缺失字段时会采用内置默认值并记录 warning。 |
| 核心定位配置 | 大量字段通过 `YAML::Node.as<T>()` 读取，缺失或类型错误可能抛异常。 |

所有默认 YAML 已包含 `ros_output` 段，且保留 `system.pub_tf`。阶段二应补充配置 schema、默认值汇总、启动前配置审计脚本和路径存在性检查。

## 生命周期和线程

静态发现的线程/生命周期点：

| 模块 | 行为 | 风险 |
| --- | --- | --- |
| `Localization` | 析构和 `Finish()` 释放 LidarLoc、LIO、PGO、异步处理器。 | `LocSystem::~LocSystem()` 直接调用 `loc_->Finish()`，若初始化失败或空指针路径未覆盖，存在脆弱性。 |
| `AsyncMessageProcess` | 后台线程处理消息队列，`Quit()` 设置退出并 join。 | 队列丢弃策略和 shutdown 时序需真实数据压力验证。 |
| `LidarLoc` | `update_map_thread_` 动态加载地图，`Finish()` join。 | 地图对象、dynamic layer 和 NDT target 的并发访问需进一步验证。 |
| `PangolinWindowImpl` | UI 线程/窗口更新全局地图。 | 无 GUI 环境或 headless 部署需要禁用并验证。 |

## debug 文件输出

静态发现多处写盘：

| 文件/函数 | 写盘行为 | 工业化风险 |
| --- | --- | --- |
| `LidarLoc::Localize()` | `debug_dump.enable_lidar_loc_target_dump=true` 时写 `debug_dump.lidar_loc_target_path` | 默认关闭；开启后仍可能带来定位回调 IO 开销，建议只在排查时使用。 |
| `LaserMapping::SaveMap()` | `debug_dump.enable_lio_map_dump=true` 时写 `debug_dump.lio_map_path` | 默认关闭；开启后会构建并写出 LIO global map，建议只在排查时使用。 |
| `TiledMap` | 保存静态/动态 chunk | 应保留，但需要事务、路径、配额和失败处理。 |
| recover pose | 写入 `recover_pose_path` | 有恢复价值，但需要明确频率、权限和损坏恢复策略。 |

## 当前已实现能力清单

| 能力 | 状态 |
| --- | --- |
| 在线 ROS2 定位入口 | 静态存在，待人工运行验证。 |
| 离线 rosbag 定位入口 | 静态存在，topic 输出边界已明确，待人工运行验证。 |
| IMU + LiDAR 输入 | 静态存在，需真实传感器/数据集验证。 |
| LIO/DR 预测 | 静态存在，需轨迹验证。 |
| NDT-OMP scan-to-map | 静态存在并保持默认路线。 |
| TiledMap 大地图 chunk 加载 | 静态存在。 |
| 动态地图层 | 静态存在，但治理不足。 |
| PGO/高频平滑输出 | 静态存在。 |
| `map -> base_link` TF | 静态保留。 |
| pose/status/diagnostics topic | 静态存在。 |
| odometry topic | 静态存在，默认关闭。 |
| service 定义 | 已生成，但 runtime 注册不完整。 |

## 当前无法静态确认的事项

| 事项 | 原因 |
| --- | --- |
| `colcon build` 是否通过 | 当前 Windows 环境无 ROS2/colcon。 |
| ROS2 topic 是否真实发布 | 当前无法运行节点。 |
| TF 时间戳是否与 bag/sim time 完全兼容 | 需要 ROS2 clock 和 bag 回放环境。 |
| NDT-OMP 在目标 Ubuntu/PCL 下是否编译稳定 | 依赖上游 `ndt_omp/pclomp` 与 PCL/编译器组合。 |
| 地图 chunk 缺失、损坏、跨版本时行为 | 需要构造地图数据验证。 |
| Livox/机械雷达实际点字段适配 | 需要真实 sensor profile 和数据。 |
| 高频输出耗时和队列积压 | 需要长时间 rosbag 或实车数据。 |

## 待人工 ROS2 环境验证事项

1. Ubuntu 22.04 + ROS2 Humble 下执行 `colcon build --packages-select lightning_localization`。
2. 使用同一 config/map/bag 对比阶段一 TF 输出和补强后的 topic 输出。
3. 使用 `ros2 run tf2_ros tf2_echo map base_link` 验证 TF。
4. 使用 `ros2 topic echo /localization/pose`、`/localization/status`、`/localization/diagnostics` 验证内容。
5. 若启用 odometry，使用 `ros2 topic echo /localization/odometry` 和 `ros2 topic hz /localization/odometry` 验证默认关闭/显式开启语义。
6. 使用 `ros2 topic hz` 检查 pose/status/diagnostics 发布频率和节流。
7. RViz2 同时显示 TF、PoseStamped、点云，确认 frame 和时间戳一致。
8. Pangolin 在 `system.with_ui=true` 时确认窗口更新和 headless 禁用路径。
9. 用 NCLT/VBR/Livox/Robosense 等配置分别验证输入链路和地图路径。
10. 长时间回放验证内存、线程退出、debug 写盘、dynamic layer 保存和恢复位姿。
