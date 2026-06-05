from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch.substitutions import PathJoinSubstitution


def generate_launch_description():
    config = LaunchConfiguration("config")
    use_rviz = LaunchConfiguration("use_rviz")
    rviz_config = LaunchConfiguration("rviz_config")
    use_sim_time = LaunchConfiguration("use_sim_time")
    initial_pose_source = LaunchConfiguration("initial_pose_source")
    initialpose_topic = LaunchConfiguration("initialpose_topic")

    return LaunchDescription([
        DeclareLaunchArgument(
            "config",
            default_value="./config/default.yaml",
            description="Path to a lightning localization YAML config file.",
        ),
        DeclareLaunchArgument(
            "use_rviz",
            default_value="false",
            description="Start RViz2 with the lightning localization RViz config.",
        ),
        DeclareLaunchArgument(
            "rviz_config",
            default_value=PathJoinSubstitution([
                FindPackageShare("lightning_localization"),
                "rviz",
                "lightning_localization.rviz",
            ]),
            description="Path to the RViz2 config file.",
        ),
        DeclareLaunchArgument(
            "use_sim_time",
            default_value="false",
            description="Set ROS use_sim_time parameter on the localization node.",
        ),
        DeclareLaunchArgument(
            "initial_pose_source",
            default_value="",
            description="Override initialization.source at launch time; empty keeps the YAML value.",
        ),
        DeclareLaunchArgument(
            "initialpose_topic",
            default_value="",
            description="Override initialization.rviz_initialpose.topic at launch time; empty keeps the YAML value.",
        ),
        Node(
            package="lightning_localization",
            executable="run_loc_online",
            name="lightning_localization_online",
            output="screen",
            arguments=["--config", config],
            parameters=[{
                "use_sim_time": use_sim_time,
                "initial_pose_source": initial_pose_source,
                "initialpose_topic": initialpose_topic,
            }],
        ),
        Node(
            package="rviz2",
            executable="rviz2",
            name="rviz2",
            output="screen",
            arguments=["-d", rviz_config],
            condition=IfCondition(use_rviz),
        ),
    ])
