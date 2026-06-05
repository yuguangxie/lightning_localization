# 阶段二技术验证计划

检索日期：2026-06-04  
验证边界：当前 Windows Codex 环境不能执行 ROS2 编译、ROS2 节点运行、rosbag 回放、RViz2 或 Pangolin 观察。本文定义阶段二必须完成的验证项目，运行类项目均为待人工 ROS2 环境验证。

## 验证分层

| 层级 | 目的 | 可在当前环境完成 | 必须在 ROS2 环境完成 |
| --- | --- | --- | --- |
| 静态验证 | 发现源码、配置、构建和文档一致性问题 | 是 | 否 |
| 纯 C++ 测试 | 验证 backend、地图、配置、score 等核心逻辑 | 部分可设计，需目标编译器运行 | 推荐在 Ubuntu 执行 |
| ROS2/colcon 编译 | 验证依赖、rosidl、ament、target linkage | 否 | 是 |
| rosbag 回放 | 验证真实时间序列和定位链路 | 否 | 是 |
| RViz2/Pangolin 观察 | 验证 frame、轨迹、地图和 UI | 否 | 是 |
| 轨迹评估 | 量化精度、丢失率和回归 | 否 | 是 |
| 长期运行 | 验证内存、线程、写盘和动态层 | 否 | 是 |

## 需要静态验证的内容

| 项 | 方法 | 验收 |
| --- | --- | --- |
| 默认 NDT-OMP 未被替换 | 搜索 `pclomp::NormalDistributionsTransform`、backend config、默认 YAML | 默认 backend 为 NDT-OMP。 |
| optional deps 不污染默认构建 | 检查 CMake option 和 package.xml | 未启用 small_gicp/fast_gicp/CUDA/GTSAM 时不要求安装。 |
| topic 名称一致 | 搜索 `/localization/`、`ros_output` | 代码、配置、文档一致。 |
| TF 保留 | 搜索 `sendTransform`、`system.pub_tf` | `map -> base_link` 仍由原开关控制。 |
| debug dump 默认关闭 | 搜索 `savePCDFile`、新增 `debug_dump` | 高频写盘必须受配置控制。 |
| 地图 metadata 校验 | 检查 metadata parser 和错误处理 | 缺失/损坏 chunk 有明确错误或 warning。 |
| docs 无虚假验证 | 搜索运行验证被误写成完成的表述 | 未执行项均标记待人工验证。 |

## 需要纯 C++ 测试的内容

| 测试项 | 输入 | 预期输出 |
| --- | --- | --- |
| `RegistrationResult` 字段完整性 | 构造后端结果 | pose、score、has_converged、fitness/residual、inlier ratio、covariance/source、failure reason、elapsed time、backend name 均可读。 |
| NDT-OMP backend adapter | 小规模 target/source 点云 | 能调用 align 并返回统一结果，失败路径有 reason。 |
| backend factory | YAML backend 名称 | 默认返回 NDT-OMP，未知 backend 返回明确错误。 |
| 配置 schema | 缺字段、错类型、越界值 | 输出错误列表，不静默使用危险值。 |
| 地图 metadata | 正常、缺失、hash 错误、chunk 缺失 | 能区分兼容模式、错误和 warning。 |
| debug dump policy | 默认配置、开启配置、频率限制 | 默认不写，高频时受节流。 |
| status/diagnostics mapper | 不同状态组合 | level 和 KeyValue 与文档一致。 |

## 需要 ROS2/colcon 编译验证的内容

```bash
colcon build --packages-select lightning_localization --cmake-args -DCMAKE_BUILD_TYPE=Release
source install/setup.bash
ros2 pkg executables lightning_localization
ros2 interface show lightning_localization/srv/LocCmd
ros2 interface show lightning_localization/srv/SaveMap
```

可选依赖矩阵：

| 构建矩阵 | 目标 |
| --- | --- |
| 默认 NDT-OMP only | 必须通过。 |
| small_gicp CPU | 可选通过，不影响默认。 |
| fast_gicp CPU | 可选通过，不影响默认。 |
| VGICP CPU | 可选通过，不影响默认。 |
| CUDA backend | 单独 GPU 环境验证，失败不影响默认。 |
| gtsam_points | 单独 GTSAM/CUDA 版本矩阵验证，失败不影响默认。 |

## 需要 rosbag 回放验证的内容

| 场景 | 配置 | 检查项 |
| --- | --- | --- |
| NCLT | `config/default_nclt.yaml` | 离线/在线输入、map path、TF、轨迹、耗时。 |
| VBR | `config/default_vbr.yaml` | 默认 NDT-OMP 回归和 chunk 加载。 |
| Livox | `config/default_livox.yaml` | Livox CustomMsg 字段、时间戳、外参。 |
| Robosense | `config/default_robosense.yaml` | 机械雷达字段、scan 格式、运动畸变影响。 |
| 自采数据 | 项目实际配置 | 传感器 profile、地图版本、定位丢失率。 |

人工命令模板：

```bash
source install/setup.bash
ros2 bag play /path/to/bag --clock
ros2 run lightning_localization run_loc_online --ros-args --params-file /path/to/config.yaml
```

实际入口参数以当前 launch/README 为准，必须记录完整命令和日志。

## 需要 RViz2/Pangolin 观察的内容

| 工具 | 检查项 |
| --- | --- |
| RViz2 | Fixed Frame=`map`，TF tree，PoseStamped，点云，轨迹跳变。 |
| RViz2 | `/localization/pose` 与 `map -> base_link` 一致性。 |
| RViz2 | invalid result 不发布 pose-like topic 的默认行为。 |
| Pangolin | `system.with_ui=true` 时地图、scan、轨迹可视化。 |
| Headless | `system.with_ui=false` 时无 GUI 依赖阻塞。 |

