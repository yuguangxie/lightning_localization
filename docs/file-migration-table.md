# File Migration Table

| Source | Destination | Stage-one decision | Reason |
|---|---|---|---|
| `lightning-lm/src/app/run_loc_online.cc` | `src/app/run_loc_online.cc` | migrated | online localization entry |
| `lightning-lm/src/app/run_loc_offline.cc` | `src/app/run_loc_offline.cc` | migrated | offline rosbag localization entry |
| `lightning-lm/src/core/system/loc_system.*` | `src/core/system/loc_system.*` | migrated | ROS2 subscription and TF wrapper |
| `lightning-lm/src/core/localization/**` | `src/core/localization/**` | migrated | localization coordinator, matcher, result, PGO |
| `lightning-lm/src/core/lio/**` | `src/core/lio/**` | migrated | LIO prediction and point cloud preprocessing dependency |
| `lightning-lm/src/core/maps/**` | `src/core/maps/**` | migrated | tiled map and dynamic layer |
| `lightning-lm/src/core/ivox3d/**` | `src/core/ivox3d/**` | migrated | LIO voxel map dependency |
| `lightning-lm/src/core/miao/core/**` | `src/core/miao/core/**` | migrated | PGO optimizer dependency |
| `lightning-lm/src/core/miao/utils/**` | `src/core/miao/utils/**` | migrated | optimizer utility dependency |
| `lightning-lm/src/common/**` | `src/common/**` | migrated | common pose, point, IMU, option types |
| `lightning-lm/src/io/**` | `src/io/**` | migrated | YAML and file IO |
| `lightning-lm/src/utils/**` | `src/utils/**` | migrated | timer, sync, point cloud helpers |
| `lightning-lm/src/wrapper/**` | `src/wrapper/**` | migrated | rosbag and ROS point conversion |
| `lightning-lm/src/ui/**` | `src/ui/**` | migrated | Pangolin visualization path used by localization |
| `lightning-lm/config/default*.yaml` | `config/default*.yaml` | migrated | preserve config behavior |
| `lightning-lm/srv/*.srv` | `srv/*.srv` | migrated | preserve upstream service interface definitions |
| `lightning-lm/thirdparty/Sophus/**` | `thirdparty/Sophus/**` | migrated | SE3/SO3 dependency |
| `lightning-lm/thirdparty/livox_ros_driver/**` | `thirdparty/livox_ros_driver/**` | migrated | Livox custom message generation |
| `lightning-lm/src/core/system/slam.*` | none | isolated | SLAM runtime, not localization runtime |
| `lightning-lm/src/app/run_slam_*` | none | isolated | SLAM application entry points |
| `lightning-lm/src/app/run_frontend_offline.cc` | none | isolated | frontend-only application |
| `lightning-lm/src/app/run_loop_offline.cc` | none | isolated | loop-closing application |
| `lightning-lm/src/core/loop_closing/**` | none | isolated | not required by localization compile target |
| `lightning-lm/src/core/g2p5/**` | none | isolated | grid map generation/display path, not required by localization target |
| original `lightning-lm/` files | unchanged | cannot delete | protect upstream project and user work |

