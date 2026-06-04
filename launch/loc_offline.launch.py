from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    config = LaunchConfiguration("config")
    input_bag = LaunchConfiguration("input_bag")
    map_path = LaunchConfiguration("map_path")

    return LaunchDescription([
        DeclareLaunchArgument(
            "config",
            default_value="./config/default.yaml",
            description="Path to a lightning localization YAML config file.",
        ),
        DeclareLaunchArgument(
            "input_bag",
            default_value="",
            description="Path to the ROS2 bag to replay offline.",
        ),
        DeclareLaunchArgument(
            "map_path",
            default_value="./data/new_map/",
            description="Path to a tiled lightning-lm localization map.",
        ),
        Node(
            package="lightning_localization",
            executable="run_loc_offline",
            name="lightning_localization_offline",
            output="screen",
            arguments=["--config", config, "--input_bag", input_bag, "--map_path", map_path],
        ),
    ])
