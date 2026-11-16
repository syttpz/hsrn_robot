from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    # 1. 启动 YOLO 节点
    yolo_node = Node(
        package='yolo_detector',
        executable='yolo_node',
        name='yolo_detector_node',
        output='screen'
    )

    # 2. 启动 RealSense 相机驱动 (这里假设你的系统里已经安装了 realsense2_camera 包)
    # 注意：如果你们组用的是自定义的相机 launch 文件，请在这里替换
    realsense_dir = get_package_share_directory('realsense2_camera')
    realsense_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(realsense_dir, 'launch', 'rs_launch.py')
        ),
        launch_arguments={
            'depth_module.depth_profile': '640x480x6',
            'rgb_camera.color_profile': '640x480x6'
        }.items()
    )

    return LaunchDescription([
        realsense_launch,
        yolo_node
    ])