# 阶段二近工业级定位模块路线图

检索日期：2026-06-04  
边界说明：阶段二可以设计并逐步实现工业化能力，但默认 NDT-OMP 行为不能被破坏。small_gicp、fast_gicp、VGICP、gtsam_points 必须作为可选依赖；CUDA/GTSAM 不得成为默认必需项。当前文档是规划，不代表 ROS2/colcon/rosbag/RViz2/Pangolin 已验证。

## 阶段二总体目标

阶段二目标是在阶段一独立 ROS2 定位包基础上，构建“近工业级”的定位模块能力：

1. 保持默认 NDT-OMP scan-to-map 主链路和阶段一输出兼容。
2. 建立可插拔 registration backend 架构。
3. 增强质量评价、退化检测、failure reason 和 diagnostics。
4. 补齐初始化/重定位、service、lifecycle 和线程安全。
5. 建立地图 metadata/chunk 校验和 debug dump 治理。
6. 建立纯 C++、ROS2、rosbag、轨迹精度、性能和长期运行验证体系。

## 不得破坏的阶段一行为

| 行为 | 阶段二要求 |
| --- | --- |
| 默认 matcher | 仍为 NDT-OMP。 |
| 默认 TF | 保留 `map -> base_link`。 |
| 默认 topic | 保留 `/localization/pose`、`/localization/status`、`/localization/diagnostics`，odometry 默认关闭。 |
| 在线入口 | `run_loc_online` 可继续使用。 |
| 离线入口 | `run_loc_offline` 不因 topic 增强而破坏。 |
| 地图格式 | 兼容当前 `index.txt` 和 chunk 文件，同时增加 metadata 校验。 |

## 工作包划分

