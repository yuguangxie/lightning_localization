# 风险登记

| ID | 风险 | 影响 | 缓解措施 | 验证状态 |
|---|---|---|---|---|
| R1 | 当前工作区没有 ROS2 或 colcon | 无法在本地证明构建或运行 | 提供明确人工验证步骤 | 待人工 ROS2 环境验证 |
| R2 | 工作区没有完整 bag/map 数据 | 无法在本地回放定位 | 要求使用与上游对比相同的 config、map 和 bag | 待人工 ROS2 环境验证 |
| R3 | Pangolin/OpenGL 可用性随目标机器变化 | UI 构建或运行可能失败 | 明确依赖并文档化环境准备 | 待人工 ROS2 环境验证 |
| R4 | `LocSystem::~LocSystem()` 继承了 `loc_` 非空假设 | 初始化失败路径可能较脆弱 | 阶段一保持行为，记录为工业化增强风险 | 已记录静态风险 |
| R5 | 在线节点同时订阅标准点云和 Livox 自定义 topic | 若两个 topic 携带同一传感器流，可能重复输入点云 | 部署时必须明确配置活动数据源 | 待人工 ROS2 环境验证 |
| R6 | `LocCmd.srv` 已保留但未在 `LocSystem` 中注册 | 外部重定位 service 未激活 | 文档化为阶段一继承边界 | 已记录静态风险 |
| R7 | 启用调试 PCD dump 时可能产生在线磁盘 IO | 若操作者开启高频 dump，可能带来实时延迟和文件覆盖风险 | 已通过 `debug_dump` 默认关闭；仅建议定向排查时开启 | 默认关闭 |
| R8 | PGO YAML 字段可能未完全驱动当前 PGO 实现 | 配置预期可能与运行时偏离 | 保留原行为并记录参数注意事项 | 已记录静态风险 |
| R9 | 复制后的 service 包名从上游 `lightning` 变为 `lightning_localization` | 下游包含生成 service 头文件的代码可能需要更新包名 | 定位运行时未包含这些头文件，文档化接口边界 | 待人工集成验证 |
| R10 | 文件系统目录和 ROS package 名均为 `lightning_localization` | 早期阶段一记录使用了过期目录名 | 保持文档与实际包目录一致 | 已在 topic 输出增强中更新 |
| R11 | Master prompt 引用了 `plans/stage-one-extraction-plan.md`，但初始工作区缺少该文件 | 执行包证据可能不完整 | 基于 master prompt 和当前 package 证据补建计划文件 | 已记录静态风险 |
