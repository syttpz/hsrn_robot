from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='temperatur_humidity',
            executable='sensor_publisher',
            name='temperature_humidity_publisher',
            output='screen',
        ),
    ])