| 工作包 | 输入 | 输出 | 涉及文件 | 技术难点 | 验证方法 | 验收标准 |
| --- | --- | --- | --- | --- | --- | --- |
| 1. `RegistrationBackend` 抽象 | 当前 `LidarLoc::Localize()`、NDT-OMP 参数 | `RegistrationBackend` interface、backend factory、配置枚举 | `src/core/localization/lidar_loc/`、新增 `registration/` | 不改变默认 NDT 行为，避免大规模改写 `LidarLoc` | 纯 C++ 单测、默认 backend 静态检查、rosbag 回归 | 默认配置轨迹与阶段一一致，未安装 optional deps 可构建。 |
| 2. NDT-OMP 默认 backend 封装 | 当前 pclomp NDT 实例 | `NdtOmpBackend`、统一 `RegistrationResult` | `lidar_loc.cc`、`thirdparty/pclomp` 引用 | 保留参数、粗匹配、target 更新和 confidence 兼容 | 纯 C++ mock cloud、ROS2 bag 人工验证 | NDT-OMP 为默认 backend，topic confidence 兼容旧语义。 |
| 3. small_gicp backend 可选接入 | small_gicp 官方库、PCL wrapper | `SmallGicpBackend` optional CMake 开关 | `cmake/`、`registration/` | PCL 点云适配、协方差估计、OpenMP/TBB 选择 | optional build matrix、bag 对比 | 未启用时默认构建不依赖 small_gicp。 |
| 4. fast_gicp backend 可选接入 | fast_gicp CPU backend | `FastGicpBackend` optional | `cmake/`、`registration/` | 原库偏 ROS1/catkin 历史，依赖 Sophus/nvbio/CUDA 可选 | CPU-only build、bag 对比 | CPU backend 可独立启用，失败时不影响 NDT。 |
| 5. VGICP backend 可选接入 | fast_gicp/small_gicp VGICP | `VgicpBackend` optional | `registration/` | voxel covariance、内存、参数语义和地图 target 管理 | 同一 bag 后端对比 | 输出统一 score 和耗时，默认不开启。 |
| 6. gtsam_points 可选增强路线 | gtsam_points/GTSAM 官方库 | 重定位候选或因子图增强原型 | 新增可选模块，不替换 `Localize()` | GTSAM 版本、CUDA 架构、因子图边界 | 独立 demo、离线评估 | 作为 optional path 存在，默认 NDT-OMP 不依赖 GTSAM。 |
| 7. 统一匹配结果结构 | 当前 confidence/status 字段 | `RegistrationResult`：pose、score、has_converged、fitness/residual、inlier ratio、covariance/source、failure reason、elapsed time、backend name | `localization_result.h`、`lidar_loc` | 字段兼容和 downstream 映射 | 单测和 diagnostics 静态检查 | diagnostics 能输出完整字段，默认 confidence 不丢失。 |
| 8. 质量评分和退化检测 | NDT output、LIO/DR delta、地图几何 | 统一 health score、degeneracy flags | `lidar_loc`、`pgo`、diagnostics | 不同后端分数不可直接比较 | bag 标注场景、异常注入 | GOOD/WARN/ERROR 与实际状态一致。 |
| 9. 初始化/重定位接口增强 | `SetInitPose`、`LocCmd.srv`、recover pose | runtime service/action、状态机、超时和回滚 | `LocSystem`、`Localization`、srv | 并发命令与定位线程同步 | ROS2 service 人工验证 | 外部可触发初始化/重定位并收到结果。 |
| 10. ROS2 status/diagnostics/service 增强 | 当前 topic 输出 | 结构化状态、故障码、状态查询 service | `LocSystem`、docs | 字段稳定性和频率控制 | `ros2 topic echo/hz`、service call | 诊断字段完整率满足验收。 |
| 11. lifecycle 和线程安全 | 当前普通 node、线程/Finish | lifecycle wrapper、重复 Finish 安全、异常退出保护 | `LocSystem`、`Localization`、`AsyncMessageProcess`、`LidarLoc` | 不破坏现有入口 | 单测、长时间回放、异常终止 | 退出无崩溃，线程 join 可验证。 |
| 12. debug dump 开关和数据治理 | `./data/tgt.pcd`、`./data/lio.pcd`、recover pose | `debug_dump` 配置、目录、频率、大小限制 | `lidar_loc.cc`、`laser_mapping.cc`、config | 不影响定位主循环 | 静态检查、运行目录检查 | 默认不写高频 PCD，开启后受限写盘。 |
| 13. 地图 metadata/chunk 校验 | `index.txt`、chunk 文件 | metadata schema、hash、chunk 缺失报告 | `TiledMap`、scripts、docs | 兼容旧地图并给出可理解错误 | 人工构造缺失 chunk | 缺失/损坏地图启动前可诊断。 |
| 14. 纯 C++ 非 ROS 测试 | 核心模块 | backend、配置、地图、score 单测 | `tests/` | 从 ROS 类型中解耦 | CTest/GTest | 不依赖 ROS2 runtime 可跑核心测试。 |
| 15. ROS2 bag 人工验证 | NCLT/VBR/Livox/自采 bag | 验证记录和日志 | `docs/`、scripts | 数据路径和时间同步 | Ubuntu ROS2 人工流程 | 每个场景有 TF/topic/轨迹记录。 |
| 16. 轨迹 ATE/RPE 和性能评估 | bag 输出轨迹、GT 或参考轨迹 | ATE/RPE、耗时、丢失率报告 | scripts、docs | 坐标对齐和时间同步 | evo 或等价工具 | 指标表完整可复现。 |
| 17. 官方 issue 风险处理 | 上游 issue/README | 依赖版本矩阵和规避策略 | docs、cmake | 外部依赖变化 | CI matrix、人工构建 | optional deps 风险不影响默认构建。 |

## 推荐实施顺序

| 顺序 | 工作 | 理由 |
| --- | --- | --- |
| 1 | 建立 Ubuntu/ROS2 阶段一运行基线 | 没有运行基线无法判断阶段二是否回归。 |
| 2 | 治理 debug dump 和配置 schema | 小范围高收益，降低性能与启动风险。 |
| 3 | 设计 `RegistrationResult` 和质量字段 | 多后端、diagnostics、验证指标都依赖它。 |
| 4 | 封装 NDT-OMP backend | 在默认行为可回归的前提下创建扩展点。 |
| 5 | 增强 status/diagnostics/service | 提高可观测性和可运维性。 |
| 6 | 地图 metadata/chunk 校验 | 防止数据问题被误判为算法问题。 |
| 7 | lifecycle 和线程安全 | 长时间运行前必须完成。 |
| 8 | small_gicp/fast_gicp/VGICP optional 接入 | 有基线和指标后再做后端对比。 |
| 9 | gtsam_points optional 增强 | 作为重定位或因子图增强，不作为第一替换项。 |
| 10 | 自动化测试和 CI 矩阵 | 固化阶段二成果。 |

