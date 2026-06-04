# 阶段一最终报告

## 结果

阶段一使用 `lightning_localization_v1/` 作为原始 lightning-lm 定位运行时的独立 ROS2 package 目录。其 ROS package 名为 `lightning_localization`。本次剥离保持默认 NDT-OMP 行为，未引入阶段二工业级匹配后端重构。

## 已创建包

关键包文件：

- `package.xml`
- `CMakeLists.txt`
- `cmake/packages.cmake`
- `src/CMakeLists.txt`
- `src/app/CMakeLists.txt`
- `launch/loc_online.launch.py`
- `launch/loc_offline.launch.py`

关键运行入口：

- `src/app/run_loc_online.cc`
- `src/app/run_loc_offline.cc`

关键文档：

- `README.md`
- `docs/architecture.md`
- `docs/io-spec.md`
- `docs/config-parameters.md`
- `docs/input-output-spec.md`
- `docs/configuration-parameters.md`
- `docs/localization-dependency-graph.md`
- `docs/file-migration-table.md`
- `docs/manual-ros2-validation.md`
- `docs/stage-one-acceptance-checklist.md`
- `docs/stage-one-static-audit-report.md`
- `docs/risk-register.md`

## 保留行为

该包保留：

- `LocSystem`
- `loc::Localization`
- `LidarLoc`
- 点云预处理
- LIO 预测
- PGO 和高频外推
- `TiledMap`
- NDT-OMP
- 动态地图层代码路径
- 在线和离线定位入口
- TF 输出
- Pangolin UI 代码路径
- YAML 配置读取
- Livox 自定义消息支持

## 从构建目标排除

新构建目标排除了 SLAM 应用、仅前端应用、回环应用、`SlamSystem` 和保存地图运行时接线。上游项目保持不变。

## 静态审计

已完成以下静态审计：

- 文件存在性
- 迁移范围
- CMake/package.xml 结构
- topic/frame/map-path 保留
- 默认 NDT-OMP 保留
- ROS2 人工验证流程
- 风险登记
- 本地 PowerShell 静态检查
- 本地 quoted include 闭包检查

## 需要人工验证

当前 Windows 工作区没有 ROS2 和 colcon，也没有完整 ROS2 bag/map 数据。因此以下项目待人工 ROS2 环境验证：

- ROS2 构建
- 在线节点运行
- 离线 rosbag 回放
- RViz TF 检查
- Pangolin 运行时检查
- 与上游 lightning-lm 的轨迹和耗时对比

## 完成标准

阶段一的源码迁移、构建文件重组、文档、静态审计和人工验证流程已完成。运行时验收必须在真实 ROS2 环境中按照 `docs/manual-ros2-validation.md` 确认。

