"""Nav2 (A* + Regulated Pure Pursuit) on the real robot.

Starts map_server, the base_link -> camera static TF and Nav2. It expects the rest
of the TF tree to come from elsewhere:
    map -> odom        RTAB-Map localization
    odom -> base_link  iRobot Create 3 (also /odom, and it drives from /cmd_vel)

    ros2 launch src/nav/launch/robot.launch.py map:=/root/ros2_ws/maps/370J_7th.yaml
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

# Resolved relative to this file so it also works before src/nav is a colcon package.
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_PARAMS = os.path.join(ROOT, 'config', 'nav2_robot.yaml')


def generate_launch_description():
    map_yaml = LaunchConfiguration('map')
    params = LaunchConfiguration('params_file')
    bringup = get_package_share_directory('nav2_bringup')

    args = [
        DeclareLaunchArgument('map', description='Nav2 map YAML (from nav_sim/scripts/extract_map.py)'),
        DeclareLaunchArgument('params_file', default_value=DEFAULT_PARAMS,
                              description='Nav2 parameters'),
        DeclareLaunchArgument('camera_frame', default_value='camera_link',
                              description='Root frame of the RealSense driver'),
        # Camera mount on the robot, metres / radians. TODO: measure; 0 until then.
        DeclareLaunchArgument('cam_x', default_value='0.0'),
        DeclareLaunchArgument('cam_y', default_value='0.0'),
        DeclareLaunchArgument('cam_z', default_value='0.0'),
        DeclareLaunchArgument('cam_yaw', default_value='0.0'),
    ]

    return LaunchDescription(args + [
        Node(package='tf2_ros', executable='static_transform_publisher',
             name='base_to_camera_tf', output='log',
             arguments=['--x', LaunchConfiguration('cam_x'),
                        '--y', LaunchConfiguration('cam_y'),
                        '--z', LaunchConfiguration('cam_z'),
                        '--yaw', LaunchConfiguration('cam_yaw'),
                        '--frame-id', 'base_link',
                        '--child-frame-id', LaunchConfiguration('camera_frame')]),

        Node(package='nav2_map_server', executable='map_server', name='map_server', output='screen',
             parameters=[params, {'yaml_filename': map_yaml}]),
        Node(package='nav2_lifecycle_manager', executable='lifecycle_manager',
             name='lifecycle_manager_map', output='screen',
             parameters=[{'autostart': True, 'node_names': ['map_server']}]),

        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(bringup, 'launch', 'navigation_launch.py')),
            launch_arguments={'params_file': params, 'use_sim_time': 'False',
                              'autostart': 'True'}.items()),
    ])
