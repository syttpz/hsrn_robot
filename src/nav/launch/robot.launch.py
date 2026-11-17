"""Nav2 (A* + Regulated Pure Pursuit) on the real robot.

Default: navigate on a SAVED map. Starts everything except the Create 3:
    RealSense + RTAB-Map localization  map -> odom        (localization.launch.py)
    static TF                          base_link -> camera_link
    map_server                         /map from the saved YAML (what A* plans on)
    Nav2                               A* + Pure Pursuit -> /cmd_vel
    iRobot Create 3                    odom -> base_link and /odom; drives from /cmd_vel

    ros2 launch src/nav/launch/robot.launch.py                  # 370J_7th
    ros2 launch src/nav/launch/robot.launch.py \
        map:=/root/ros2_ws/maps/map_<ts>/my_map.yaml \
        database:=/root/ros2_ws/maps/map_<ts>/rtabmap.db       # a startmapping.sh run

map and database MUST come from the same mapping run, or /map and the robot's
position will not line up. Do not run src/startmapping.sh in this mode.

slam:=true instead navigates while src/startmapping.sh is running; that script then
provides /map, map -> odom and the camera TF, and this launch starts only Nav2.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import UnlessCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

# Resolved relative to this file so it also works before src/nav is a colcon package.
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_PARAMS = os.path.join(ROOT, 'config', 'nav2_robot.yaml')
MAPS = '/root/ros2_ws/maps'  # ./maps on the host, mounted by docker-compose.yml


def generate_launch_description():
    params = LaunchConfiguration('params_file')
    saved_map_mode = UnlessCondition(LaunchConfiguration('slam'))
    bringup = get_package_share_directory('nav2_bringup')

    args = [
        DeclareLaunchArgument('slam', default_value='false',
                              description='true = navigate on top of startmapping.sh '
                                          'instead of a saved map'),
        DeclareLaunchArgument('map', default_value=os.path.join(MAPS, '370J_7th.yaml'),
                              description='Saved map YAML for map_server'),
        DeclareLaunchArgument('database', default_value=os.path.join(MAPS, '370J_7th.db'),
                              description='RTAB-Map database the map YAML was made from'),
        DeclareLaunchArgument('start_camera', default_value='true',
                              description='Start the RealSense driver. false if another '
                                          'driver already publishes aligned depth'),
        DeclareLaunchArgument('params_file', default_value=DEFAULT_PARAMS,
                              description='Nav2 parameters'),
        DeclareLaunchArgument('camera_frame', default_value='camera_link',
                              description='Root frame of the RealSense driver'),
        # Camera mount on the robot, metres / radians. Must match CAM_* in
        # src/startmapping.sh: the map was built with this offset.
        DeclareLaunchArgument('cam_x', default_value='0.15'),
        DeclareLaunchArgument('cam_y', default_value='0.0'),
        DeclareLaunchArgument('cam_z', default_value='0.05'),
        DeclareLaunchArgument('cam_yaw', default_value='0.0'),
    ]

    # In slam mode startmapping.sh already provides all of these; starting them too
    # would duplicate /map, map -> odom and base_link -> camera_link.
    saved_map = [
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(ROOT, 'launch', 'localization.launch.py')),
            condition=saved_map_mode,
            launch_arguments={'database': LaunchConfiguration('database'),
                              'start_camera': LaunchConfiguration('start_camera')}.items()),
        Node(package='tf2_ros', executable='static_transform_publisher',
             name='base_to_camera_tf', output='log', condition=saved_map_mode,
             arguments=['--x', LaunchConfiguration('cam_x'),
                        '--y', LaunchConfiguration('cam_y'),
                        '--z', LaunchConfiguration('cam_z'),
                        '--yaw', LaunchConfiguration('cam_yaw'),
                        '--frame-id', 'base_link',
                        '--child-frame-id', LaunchConfiguration('camera_frame')]),
        Node(package='nav2_map_server', executable='map_server', name='map_server',
             output='screen', condition=saved_map_mode,
             parameters=[params, {'yaml_filename': LaunchConfiguration('map')}]),
        Node(package='nav2_lifecycle_manager', executable='lifecycle_manager',
             name='lifecycle_manager_map', output='screen', condition=saved_map_mode,
             parameters=[{'autostart': True, 'node_names': ['map_server']}]),
    ]

    nav2 = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(bringup, 'launch', 'navigation_launch.py')),
        launch_arguments={'params_file': params, 'use_sim_time': 'False',
                          'autostart': 'True'}.items())

    return LaunchDescription(args + saved_map + [nav2])
