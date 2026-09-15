"""
    robot                          corelink                    server (this)
    ----------------------------   ---------   -------------------------------
    /camera/.../color/image_raw    --UDP-->    color_image_receiver  -.
    /camera/.../aligned_depth...   --UDP-->    depth_image_receiver  -+-> rgbd_sync
    /camera/.../color/camera_info  --UDP-->    camera_info_receiver  -'      |
                                                                             v
                                                              rgbd_odometry -> rtabmap

"""

import os
from datetime import datetime

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, Shutdown
from launch.conditions import IfCondition, UnlessCondition
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue

QOS_BEST_EFFORT = '2'

RGB_TOPIC = '/camera/camera/color/image_raw'
DEPTH_TOPIC = '/camera/camera/aligned_depth_to_color/image_raw'
INFO_TOPIC = '/camera/camera/color/camera_info'
RGB_COMPRESSED_TOPIC = RGB_TOPIC + '/compressed'
DEPTH_COMPRESSED_TOPIC = DEPTH_TOPIC + '/compressedDepth'


def _receiver(name, topic, msgtype, params_file, protocol, reliability,
              stream_type=None, condition=None, peer_timeout=10):
    return Node(
        package='ros2_bridge_node',
        executable='ros2_bridge_node',
        name=name,
        output='screen',
        condition=condition,
        on_exit=Shutdown(reason=f'{name} exited'),
        parameters=[
            params_file,
            {
                'topic.name': topic,
                'topic.type': msgtype,
                'topic.direction': 'from_corelink',
                'corelink.stream_type': stream_type or topic,
                'corelink.data_protocol': protocol,
                'corelink.peer_activity_timeout_s': peer_timeout,
                'qos.reliability': reliability,
            },
        ],
    )