## 不建议一开始做的事项

| 不建议事项 | 原因 |
| --- | --- |
| 直接替换 `LidarLoc::Localize()` 默认算法 | 容易破坏阶段一回归，无法定位问题来源。 |
| 把 CUDA/GTSAM 设为默认依赖 | 会显著提高安装难度并影响无 GPU 环境。 |
| 先做大规模重构 | 当前需要先建立运行基线、指标和回退策略。 |
| 先追求 VGICP/GTSAM 性能 | 缺少质量评估和地图校验时，性能结果不可解释。 |
| 把 odometry covariance 写成真实估计 | 当前只是静态占位，误导下游融合。 |

## 必须先解决的技术问题

1. 默认 debug 写盘必须配置化并默认关闭。
2. `RegistrationResult` 结构必须先定稿，字段至少包含 pose、confidence/score、has_converged、fitness/residual、inlier ratio、covariance 或 covariance placeholder、failure reason、elapsed time、backend name。
3. 地图 metadata/chunk 校验必须能识别缺失、版本和 hash 风险。
4. `LocCmd.srv` 或等价 service 必须在 runtime 层完成注册和状态反馈。
5. 阶段一默认 NDT-OMP 基线必须在真实 ROS2 环境记录。

## 可选增强项

| 增强项 | 价值 | 默认状态建议 |
| --- | --- | --- |
| small_gicp CPU backend | 低依赖、高性能、适合先试 | 可选关闭。 |
| fast_gicp CPU backend | PCL 接口兼容，便于对比 | 可选关闭。 |
| VGICP backend | 在部分场景精度/稳定性更好 | 可选关闭。 |
| CUDA backend | 高性能，但环境复杂 | 可选关闭，单独 CI/文档。 |
| gtsam_points 重定位/因子图增强 | 可做多帧约束和高级重定位 | 可选实验路径。 |
| lifecycle node | 产品部署友好 | 可作为 wrapper 增量引入。 |

## 人工 ROS2 验证计划

必须在 Ubuntu 22.04 + ROS2 Humble 环境执行：

```bash
colcon build --packages-select lightning_localization
source install/setup.bash
ros2 run lightning_localization run_loc_online --ros-args
ros2 run tf2_ros tf2_echo map base_link
ros2 topic echo /localization/pose
ros2 topic echo /localization/status
ros2 topic echo /localization/diagnostics
ros2 topic hz /localization/pose
ros2 topic hz /localization/status
ros2 topic hz /localization/diagnostics
ros2 param list
```

若显式设置 `ros_output.publish_odometry=true`：

```bash
ros2 topic echo /localization/odometry
ros2 topic hz /localization/odometry
```

这些命令当前未在 Windows Codex 环境执行。

## 风险与回退策略

| 风险 | 回退策略 |
| --- | --- |
| 新 backend 编译失败 | optional CMake 开关默认关闭；默认 NDT-OMP 不依赖它。 |
| 新 score 影响状态判断 | 保留旧 confidence 字段，新增字段灰度输出。 |
| lifecycle wrapper 影响原入口 | 保留 `run_loc_online` 普通入口。 |
| 地图 metadata 影响旧地图 | 旧地图进入兼容模式，但 diagnostics 明确 warning。 |
| service 并发触发导致状态错乱 | 命令队列串行化，初始化/重定位期间状态机锁定。 |

## 阶段二完成定义

| 维度 | 完成定义 |
| --- | --- |
| 默认行为 | NDT-OMP 默认链路、TF 和 topic 输出与阶段一兼容。 |
| 构建 | 默认配置在 Ubuntu 22.04 + ROS2 Humble 通过 colcon；optional deps 独立矩阵验证。 |
| 接口 | status、diagnostics、service 和 topic 字段稳定并文档化。 |
| 算法 | `RegistrationResult` 统一结构落地，至少 NDT-OMP backend 完成封装。 |
| 地图 | metadata/chunk 校验可识别主要数据错误。 |
| 质量 | ATE/RPE、耗时、丢失率、初始化/重定位成功率形成报告。 |
| 运维 | debug dump 默认关闭，线程退出和生命周期有验证记录。 |
| 文档 | 使用、配置、验证、风险和回退文档完整。 |
