# 文件迁移表

| 原路径 | 目标路径 | 阶段一决策 | 原因 |
|---|---|---|---|
| `lightning-lm/src/app/run_loc_online.cc` | `src/app/run_loc_online.cc` | 已迁移 | 在线定位入口 |
| `lightning-lm/src/app/run_loc_offline.cc` | `src/app/run_loc_offline.cc` | 已迁移 | 离线 rosbag 定位入口 |
| `lightning-lm/src/core/system/loc_system.*` | `src/core/system/loc_system.*` | 已迁移 | ROS2 订阅与 TF 封装 |
| `lightning-lm/src/core/localization/**` | `src/core/localization/**` | 已迁移 | 定位总控、匹配器、结果、PGO |
| `lightning-lm/src/core/lio/**` | `src/core/lio/**` | 已迁移 | LIO 预测和点云预处理依赖 |
| `lightning-lm/src/core/maps/**` | `src/core/maps/**` | 已迁移 | 分块地图和动态层 |
| `lightning-lm/src/core/ivox3d/**` | `src/core/ivox3d/**` | 已迁移 | LIO 体素地图依赖 |
| `lightning-lm/src/core/miao/core/**` | `src/core/miao/core/**` | 已迁移 | PGO 优化器依赖 |
| `lightning-lm/src/core/miao/utils/**` | `src/core/miao/utils/**` | 已迁移 | 优化器工具依赖 |
| `lightning-lm/src/common/**` | `src/common/**` | 已迁移 | 通用位姿、点、IMU、选项类型 |
| `lightning-lm/src/io/**` | `src/io/**` | 已迁移 | YAML 与文件 IO |
| `lightning-lm/src/utils/**` | `src/utils/**` | 已迁移 | timer、sync、点云辅助工具 |
| `lightning-lm/src/wrapper/**` | `src/wrapper/**` | 已迁移 | rosbag 与 ROS 点云转换 |
| `lightning-lm/src/ui/**` | `src/ui/**` | 已迁移 | 定位使用的 Pangolin 可视化路径 |
| `lightning-lm/config/default*.yaml` | `config/default*.yaml` | 已迁移 | 保持配置行为 |
| `lightning-lm/srv/*.srv` | `srv/*.srv` | 已迁移 | 保留上游 service 接口定义 |
| `lightning-lm/thirdparty/Sophus/**` | `thirdparty/Sophus/**` | 已迁移 | SE3/SO3 依赖 |
| `lightning-lm/thirdparty/livox_ros_driver/**` | `thirdparty/livox_ros_driver/**` | 已迁移 | Livox 自定义消息生成 |
| `lightning-lm/src/core/system/slam.*` | 无 | 已隔离 | SLAM 运行时，不属于定位运行时 |
| `lightning-lm/src/app/run_slam_*` | 无 | 已隔离 | SLAM 应用入口 |
| `lightning-lm/src/app/run_frontend_offline.cc` | 无 | 已隔离 | 仅前端应用 |
| `lightning-lm/src/app/run_loop_offline.cc` | 无 | 已隔离 | 回环应用 |
| `lightning-lm/src/core/loop_closing/**` | 无 | 已隔离 | 定位编译目标不需要 |
| `lightning-lm/src/core/g2p5/**` | 无 | 已隔离 | 栅格地图生成/展示路径，定位目标不需要 |
| 原始 `lightning-lm/` 文件 | 保持不变 | 禁止删除 | 保护上游项目和用户工作 |