## 需要轨迹评估的内容

| 指标 | 定义 | 数据来源 |
| --- | --- | --- |
| ATE RMSE | 轨迹对齐后的绝对轨迹误差均方根 | GT 或参考轨迹 vs `/localization/pose`。 |
| ATE P95 | ATE 95 分位 | 同上。 |
| RPE translation | 相邻时间间隔相对平移误差 | GT/参考轨迹。 |
| RPE rotation | 相邻时间间隔相对旋转误差 | GT/参考轨迹。 |
| 初始化成功率 | 初始定位在阈值时间内达到 GOOD 的比例 | status/diagnostics。 |
| 重定位成功率 | 触发重定位后恢复 GOOD 的比例 | service response + diagnostics。 |
| 定位丢失率 | FAIL 或无 valid pose 的时间占比 | status + pose topic。 |
| 平均匹配耗时 | backend elapsed time 平均值 | diagnostics 或日志。 |
| P95 匹配耗时 | backend elapsed time 95 分位 | diagnostics 或日志。 |
| TF 输出频率 | `map -> base_link` 发布频率 | `tf2_echo`/bag 解析。 |
| status/diagnostics 完整率 | 实际消息数/期望消息数 | topic hz/bag。 |
| failure reason 分布 | 各 failure reason 占比 | diagnostics。 |
| map chunk 缺失处理 | 缺失 chunk 时错误/降级是否符合预期 | 构造地图。 |
| debug dump 开关行为 | 默认无高频 PCD 写入，开启后可控 | 文件系统审计。 |
| 默认 NDT-OMP 回归一致性 | 阶段二默认轨迹与阶段一基线差异 | 同一 bag/config/map 对比。 |

## 需要后端对比的内容

| 后端 | 对比维度 | 结论要求 |
| --- | --- | --- |
| NDT-OMP | 默认基线 | 阶段二默认必须不回归。 |
| small_gicp | 精度、耗时、初始化鲁棒性 | 只作为可选增强，记录适用场景。 |
| fast_gicp | 精度、耗时、构建复杂度 | CPU-only 与 CUDA 分开评估。 |
| VGICP | 退化场景、地图密度、内存 | 不以单次好结果替代统计评估。 |
| gtsam_points | 重定位/因子图增强 | 先做离线增强，不替换默认 `Localize()`。 |

## 需要异常场景验证的内容

| 场景 | 期望行为 |
| --- | --- |
| 缺 IMU | status/diagnostics 报告输入缺失，不崩溃。 |
| 缺 LiDAR | status/diagnostics 报告输入缺失，不发布伪造 pose。 |
| 地图路径不存在 | 启动失败并给出明确错误。 |
| chunk 文件缺失 | metadata/chunk 校验报告具体 chunk。 |
| 初值偏差大 | 进入 INITIALIZING 或 FAIL，不误报 GOOD。 |
| 低纹理/平面退化 | diagnostics 报告 degeneracy/failure reason。 |
| 时间戳跳变 | 不输出错误 TF，记录 warning。 |
| service 并发重定位 | 命令串行化或拒绝并发，返回明确状态。 |
| debug dump 目录不可写 | 不阻塞定位主流程，报告 warning。 |

## 需要长期运行验证的内容

| 项 | 验证方式 | 验收 |
| --- | --- | --- |
| 8 小时 bag 循环 | 重复回放或长包 | 无崩溃，内存无持续增长。 |
| 高频 topic | `ros2 topic hz` 和 bag 解析 | 频率稳定，节流有效。 |
| 地图动态层 | 长时间更新和退出保存 | 文件完整，退出无死锁。 |
| debug dump | 默认关闭和显式开启两种模式 | 默认不增长，开启有频率/容量限制。 |
| 线程退出 | Ctrl-C、SIGTERM、异常配置 | `Finish()` 和 join 正常。 |

## 推荐测试数据

| 数据 | 用途 |
| --- | --- |
| NCLT | 公开数据集，验证离线定位、地图路径和轨迹评估。 |
| VBR | 原项目示例数据，验证与 upstream 行为一致性。 |
| UrbanLoco | 城市场景和动态目标风险验证。 |
| Livox 自采 | 非重复扫描雷达适配验证。 |
| Robosense/机械雷达自采 | 多线机械雷达字段和退化验证。 |
| 人工损坏地图 | 验证 metadata/chunk 校验和故障提示。 |

## 验收表格

| 验收项 | 结果记录 | 通过条件 |
| --- | --- | --- |
| 默认 colcon build | 待人工 ROS2 环境验证 | Release build 成功，无缺依赖。 |
| 默认 NDT-OMP 回归 | 待人工 ROS2 环境验证 | 同一 bag/config/map 下轨迹差异在阈值内。 |
| TF 输出 | 待人工 ROS2 环境验证 | `map -> base_link` 连续且 frame 正确。 |
| pose/status/diagnostics | 待人工 ROS2 环境验证 | topic 存在、字段完整、频率合理。 |
| odometry optional | 待人工 ROS2 环境验证 | 默认关闭，开启后 covariance source 清晰。 |
| ATE/RPE | 待人工 ROS2 环境验证 | 指标报告完整。 |
| 初始化/重定位 | 待人工 ROS2 环境验证 | 成功率、耗时、failure reason 可记录。 |
| debug dump | 待人工 ROS2 环境验证 | 默认关闭且开启可控。 |
| 地图校验 | 待人工 ROS2 环境验证 | 缺失/损坏地图可诊断。 |
| optional backend | 待人工 ROS2 环境验证 | 未启用不影响默认；启用后有独立结果。 |
