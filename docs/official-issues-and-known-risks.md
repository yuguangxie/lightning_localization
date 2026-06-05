# 官方 issue、上游资料与已知风险

检索日期：2026-06-04  
联网状态：成功。检索来源包括 lightning-lm 官方 GitHub 仓库、NDT-OMP/fast_gicp/small_gicp/gtsam_points 官方仓库与 issue 页面、ROS2 Humble 官方消息文档。本文只记录公开页面可检索的信息，不把未复现的 issue 当作当前项目已发生故障。

## 搜索范围

| 范围 | 链接 |
| --- | --- |
| lightning-lm 官方仓库 | [gaoxiang12/lightning-lm](https://github.com/gaoxiang12/lightning-lm) |
| lightning-lm issues | [gaoxiang12/lightning-lm/issues](https://github.com/gaoxiang12/lightning-lm/issues) |
| NDT-OMP 上游 | [koide3/ndt_omp](https://github.com/koide3/ndt_omp) |
| fast_gicp 上游 | [koide3/fast_gicp](https://github.com/koide3/fast_gicp) |
| small_gicp 上游 | [koide3/small_gicp](https://github.com/koide3/small_gicp) |
| gtsam_points 上游 | [koide3/gtsam_points](https://github.com/koide3/gtsam_points) |
| ROS2 diagnostic_msgs | [diagnostic_msgs Humble docs](https://docs.ros.org/en/ros2_packages/humble/api/diagnostic_msgs/) |
| ROS2 geometry_msgs | [geometry_msgs Humble docs](https://docs.ros.org/en/humble/p/geometry_msgs/README.html) |
| ROS2 std_msgs | [std_msgs Humble docs](https://docs.ros.org/en/humble/p/std_msgs/index.html) |

## 搜索关键词

`lightning-lm localization`、`lightning-lm loc`、`lightning-lm ROS2 Humble`、`lightning-lm build`、`lightning-lm livox`、`lightning-lm NCLT`、`lightning-lm rosbag`、`lightning-lm map index`、`ndt_omp pcl ros2 build`、`pclomp ndt_omp issue`、`fast_gicp ros2 humble`、`fast_gicp cuda build issue`、`fast_gicp VGICP issue`、`small_gicp pcl registration issue`、`small_gicp VGICP issue`、`gtsam_points build issue`、`gtsam_points CUDA GTSAM issue`、`gtsam_points VGICP issue`。

## 官方资料摘要

| 来源 | 相关事实 | 对当前项目的含义 |
| --- | --- | --- |
| [gaoxiang12/lightning-lm README](https://github.com/gaoxiang12/lightning-lm) | 官方说明 lightning-lm 是 LiDAR mapping/localization 模块，依赖 ROS2 Humble、Pangolin、OpenCV、PCL、yaml-cpp、glog、gflags、pcl_conversions，并通过 `colcon build` 构建。 | 阶段一独立包保留这些依赖合理；当前 Windows 环境不能替代 Ubuntu/ROS2 构建验证。 |
| [koide3/ndt_omp README](https://github.com/koide3/ndt_omp) | NDT-OMP 是多线程/SSE 友好的 NDT，官方页面提示非 ROS1/ROS2 使用可参考 fork。 | 当前包使用内置/迁移 pclomp 路线，应在 ROS2/PCL 组合下做真实构建验证。 |
| [koide3/fast_gicp README](https://github.com/koide3/fast_gicp) | fast_gicp 包含 GICP/VGICP/CUDA NDT 等，README 标注 CUDA 可选并有 `BUILD_VGICP_CUDA`。 | 阶段二接入必须 optional，CUDA 不得默认必需。 |
| [koide3/small_gicp README](https://github.com/koide3/small_gicp) | small_gicp 是 header-only C++ 库，核心依赖少，OpenMP/TBB/PCL 为可选。 | 适合作为阶段二较低风险的 optional CPU backend 候选。 |
| [koide3/gtsam_points README](https://github.com/koide3/gtsam_points) | gtsam_points 依赖 GTSAM，CUDA/TBB/OpenMP/PCL 可选，支持 Ubuntu 22.04/24.04 和若干 CUDA/GTSAM 版本。 | 适合作为可选重定位/因子图增强路线，不应成为默认依赖。 |
| [diagnostic_msgs Humble docs](https://docs.ros.org/en/ros2_packages/humble/api/diagnostic_msgs/) | `DiagnosticArray`、`DiagnosticStatus`、`KeyValue` 用于机器人状态诊断。 | 当前 `/localization/diagnostics` 使用标准消息路线合理。 |
| [geometry_msgs Humble docs](https://docs.ros.org/en/humble/p/geometry_msgs/README.html) | `PoseStamped` 表达带 frame 和 timestamp 的 pose。 | `/localization/pose` 使用 `PoseStamped` 合理。 |
| [std_msgs Humble docs](https://docs.ros.org/en/humble/p/std_msgs/index.html) | 官方建议若有明确语义，优先使用有意义字段的消息类型。 | `std_msgs/String` status 适合作为阶段一轻量输出；阶段二可增加结构化消息或 service。 |

## issue 与风险结果表

| 来源 | Issue/链接 | 标题 | 状态 | 相关模块 | 对当前项目影响 | 阶段二处理建议 |
| --- | --- | --- | --- | --- | --- | --- |
| lightning-lm | [#61](https://github.com/gaoxiang12/lightning-lm/issues/61) | 多线机械雷达建图漂移问题 | Open | 传感器适配、建图/定位质量 | 说明机械雷达和自采数据可能存在漂移/适配风险。 | 建立传感器 profile、字段检查和自采数据验证矩阵。 |
| lightning-lm | [#60](https://github.com/gaoxiang12/lightning-lm/issues/60) | 关于 Lidar 倒置安装涉及的参数调整 | Open | 外参、坐标系、传感器安装 | 倒置/非标准安装会影响点云坐标和初始化。 | 阶段二增加外参审计、frame 文档和 RViz2 验证步骤。 |
| lightning-lm | [#59](https://github.com/gaoxiang12/lightning-lm/issues/59) | 自采数据的适配问题 | Open | 数据接入 | 自采 bag/topic/字段/外参是高频风险。 | 建立输入数据审计脚本和传感器适配手册。 |
| lightning-lm | [#58](https://github.com/gaoxiang12/lightning-lm/issues/58) | ARM 上运行计算时间不稳定、核心绑定和线程优先级问题 | Open | 性能、实时性 | 长时间定位可能受线程调度影响。 | 记录匹配耗时 P95、队列长度，提供线程参数和部署建议。 |
| lightning-lm | [#57](https://github.com/gaoxiang12/lightning-lm/issues/57) | 回环检测报错 | Open | 回环/PGO | 当前定位包主要关注 localization，但 PGO/地图一致性仍需注意。 | 阶段二明确 localization 与 loop closure 边界。 |
| lightning-lm | [#55](https://github.com/gaoxiang12/lightning-lm/issues/55) | NCLT 数据 -O2 和 -O3 编译结果差异很大 | Open | 编译优化、数值稳定 | Release 编译选项可能影响定位结果。 | 固定编译矩阵，记录默认优化级别和轨迹回归。 |
| lightning-lm | [#46](https://github.com/gaoxiang12/lightning-lm/issues/46) | 关于重定位问题 | Open | 重定位 | 与阶段二初始化/重定位增强直接相关。 | 优先实现重定位 service、状态机和成功率指标。 |
| lightning-lm | [#42](https://github.com/gaoxiang12/lightning-lm/issues/42) | 如何手动拼接地图 | Open | 地图构建/metadata | 地图拼接和版本管理会影响定位地图一致性。 | 引入 metadata/hash/chunk 校验。 |
| lightning-lm | [#40](https://github.com/gaoxiang12/lightning-lm/issues/40) | 关于使用 NCLT 数据建图与在线定位的问题 | Open | 数据集验证 | NCLT 是建议验证数据，路径和流程需可复现。 | 阶段二把 NCLT 纳入人工验证矩阵。 |
| NDT-OMP | [#78](https://github.com/koide3/ndt_omp/issues/78) | heavily on planar environment assumptions | Open | NDT 退化 | 平面退化会影响 scan-to-map 收敛质量。 | 增加退化检测和 failure reason，不只依赖 probability。 |
| NDT-OMP | [#70](https://github.com/koide3/ndt_omp/issues/70) | `vector::reserve` length_error | Open | NDT runtime | 特定输入可能触发异常。 | 包装 backend 异常并输出 diagnostics。 |
| NDT-OMP | [#67](https://github.com/koide3/ndt_omp/issues/67) | Can't found PCL library when using ndt_omp in other catkin packages | Open | PCL/build | 上游存在 PCL 查找/集成风险。 | 保持当前内置 pclomp 路线并做 ROS2 colcon 构建验证。 |
| NDT-OMP | [#66](https://github.com/koide3/ndt_omp/issues/66) | Crash app when using scan_matching on ubuntu 22 | Open | Ubuntu 22 runtime | 与目标 Ubuntu 22.04 环境相关。 | 阶段二必须进行真实 Ubuntu 22.04 回放验证。 |
| NDT-OMP | [#63](https://github.com/koide3/ndt_omp/issues/63) | Registration running error | Open | NDT registration | 匹配失败需要结构化处理。 | backend 捕获失败并输出 failure reason。 |
| NDT-OMP | [#61](https://github.com/koide3/ndt_omp/issues/61) | how to know degradation about ndt | Open | 退化检测 | 与当前质量评价短板一致。 | 引入 degeneracy metrics 和场景标注验证。 |
| NDT-OMP | [#48](https://github.com/koide3/ndt_omp/issues/48) | registration time is unstable | Open | 性能 | 匹配耗时抖动会影响实时定位。 | 记录 average/P95 elapsed time，避免高频写盘。 |
| NDT-OMP | [#47](https://github.com/koide3/ndt_omp/issues/47) | Gradient check not compatible with latest PCL | Open | PCL 兼容性 | PCL 版本升级可能影响编译。 | 固定 PCL/ROS2 版本矩阵，CI 检查。 |
| fast_gicp | [#170](https://github.com/koide3/fast_gicp/issues/170) | NDT_CUDA find_voxel_correspondences Failed | Open | CUDA NDT | CUDA 路线存在运行风险。 | CUDA backend optional，错误不影响默认 NDT-OMP。 |
| fast_gicp | [#169](https://github.com/koide3/fast_gicp/issues/169) | registration failure for large rotation | Open | GICP/VGICP 鲁棒性 | 大姿态误差重定位可能失败。 | 后端对比需包含大初值偏差场景。 |
| fast_gicp | [#158](https://github.com/koide3/fast_gicp/issues/158) | can't build fast_gicp.lib | Open | build | 构建平台差异风险。 | fast_gicp optional，独立构建文档。 |
| fast_gicp | [#154](https://github.com/koide3/fast_gicp/issues/154) | core dumped | Open | runtime | 后端异常需隔离。 | backend 层捕获异常，输出 failure reason。 |
| fast_gicp | [#151](https://github.com/koide3/fast_gicp/issues/151) | Optimization Problem for Fast GICP (OpenMP) | Open | OpenMP/优化 | 线程参数可能影响性能和收敛。 | 后端参数暴露并纳入性能验证。 |
| small_gicp | [#3](https://github.com/koide3/small_gicp/issues/3) | Posting the way you are using small_gicp | Open | 使用案例/PCL 替换 | 说明 small_gicp 可被用于替换/适配现有 registration。 | 先做 CPU optional adapter，保留 PCL 点云适配边界。 |
| gtsam_points | [#37](https://github.com/koide3/gtsam_points/issues/37) | CMake unable to find GTSAM | Open | GTSAM/build/CUDA | GTSAM 查找和 CUDA 构建复杂。 | gtsam_points 不作为默认依赖，建立独立构建矩阵。 |
| GLIM/gtsam_points 相关 | [koide3/glim#232](https://github.com/koide3/glim/issues/232) | GTSAM 4.3a0 + gtsam_points shared_ptr mismatch | Open | GTSAM 版本/API | gtsam_points 与 GTSAM 版本组合敏感。 | 固定 GTSAM 版本并记录兼容矩阵。 |
| GLIM/gtsam_points 相关 | [koide3/glim#179](https://github.com/koide3/glim/issues/179) | Missing gtsam_points/ann/kdtreex.hpp on Jetson Orin Nano | Open | Jetson/ROS2/CUDA/build | ARM/CUDA 环境风险高。 | Jetson 单独验证，不影响默认 CPU 路线。 |
| GLIM/gtsam_points 相关 | [koide3/glim#159](https://github.com/koide3/glim/issues/159) | Mention CMAKE_CUDA_ARCHITECTURES | Open | CUDA 架构 | CUDA arch 配置影响构建和运行。 | CUDA backend 文档必须要求显式架构配置。 |
| GLIM/gtsam_points 相关 | [koide3/glim#128](https://github.com/koide3/glim/issues/128) | CPU version still gets CUDA related error | Open | optional CUDA dependency | 即使配置 CPU，已安装配置也可能引入 CUDA 依赖。 | optional deps 要通过独立 workspace 和 clean build 验证。 |

## 与当前项目直接相关的问题

1. lightning-lm 自采数据、NCLT、重定位、机械雷达、倒置安装和性能抖动 issue 与 `lightning_localization` 阶段二直接相关。
2. NDT-OMP 的 Ubuntu 22、PCL 兼容、退化、运行异常和耗时不稳定 issue 与默认 matcher 直接相关。
3. ROS2 标准消息文档支持当前 topic 设计，但 `std_msgs/String` status 长期可维护性有限，阶段二可增加结构化状态接口。

## 与阶段二可选依赖相关的问题

| 依赖 | 风险 |
| --- | --- |
| small_gicp | 总体依赖较轻，但 PCL wrapper、OpenMP/TBB 选择和点云协方差语义需要适配。 |
| fast_gicp | 历史上偏 ROS1/catkin，CUDA 和 OpenMP 问题较多，接入应 CPU-only 起步。 |
| VGICP | 对 voxel 参数、地图密度、内存和初值更敏感，必须通过统一指标比较。 |
| gtsam_points | GTSAM/CUDA/Jetson 版本组合复杂，适合作为可选增强，不适合默认依赖。 |

## ROS2/colcon/PCL/CUDA/GTSAM 环境风险

| 环境项 | 风险 | 规避建议 |
| --- | --- | --- |
| ROS2 Humble | 本机未验证，依赖 rosidl/ament/tf2/rosbag2 | Ubuntu 22.04 clean workspace 验证。 |
| colcon | 包依赖和 install 规则可能只在真实环境暴露 | 建立 CI 或人工日志。 |
| PCL | NDT-OMP、pcl_conversions、点云字段受版本影响 | 固定目标 PCL 版本并记录。 |
| CUDA | fast_gicp/gtsam_points optional 后端需要架构和 toolkit 匹配 | 默认关闭，GPU 单独矩阵。 |
| GTSAM | gtsam_points 对 GTSAM 版本敏感 | 固定版本，避免污染默认构建。 |

## 阶段二规避策略

1. 默认构建只依赖阶段一已有 NDT-OMP/pclomp 路线。
2. optional backend 使用 CMake option，未启用时不 `find_package`。
3. 每个 optional backend 有独立文档、构建命令和 failure reason。
4. 后端异常不传播为进程崩溃，统一转为 `RegistrationResult` failure。
5. 所有后端必须输出统一指标，不能只比较单帧耗时。
6. CUDA/GTSAM 在单独 workspace 验证，不影响默认 ROS2 package。

## 无法联网时的人工 issue 搜索流程

当前已联网成功；若之后环境无法访问 GitHub，可按以下流程人工补充：

| 步骤 | 操作 |
| --- | --- |
| 1 | 打开对应官方仓库的 Issues 页面。 |
| 2 | 使用本文“搜索关键词”逐项搜索。 |
| 3 | 记录 issue 编号、标题、状态、时间、相关模块、影响和处理建议。 |
| 4 | 区分“当前项目直接风险”和“optional dependency 外部风险”。 |
| 5 | 不把未复现 issue 写成本项目已发生问题。 |

人工记录模板：

| 来源 | Issue/链接 | 标题 | 状态 | 相关模块 | 对当前项目影响 | 阶段二处理建议 |
| --- | --- | --- | --- | --- | --- | --- |
| 仓库名 | 链接 | 标题 | Open/Closed | 模块 | 影响 | 建议 |
