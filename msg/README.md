# Message Layout

The extracted localization package does not define additional package-local messages. It uses standard ROS2 messages plus Livox custom messages from the bundled `thirdparty/livox_ros_driver` subpackage:

- `livox_ros_driver2/msg/CustomPoint`
- `livox_ros_driver2/msg/CustomMsg`

Package-local service definitions are kept in `srv/` because they existed in the upstream project. `LocCmd.srv` is preserved for the current relocalization interface definition, although the online localization runtime does not register that service in stage one.

