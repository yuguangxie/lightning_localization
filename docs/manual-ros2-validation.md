# Manual ROS2 Validation

## Environment

Use Ubuntu 22.04 or compatible, ROS2 Humble or newer, and colcon. Install dependencies equivalent to the upstream README and `scripts/install_dep.sh`:

```bash
sudo apt update
sudo apt install -y \
  libopencv-dev libpcl-dev pcl-tools libyaml-cpp-dev \
  libgoogle-glog-dev libgflags-dev ros-humble-pcl-conversions \
  ros-humble-tf2-ros ros-humble-rosbag2-cpp
```

Pangolin and OpenGL dependencies must be available before building with UI enabled.

## Build

Place this package in a ROS2 workspace `src/` directory:

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-select lightning_localization
source install/setup.bash
```

Expected acceptance:

- `run_loc_online` executable exists under `install/lightning_localization/lib/lightning_localization/`.
- `run_loc_offline` executable exists under `install/lightning_localization/lib/lightning_localization/`.
- Livox custom message headers are generated.

## Offline Behavior Comparison

Prepare the same config, map, and bag used with upstream lightning-lm:

```bash
ros2 run lightning_localization run_loc_offline \
  --config /path/to/default_nclt.yaml \
  --input_bag /path/to/bag \
  --map_path /path/to/data/new_map/
```

Compare against the upstream package using the same input. Record:

- initialization status and logs
- `confidence`
- localization status transitions
- timing output from `Timer::PrintAll()`
- failure messages
- generated `recover_pose.txt`

## Online Behavior Comparison

```bash
ros2 run lightning_localization run_loc_online --config /path/to/default_nclt.yaml
```

With sensors or bag replay active, check:

- configured IMU topic is received
- configured point cloud topic is received
- Livox topic is not duplicated with standard point cloud input unless intended
- `map -> base_link` TF is published when `system.pub_tf=true`
- Pangolin UI starts when `system.with_ui=true`

## RViz And TF

```bash
ros2 run tf2_ros tf2_echo map base_link
rviz2
```

Acceptance:

- TF timestamps advance.
- Pose is continuous after initialization.
- No unexplained frame name change from upstream behavior.

## Trajectory And Metrics

Export upstream and extracted package poses using the same bag and map. Compare:

- ATE
- RPE
- confidence trend
- lost-frame count
- initialization success
- average and P95 processing time

Any metric comparison is pending manual ROS2 environment validation because the current Codex workspace has no ROS2, colcon, bag, or map data.

