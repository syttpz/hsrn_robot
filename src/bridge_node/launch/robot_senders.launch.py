'''
Robot side offload test
'''

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, Shutdown
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

RGB_TOPIC = '/camera/camera/color/image_raw'
DEPTH_TOPIC = '/camera/camera/aligned_depth_to_color/image_raw'
INFO_TOPIC = '/camera/camera/color/camera_info'
RGB_COMPRESSED_TOPIC = RGB_TOPIC + '/compressed'
DEPTH_COMPRESSED_TOPIC = DEPTH_TOPIC + '/compressedDepth'


def generate_launch_description():
    share = get_package_share_directory('ros2_bridge_node')
    default_params = os.path.join(share, 'config', 'robot_side.yaml')
    default_creds = os.path.join(share, 'config', 'credentials.yaml')

    params_file = LaunchConfiguration('params_file')
    credentials_file = LaunchConfiguration('credentials_file')
    protocol = LaunchConfiguration('protocol')
    reliability = LaunchConfiguration('reliability')
    queue_max = ParameterValue(
        LaunchConfiguration('pacing_queue_max'), value_type=int)
    auto_rate = ParameterValue(
        LaunchConfiguration('auto_target_rate_hz'), value_type=float)
    forward_rate = ParameterValue(
        LaunchConfiguration('forward_rate_hz'), value_type=float)
    auto_reserve = ParameterValue(
        LaunchConfiguration('auto_reserve_fraction'), value_type=float)

    def sender(name, topic, msgtype, pacing_arg, stream_type=None,
               condition=None):
        return Node(
            package='ros2_bridge_node',
            executable='ros2_bridge_node',
            name=name,
            output='screen',
            condition=condition,
            on_exit=Shutdown(reason=f'{name} exited'),
            parameters=[
                params_file,
                credentials_file,
                {
                    'topic.name': topic,
                    'topic.type': msgtype,
                    'topic.direction': 'to_corelink',
                    'topic.max_rate': forward_rate,
                    'corelink.stream_type': stream_type or topic,
                    'corelink.data_protocol': protocol,
                    'qos.reliability': reliability,
                    'send.pacing_us': ParameterValue(
                        LaunchConfiguration(pacing_arg), value_type=int),
                    'send.auto_target_rate_hz': auto_rate,
                    'send.auto_reserve_fraction': auto_reserve,
                    'send.pacing_queue_max': queue_max,
                },
            ],
        )

    return LaunchDescription([
        DeclareLaunchArgument('params_file', default_value=default_params),
        DeclareLaunchArgument(
            'credentials_file', default_value=default_creds,
            description='Gitignored; copy credentials.yaml.example and fill in. '
                        'Loaded after params_file so it wins on shared keys'),
        DeclareLaunchArgument('protocol', default_value='udp'),
        DeclareLaunchArgument(
            'reliability', default_value='best_effort',
            description='Must match the server launch, or no data flows'),
        DeclareLaunchArgument(
            'rgb_pacing_us', default_value='0',
            description='Manual override; 0 uses automatic pacing'),
        DeclareLaunchArgument(
            'depth_pacing_us', default_value='0',
            description='Manual override; 0 uses automatic pacing'),
        DeclareLaunchArgument(
            'info_pacing_us', default_value='0',
            description='Manual override; 0 uses automatic pacing'),
        DeclareLaunchArgument(
            'forward_rate_hz', default_value='0.0',
            description='Optional independent throttle; 0 preserves RGB-D timestamp pairing'),
        DeclareLaunchArgument(
            'auto_target_rate_hz', default_value='6.0',
            description='Expected forwarded frame rate used by automatic pacing'),
        DeclareLaunchArgument(
            'compress_rgb', default_value='true',
            description='JPEG-compress RGB before Corelink; depth remains lossless raw'),
        DeclareLaunchArgument(
            'compress_depth', default_value='true',
            description='Losslessly compress depth as PNG before Corelink'),
        DeclareLaunchArgument(
            'jpeg_quality', default_value='90',
            description='JPEG quality (1-100); 90 is the mapping baseline'),
        DeclareLaunchArgument(
            'auto_reserve_fraction', default_value='0.10',
            description='Fraction of each frame period reserved as headroom'),
        DeclareLaunchArgument(
            'pacing_queue_max', default_value='256',
            description='~4 RGB frames of slack; the 128 default is under 3'),
        DeclareLaunchArgument(
            'bridge_clock', default_value='false',
            description='Send /clock too, for a server running use_sim_time'),

        Node(
            package='image_transport', executable='republish',
            name='rgb_jpeg_encoder', output='screen',
            condition=IfCondition(LaunchConfiguration('compress_rgb')),
            arguments=['raw', 'compressed'],
            remappings=[('in', RGB_TOPIC),
                        ('out/compressed', RGB_COMPRESSED_TOPIC)],
            parameters=[{
                'out.jpeg_quality': ParameterValue(
                    LaunchConfiguration('jpeg_quality'), value_type=int),
            }],
        ),
        sender('color_image_sender', RGB_COMPRESSED_TOPIC,
               'sensor_msgs/msg/CompressedImage', 'rgb_pacing_us',
               stream_type=RGB_TOPIC,
               condition=IfCondition(LaunchConfiguration('compress_rgb'))),
        sender('color_image_sender', RGB_TOPIC,
               'sensor_msgs/msg/Image', 'rgb_pacing_us',
               condition=UnlessCondition(LaunchConfiguration('compress_rgb'))),
        Node(
            package='image_transport', executable='republish',
            name='depth_png_encoder', output='screen',
            condition=IfCondition(LaunchConfiguration('compress_depth')),
            arguments=['raw', 'compressedDepth'],
            remappings=[('in', DEPTH_TOPIC),
                        ('out/compressedDepth', DEPTH_COMPRESSED_TOPIC)],
        ),
        sender('depth_image_sender', DEPTH_COMPRESSED_TOPIC,
               'sensor_msgs/msg/CompressedImage', 'depth_pacing_us',
               stream_type=DEPTH_TOPIC,
               condition=IfCondition(LaunchConfiguration('compress_depth'))),
        sender('depth_image_sender', DEPTH_TOPIC,
               'sensor_msgs/msg/Image', 'depth_pacing_us',
               condition=UnlessCondition(LaunchConfiguration('compress_depth'))),
        sender('camera_info_sender', INFO_TOPIC,
               'sensor_msgs/msg/CameraInfo', 'info_pacing_us'),

        Node(
            package='ros2_bridge_node',
            executable='ros2_bridge_node',
            name='clock_sender',
            output='screen',
            condition=IfCondition(LaunchConfiguration('bridge_clock')),
            on_exit=Shutdown(reason='clock_sender exited'),
            parameters=[
                params_file,
                credentials_file,
                {
                    'topic.name': '/clock',
                    'topic.type': 'rosgraph_msgs/msg/Clock',
                    'topic.direction': 'to_corelink',
                    'corelink.stream_type': 'ros2_clock',
                    'corelink.data_protocol': protocol,
                    'qos.reliability': 'best_effort',
                },
            ],
        ),
    ])
