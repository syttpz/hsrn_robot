"""Simulated robot + map server + Nav2 (A*, Regulated Pure Pursuit) + RViz."""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PARAMS = os.path.join(ROOT, 'config', 'nav2_params.yaml')
RVIZ = os.path.join(ROOT, 'config', 'nav2.rviz')


def generate_launch_description():
    map_yaml = LaunchConfiguration('map')
    bringup = get_package_share_directory('nav2_bringup')

    return LaunchDescription([
        DeclareLaunchArgument('map', description='Nav2 map YAML'),

        ExecuteProcess(cmd=['python3', os.path.join(ROOT, 'scripts', 'sim_robot.py')],
                       name='sim_robot', output='screen'),

        # Perfect localization: the map and odom frames coincide.
        Node(package='tf2_ros', executable='static_transform_publisher', output='log',
             arguments=['--frame-id', 'map', '--child-frame-id', 'odom']),

        Node(package='nav2_map_server', executable='map_server', name='map_server', output='screen',
             parameters=[PARAMS, {'yaml_filename': map_yaml}]),
        Node(package='nav2_lifecycle_manager', executable='lifecycle_manager',
             name='lifecycle_manager_map', output='screen',
             parameters=[{'autostart': True, 'node_names': ['map_server']}]),

        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(bringup, 'launch', 'navigation_launch.py')),
            launch_arguments={'params_file': PARAMS, 'use_sim_time': 'False',
                              'autostart': 'True'}.items()),

        Node(package='rviz2', executable='rviz2', output='log', arguments=['-d', RVIZ]),
    ])
