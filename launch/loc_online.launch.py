from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    config = LaunchConfiguration("config")

    return LaunchDescription([
        DeclareLaunchArgument(
            "config",
            default_value="./config/default.yaml",
            description="Path to a lightning localization YAML config file.",
        ),
        Node(
            package="lightning_localization",
            executable="run_loc_online",
            name="lightning_localization_online",
            output="screen",
            arguments=["--config", config],
        ),
    ])
