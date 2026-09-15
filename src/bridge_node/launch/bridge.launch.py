import os

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

_THIS_DIR = os.path.dirname(os.path.abspath(__file__))
_DEFAULT_CREDENTIALS_FILE = os.path.join(_THIS_DIR, '..', 'config', 'credentials.yaml')


def generate_launch_description():
    params_file = LaunchConfiguration('params_file')
    credentials_file = LaunchConfiguration('credentials_file')

    return LaunchDescription([
        DeclareLaunchArgument(
            'params_file',
            description=(
                'Path to a YAML params file, e.g. '
                'config/robot_side.yaml or config/server_side.yaml'
            ),
        ),
        DeclareLaunchArgument(
            'credentials_file',
            default_value=_DEFAULT_CREDENTIALS_FILE,
            description=(
                'Path to a YAML file providing corelink.username / '
                'corelink.password (gitignored -- copy '
                'config/credentials.yaml.example to config/credentials.yaml '
                'and fill in real values). Loaded after params_file, so it '
                'wins if both set the same key.'
            ),
        ),
        Node(
            package='ros2_bridge_node',
            executable='ros2_bridge_node',
            name='ros2_bridge_node',
            output='screen',
            parameters=[params_file, credentials_file],
        ),
    ])
