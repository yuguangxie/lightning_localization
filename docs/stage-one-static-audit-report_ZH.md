# 阶段一静态审计报告

## 环境

审计在 Windows 工作区中完成，当前环境没有 ROS2、没有 colcon，也没有完整 ROS2 bag/map 数据。本地未执行 ROS2 构建和运行时验证。

## 已完成静态检查

| 项 | 结果 | 证据 |
|---|---|---|
| 独立包目录 | 通过 | 文件系统目录 `lightning_localization/` 存在；ROS package 名为 `lightning_localization` |
| 在线入口 | 通过 | `src/app/run_loc_online.cc` |
| 离线入口 | 通过 | `src/app/run_loc_offline.cc` |
| ROS2 package manifest | 通过 | `package.xml` |
| 根 CMake | 通过 | `CMakeLists.txt` |
| 源码构建目标 | 通过 | `src/CMakeLists.txt` |
| 应用构建目标 | 通过 | `src/app/CMakeLists.txt` |
| 配置迁移 | 通过 | `config/default*.yaml` |
| Launch 包装 | 通过 | `launch/loc_online.launch.py`, `launch/loc_offline.launch.py` |
| Livox 消息 | 通过 | `thirdparty/livox_ros_driver/msg/*.msg` |
| 上游项目保留 | 通过 | 原始 `lightning-lm/` 文件未被编辑或删除 |
| SLAM 运行时排除 | 通过 | 新构建目标中无 `run_slam_*`、`SlamSystem` 或 save-map 运行时 |
| 上游私有依赖移除 | 通过 | 新 manifest 中无 `scrubber_common` 或 `agibot_robot` |
| 本地 PowerShell 静态检查 | 通过 | `scripts/static_check.ps1` 完成，仅有继承源码警告 |
| 本地 quoted include 闭包 | 通过 | 本地 quoted include 在包内解析，或解析到已声明外部/ROS 依赖 |

## 依赖闭包

静态依赖闭包包含在线/离线入口、`LocSystem`、`Localization`、`LidarLoc`、NDT-OMP、`TiledMap`、LIO、PGO、`miao`、common 类型、YAML/file IO、wrappers、UI、Sophus 和 Livox 消息。详见 `docs/localization-dependency-graph.md`。

被引用的 prompt 文件 `plans/stage-one-extraction-plan.md` 在初始工作区快照中缺失，已根据 master prompt、agents、skills、validation procedure 和当前包状态创建。

## CMake 与 package 审计

新的 CMake 结构声明：

- C++17
- 复制 service 的 ROS2 interface generation
- 内置 Livox 消息子包
- 定位共享库
- `run_loc_online`
- `run_loc_offline`
- config、launch、docs 和 scripts install 规则

manifest 声明了 ROS2、TF、rosbag2、PCL conversion 和 visualization 依赖，且没有上游私有依赖。

## Config、Topic、Frame 与 Map 审计

复制的 YAML 文件保留上游 key。在线模式仍读取 `system.map_path`、`common.imu_topic`、`common.lidar_topic`、`common.livox_lidar_topic` 和 `system.pub_tf`。TF 输出仍通过 `LocalizationResult::ToGeoMsg()` 保持为 `map -> base_link`。

## 继承源码注释

剥离后的代码保留了一些上游源码和第三方头文件中的待办式注释。这些是继承源码注释，不是阶段一交付占位，也没有替代任何必需迁移、构建文件、文档或验证产物。

本地静态检查将这些报告为警告而非失败，因为包迁移、构建文件、文档和验证流程不依赖占位交付。

## 本地工具限制

Python 静态检查脚本已提供给 Linux 或具备 Python 的环境使用，但当前 Windows shell 中 `python --version` 没有返回可用输出。PowerShell 静态检查已成功执行，是本次本地脚本证据。

## 本地未执行项

以下项目仍待人工 ROS2 环境验证：

- `colcon build --packages-select lightning_localization`
- `ros2 run lightning_localization run_loc_online`
- `ros2 run lightning_localization run_loc_offline`
- rosbag 回放
- RViz TF 检查
- Pangolin 运行时检查
- 轨迹指标对比
