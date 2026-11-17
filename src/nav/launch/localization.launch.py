"""RTAB-Map in localization mode on the robot: publishes map -> odom for Nav2.

Mirrors the mapping setup in src/startmapping.sh (same camera topics, base_link,
Create 3 wheel odometry on /odom) but loads an existing database read-only instead
of building a new one. Use the rtabmap.db and the map YAML from the SAME
maps/map_<timestamp>/ folder, or the map frames will not line up.

    ros2 launch src/nav/launch/localization.launch.py \
        database:=/root/ros2_ws/maps/map_<ts>/rtabmap.db

Normally started from robot.launch.py by passing database:=... there.
"""

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

CAM_NS = '/camera/camera'
QOS_BEST_EFFORT = 2


def generate_launch_description():
    database = LaunchConfiguration('database')

    args = [
        DeclareLaunchArgument('database', description='rtabmap.db from a startmapping.sh run'),
        DeclareLaunchArgument('start_camera', default_value='true',
                              description='Start the RealSense driver here. Set false if '
                                          'another driver already publishes aligned depth; '
                                          'two drivers cannot share the camera'),
    ]

    # Same profiles as startmapping.sh. Aligned depth is required by RTAB-Map.
    camera = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution(
            [FindPackageShare('realsense2_camera'), 'launch', 'rs_launch.py'])),
        condition=IfCondition(LaunchConfiguration('start_camera')),
        launch_arguments={
            'depth_module.depth_profile': '640x480x15',
            'rgb_camera.color_profile': '640x480x15',
            'align_depth.enable': 'true',
            'enable_gyro': 'false',
            'enable_accel': 'false',
        }.items())

    # The rtabmap node directly rather than rtabmap.launch.py, so its occupancy grid
    # can be moved off /map: map_server owns /map for Nav2.
    rtabmap = Node(
        package='rtabmap_slam', executable='rtabmap', name='rtabmap', output='screen',
        parameters=[{
            'frame_id': 'base_link',
            'odom_frame_id': '',          # take odometry from the /odom topic (Create 3)
            'subscribe_depth': True,
            'approx_sync': True,
            'topic_queue_size': 30,
            'sync_queue_size': 30,
            'qos_image': QOS_BEST_EFFORT,
            'qos_camera_info': QOS_BEST_EFFORT,
            'qos_odom': QOS_BEST_EFFORT,
            'publish_tf': True,           # map -> odom
            'wait_for_transform': 0.2,
            'database_path': database,
            'use_sim_time': False,
            # Localization mode: do not add to the map, load all of it at start.
            'Mem/IncrementalMemory': 'false',
            'Mem/InitWMWithAllNodes': 'true',
        }],
        remappings=[
            ('rgb/image', CAM_NS + '/color/image_raw'),
            ('depth/image', CAM_NS + '/aligned_depth_to_color/image_raw'),
            ('rgb/camera_info', CAM_NS + '/color/camera_info'),
            ('odom', '/odom'),
            ('map', '/rtabmap/grid_map'),
        ])

    return LaunchDescription(args + [camera, rtabmap])
