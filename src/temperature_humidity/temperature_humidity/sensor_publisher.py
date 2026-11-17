import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Temperature, RelativeHumidity


class SensorPublisher(Node):
    def __init__(self):
        super().__init__('temperature_humidity_publisher')
        self.temp_pub = self.create_publisher(
            Temperature, '/temperature', 10)
        self.humidity_pub = self.create_publisher(
            RelativeHumidity, '/humidity', 10)
        self.timer = self.create_timer(1.0, self.publish_readings)

    def publish_readings(self):
        stamp = self.get_clock().now().to_msg()

        temp = Temperature()
        temp.header.stamp = stamp
        temp.header.frame_id = 'environment_sensor'
        temp.temperature = 25.3

        humidity = RelativeHumidity()
        humidity.header.stamp = stamp
        humidity.header.frame_id = 'environment_sensor'
        humidity.relative_humidity = 0.60

        self.temp_pub.publish(temp)
        self.humidity_pub.publish(humidity)
        self.get_logger().info('SIMULATION: 25.3 C, 60% humidity')


def main(args=None):
    rclpy.init(args=args)
    node = SensorPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == '__main__':
    main()