def generate_launch_description():
    bridge_share = get_package_share_directory('ros2_bridge_node')
    default_params = os.path.join(bridge_share, 'config', 'server_side.yaml')

    params_file = LaunchConfiguration('params_file')
    protocol = LaunchConfiguration('protocol')
    reliability = LaunchConfiguration('reliability')
    frame_id = LaunchConfiguration('frame_id')
    use_sim_time = LaunchConfiguration('use_sim_time')
    approx_sync_max_interval = LaunchConfiguration('approx_sync_max_interval')
    queue_size = LaunchConfiguration('queue_size')
    peer_timeout = ParameterValue(
        LaunchConfiguration('peer_activity_timeout_s'), value_type=int)

    default_run_id = os.environ.get('RUN_ID') or datetime.now().strftime('%Y%m%d-%H%M%S')

    default_protocol = os.environ.get('CORELINK_PROTOCOL', 'udp')
    database_path = ParameterValue(
        [LaunchConfiguration('database_dir'), '/rtabmap_',
         LaunchConfiguration('run_id'), '.db'],
        value_type=str)

    qos = ParameterValue(
        PythonExpression(
            ["'", QOS_BEST_EFFORT, "' if '", reliability, "' == 'best_effort' else '1'"]),
        value_type=int)
    max_interval = ParameterValue(approx_sync_max_interval, value_type=float)
    queue = ParameterValue(queue_size, value_type=int)

    common = {'use_sim_time': ParameterValue(use_sim_time, value_type=bool)}

    args = [
        DeclareLaunchArgument(
            'params_file', default_value=default_params,
            description='Corelink params for the receivers (from_corelink side)'),
        DeclareLaunchArgument(
            'protocol', default_value=default_protocol,
            description='Corelink data protocol: udp | tcp | ws. Defaults to '
                        '$CORELINK_PROTOCOL so the deployment can switch it '
                        'without rebuilding the image'),
        DeclareLaunchArgument(
            'reliability', default_value='best_effort',
            description='ROS QoS on both the receivers and the rtabmap nodes; '
                        'these must agree or no data flows'),
        DeclareLaunchArgument(
            'frame_id', default_value='openni_rgb_optical_frame',
            description='Fixed frame of the camera. Defaults to the TUM bag RGB '
                        'optical frame so no tf has to cross the network'),
        DeclareLaunchArgument(
            'use_sim_time', default_value='false',
            description='Requires bridge_clock:=true and /clock bridged from the '
                        'robot, otherwise every node blocks forever'),
        DeclareLaunchArgument(
            'bridge_clock', default_value='false',
            description='Also receive /clock over Corelink'),
        DeclareLaunchArgument(
            'compress_rgb', default_value='true',
            description='Receive JPEG RGB and decode it before rgbd_sync'),
        DeclareLaunchArgument(
            'compress_depth', default_value='true',
            description='Receive lossless compressedDepth and decode it before rgbd_sync'),
        DeclareLaunchArgument(
            'run_id', default_value=default_run_id,
            description='Names the database, so every pod start writes its own. '
                        'Defaults to $RUN_ID, else a launch-time timestamp'),
        DeclareLaunchArgument(
            'database_dir', default_value='/root/.ros',
            description='The PVC mount point in k8s'),
        DeclareLaunchArgument(
            'delete_db_on_start', default_value='false',
            description='Off because run_id already gives each run a fresh file. '
                        'Turning it on would delete the previous run\'s map before '
                        'it could be copied off the volume'),
        DeclareLaunchArgument(
            'approx_sync_max_interval', default_value='0.05',
            description='RGB-D pairing window in seconds. The TUM bags pair at '
                        'median 11.9 ms / p95 17.6 ms; 50 ms also tolerates '
                        'independent pre-transport throttling'),
        DeclareLaunchArgument(
            'peer_activity_timeout_s', default_value='10',
            description='Kill the receiver if OTHER Corelink streams are live but '
                        'this one has had no frame for this many seconds. 0 '
                        'disables it -- needed for diagnostic runs, where a '
                        'lossy link would otherwise tear the launch down before '
                        'any data can be measured'),
        DeclareLaunchArgument(
            'queue_size', default_value='30',
            description='Deep enough to ride out reassembly jitter'),
    ]

    receivers = [
        _receiver('color_image_receiver', RGB_COMPRESSED_TOPIC,
                  'sensor_msgs/msg/CompressedImage', params_file, protocol,
                  'reliable', stream_type=RGB_TOPIC, peer_timeout=peer_timeout,
                  condition=IfCondition(LaunchConfiguration('compress_rgb'))),
        _receiver('color_image_receiver', RGB_TOPIC,
                  'sensor_msgs/msg/Image', params_file, protocol, reliability,
                  peer_timeout=peer_timeout,
                  condition=UnlessCondition(LaunchConfiguration('compress_rgb'))),
        _receiver('depth_image_receiver', DEPTH_COMPRESSED_TOPIC,
                  'sensor_msgs/msg/CompressedImage', params_file, protocol,
                  'reliable', stream_type=DEPTH_TOPIC, peer_timeout=peer_timeout,
                  condition=IfCondition(LaunchConfiguration('compress_depth'))),
        _receiver('depth_image_receiver', DEPTH_TOPIC,
                  'sensor_msgs/msg/Image', params_file, protocol, reliability,
                  peer_timeout=peer_timeout,
                  condition=UnlessCondition(LaunchConfiguration('compress_depth'))),
        _receiver('camera_info_receiver', INFO_TOPIC,
                  'sensor_msgs/msg/CameraInfo', params_file, protocol, reliability,
                  peer_timeout=peer_timeout),
    ]

    rgb_decoder = Node(
        package='image_transport', executable='republish',
        name='rgb_jpeg_decoder', output='screen',
        condition=IfCondition(LaunchConfiguration('compress_rgb')),
        arguments=['compressed', 'raw'],
        remappings=[('in/compressed', RGB_COMPRESSED_TOPIC),
                    ('out', RGB_TOPIC)],
    )

    depth_decoder = Node(
        package='image_transport', executable='republish',
        name='depth_png_decoder', output='screen',
        condition=IfCondition(LaunchConfiguration('compress_depth')),
        arguments=['compressedDepth', 'raw'],
        remappings=[('in/compressedDepth', DEPTH_COMPRESSED_TOPIC),
                    ('out', DEPTH_TOPIC)],
    )

    clock_receiver = Node(
        package='ros2_bridge_node',
        executable='ros2_bridge_node',
        name='clock_receiver',
        output='screen',
        condition=IfCondition(LaunchConfiguration('bridge_clock')),
        parameters=[
            params_file,
            {
                'topic.name': '/clock',
                'topic.type': 'rosgraph_msgs/msg/Clock',
                'topic.direction': 'from_corelink',
                'corelink.stream_type': 'ros2_clock',
                'corelink.data_protocol': protocol,
                'corelink.peer_activity_timeout_s': peer_timeout,
                'qos.reliability': 'reliable',
            },
        ],
    )

    rgbd_sync = Node(
        package='rtabmap_sync', executable='rgbd_sync', name='rgbd_sync',
        output='screen',
        parameters=[common, {
            'approx_sync': True,
            'approx_sync_max_interval': max_interval,
            'sync_queue_size': queue,
            'qos': qos,
            'qos_camera_info': qos,
        }],
        remappings=[
            ('rgb/image', RGB_TOPIC),
            ('depth/image', DEPTH_TOPIC),
            ('rgb/camera_info', INFO_TOPIC),
            ('rgbd_image', '/rgbd_image'),
        ],
    )

    rgbd_odometry = Node(
        package='rtabmap_odom', executable='rgbd_odometry', name='rgbd_odometry',
        output='screen',
        parameters=[common, {
            'frame_id': frame_id,
            'odom_frame_id': 'odom',
            'subscribe_rgbd': True,
            'approx_sync': True,
            'sync_queue_size': queue,
            'qos': qos,
            'publish_tf': True,
            'Odom/ResetCountdown': '1',
            'Vis/MinInliers': '10',
            'wait_for_transform': 0.0,
        }],
        remappings=[
            ('rgbd_image', '/rgbd_image'),
            ('odom', '/odom'),
        ],
    )

    def _rtabmap(name, condition, arguments):
        return Node(
            package='rtabmap_slam', executable='rtabmap', name=name,
            output='screen',
            condition=condition,
            parameters=[common, {
                'frame_id': frame_id,
                'odom_frame_id': '',
                'subscribe_rgbd': True,
                'subscribe_odom_info': True,
                'subscribe_depth': False,
                'subscribe_scan': False,
                'approx_sync': True,
                'sync_queue_size': queue,
                'qos': qos,
                'database_path': database_path,
                'wait_for_transform': 0.0,
            }],
            remappings=[
                ('rgbd_image', '/rgbd_image'),
                ('odom', '/odom'),
                ('odom_info', '/odom_info'),
            ],
            arguments=arguments,
        )

    delete_db = LaunchConfiguration('delete_db_on_start')
    rtabmap_fresh = _rtabmap('rtabmap', IfCondition(delete_db), ['-d'])
    rtabmap_resume = _rtabmap('rtabmap', UnlessCondition(delete_db), [])

    return LaunchDescription(
        args + receivers
        + [rgb_decoder, depth_decoder, clock_receiver, rgbd_sync, rgbd_odometry,
           rtabmap_fresh, rtabmap_resume])
