# 阶段一 Topic 输出增强报告

## 目标

增强阶段一 `lightning_localization` 独立 ROS2 定位包的 ROS2 topic 输出能力，完成现有输出审计、定位位姿 topic、定位状态 topic、diagnostics topic、可选 odometry topic、YAML 配置、构建依赖、文档、静态审计和人工 ROS2 验证流程。

## 修改文件

- `src/core/localization/localization.h`
- `src/core/localization/localization.cpp`
- `src/core/system/loc_system.h`
- `src/core/system/loc_system.cc`
- `config/default*.yaml`
- `package.xml`
- `cmake/packages.cmake`
- `src/CMakeLists.txt`
- `scripts/static_check.ps1`
- `scripts/static_check.py`
- `README.md`
- `docs/config-parameters.md`
- `docs/manual-ros2-validation.md`
- `docs/ros2-topic-publication-audit.md`
- `docs/ros2-topic-publication-audit_ZH.md`
- `docs/ros2-output-topics.md`
- `docs/ros2-output-topics_ZH.md`
- `docs/stage-one-topic-output-enhancement-report.md`
- `docs/stage-one-topic-output-enhancement-report_ZH.md`

同时修正了阶段一文档中已经过期的 `lightning_localization_v1/` 目录名。

## 新增 Topic

| Topic | 类型 | 默认状态 | 说明 |
|---|---|---|---|
| `/localization/pose` | `geometry_msgs/msg/PoseStamped` | 开启 | 默认发布有效定位位姿。 |
| `/localization/status` | `std_msgs/msg/String` | 开启 | 节流后发布每个定位结果，包含状态名、valid、confidence、timestamp 和 health 文本。 |
| `/localization/diagnostics` | `diagnostic_msgs/msg/DiagnosticArray` | 开启 | 节流后发布每个定位结果，包含 `lightning_localization/localization` 诊断状态。 |
| `/localization/odometry` | `nav_msgs/msg/Odometry` | 关闭 | pose 来自定位结果；twist 不估计；covariance 为静态配置占位值。 |

原 `map -> base_link` TF 仍由 `system.pub_tf` 控制，未被替换。

## 静态审计结果

| 检查项 | 结果 |
|---|---|
| `create_publisher` 存在并使用配置 topic 名称 | 通过 |
| `PoseStamped` 使用默认 `map` frame，可配置 | 通过 |
| 可选 `Odometry` 使用默认 `map` 与 `base_link`，可配置 | 通过 |
| `DiagnosticArray` 包含 status、valid、confidence、`lidar_loc_valid` 等 key | 通过 |
| `std_msgs/String` status 包含状态枚举名 | 通过 |
| 原 `system.pub_tf` 与 `sendTransform` 路径保留 | 通过 |
| `package.xml` 与 CMake 补齐 `builtin_interfaces` 和 `diagnostic_msgs`，保留 `nav_msgs` | 通过 |
| 所有默认 YAML 包含 `ros_output` | 通过 |
| 未新增 ROS2/colcon/rosbag/RViz/Pangolin 通过声明 | 通过 |

已执行的本地检查：

- `powershell -NoProfile -ExecutionPolicy Bypass -File .\lightning_localization\scripts\static_check.ps1`：通过；继承的上游/第三方源码标记仅作为 warning。
- 所有 `config/*.yaml` 的 `ros_output` key 文本审计：通过。
- publisher、TF 路径、消息类型和依赖文本审计：通过。
- Python 静态脚本未执行，因为当前 Windows 环境只有不可用的 WindowsApps `python.exe` 占位启动器，且没有 `py` launcher。

## 当前环境未执行

当前 Codex 工作区没有 ROS2、colcon、完整 bag/map 数据，因此未执行：

- `colcon build`
- ROS2 节点启动
- rosbag 回放
- `ros2 topic echo`
- `ros2 topic hz`
- `tf2_echo`
- RViz2 检查
- Pangolin 运行时检查

## 待人工 ROS2 验证

在 Ubuntu 22.04 + ROS2 Humble + colcon 环境中执行：

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-select lightning_localization
source install/setup.bash
ros2 run lightning_localization run_loc_online --config /path/to/default.yaml
ros2 run tf2_ros tf2_echo map base_link
ros2 topic echo /localization/pose
ros2 topic echo /localization/status
ros2 topic echo /localization/diagnostics
ros2 topic hz /localization/pose
ros2 topic hz /localization/status
ros2 topic hz /localization/diagnostics
```

如启用 odometry：

```bash
ros2 topic echo /localization/odometry
```

必须使用与阶段一 TF 验证相同的 config、map、bag 或真实传感器输入，对比 TF、pose、confidence、status、diagnostics level 和耗时。

## 风险和注意事项

- odometry covariance 是静态配置占位值，不是真实估计协方差。
- odometry twist 不由本轮估计，保持默认 0。
- status 采用 `std_msgs/String`，长期产品化时应再设计结构化接口。
- 离线 `run_loc_offline` 仍不发布 ROS2 topic，因为它绕过 `LocSystem`。
- 本轮 topic 发布路径仅完成源码静态审计，ROS2 实测待人工环境验证。

