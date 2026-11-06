#!/usr/bin/env python3
"""Simulated differential-drive robot that moves on the Nav2 map.

Subscribes  /cmd_vel, /map
Publishes   /odom, TF odom -> base_footprint

Localization is perfect: map and odom are the same frame. The robot stops and
logs BUMP if its body would touch an occupied map cell.
"""

import math
import os

import numpy as np
import rclpy
from geometry_msgs.msg import TransformStamped, Twist
from nav_msgs.msg import OccupancyGrid, Odometry
from rclpy.node import Node
from rclpy.qos import DurabilityPolicy, QoSProfile
from tf2_ros import TransformBroadcaster

RATE_HZ = 50.0
CMD_TIMEOUT_S = 0.5
OCCUPIED = 65


def disk_offsets(radius_cells):
    """(rows, cols) offsets of the cells inside a disk."""
    r = int(math.ceil(radius_cells))
    rows, cols = np.mgrid[-r:r + 1, -r:r + 1]
    inside = rows ** 2 + cols ** 2 <= radius_cells ** 2
    return rows[inside], cols[inside]


def most_open_cell(free, blocked):
    """(row, col) of the free cell farthest from any blocked cell, by repeated erosion."""
    clear = free & ~blocked
    while True:
        eroded = clear.copy()
        eroded[1:, :] &= clear[:-1, :]
        eroded[:-1, :] &= clear[1:, :]
        eroded[:, 1:] &= clear[:, :-1]
        eroded[:, :-1] &= clear[:, 1:]
        eroded[[0, -1], :] = False
        eroded[:, [0, -1]] = False
        if not eroded.any():
            rows, cols = np.nonzero(clear)
            return rows[0], cols[0]
        clear = eroded


class SimRobot(Node):
    def __init__(self):
        super().__init__('sim_robot')
        self.radius = self.declare_parameter('robot_radius', 0.18).value

        self.pose = None  # (x, y, yaw); set once the map arrives
        self.grid = None
        self.cmd = Twist()
        self.last_cmd = self.get_clock().now()
        self.bumped = False

        self.tf = TransformBroadcaster(self)
        self.odom_pub = self.create_publisher(Odometry, 'odom', 10)
        self.create_subscription(Twist, 'cmd_vel', self.on_cmd, 10)
        map_qos = QoSProfile(depth=1, durability=DurabilityPolicy.TRANSIENT_LOCAL)
        self.create_subscription(OccupancyGrid, 'map', self.on_map, map_qos)
        self.create_timer(1.0 / RATE_HZ, self.tick)

    def on_cmd(self, msg):
        self.cmd = msg
        self.last_cmd = self.get_clock().now()

    def on_map(self, msg):
        data = np.array(msg.data, dtype=np.int16).reshape(msg.info.height, msg.info.width)
        self.grid = data >= OCCUPIED
        self.resolution = msg.info.resolution
        self.origin = (msg.info.origin.position.x, msg.info.origin.position.y)
        self.disk = disk_offsets(self.radius / self.resolution)
        if self.pose is None:
            self.pose = (*self.start_position(data), 0.0)
            self.get_logger().info(f'start at ({self.pose[0]:.2f}, {self.pose[1]:.2f})')

    def start_position(self, data):
        start_x, start_y = os.environ.get('START_X'), os.environ.get('START_Y')
        if start_x and start_y:
            return float(start_x), float(start_y)
        row, col = most_open_cell(data == 0, data != 0)
        return (self.origin[0] + (col + 0.5) * self.resolution,
                self.origin[1] + (row + 0.5) * self.resolution)

    def collides(self, x, y):
        row = int((y - self.origin[1]) / self.resolution)
        col = int((x - self.origin[0]) / self.resolution)
        rows, cols = row + self.disk[0], col + self.disk[1]
        inside = (rows >= 0) & (rows < self.grid.shape[0]) & (cols >= 0) & (cols < self.grid.shape[1])
        return not inside.all() or self.grid[rows, cols].any()

    def tick(self):
        if self.pose is None:
            return
        now = self.get_clock().now()
        expired = (now - self.last_cmd).nanoseconds * 1e-9 > CMD_TIMEOUT_S
        v, w = (0.0, 0.0) if expired else (self.cmd.linear.x, self.cmd.angular.z)

        x, y, yaw = self.pose
        dt = 1.0 / RATE_HZ
        heading = yaw + 0.5 * w * dt
        nx, ny = x + v * dt * math.cos(heading), y + v * dt * math.sin(heading)
        if (nx, ny) != (x, y) and self.collides(nx, ny):
            if not self.bumped:
                self.get_logger().warn(f'BUMP at ({x:.2f}, {y:.2f})')
            self.bumped = True
            v = w = 0.0
        else:
            self.bumped = False
            x, y, yaw = nx, ny, yaw + w * dt
        self.pose = (x, y, yaw)
        self.publish_state(now.to_msg(), v, w)

    def publish_state(self, stamp, v, w):
        x, y, yaw = self.pose
        qz, qw = math.sin(yaw / 2.0), math.cos(yaw / 2.0)

        t = TransformStamped()
        t.header.stamp = stamp
        t.header.frame_id = 'odom'
        t.child_frame_id = 'base_footprint'
        t.transform.translation.x, t.transform.translation.y = x, y
        t.transform.rotation.z, t.transform.rotation.w = qz, qw
        self.tf.sendTransform(t)

        odom = Odometry()
        odom.header = t.header
        odom.child_frame_id = 'base_footprint'
        odom.pose.pose.position.x, odom.pose.pose.position.y = x, y
        odom.pose.pose.orientation.z, odom.pose.pose.orientation.w = qz, qw
        odom.twist.twist.linear.x, odom.twist.twist.angular.z = v, w
        self.odom_pub.publish(odom)


def main():
    rclpy.init()
    rclpy.spin(SimRobot())


if __name__ == '__main__':
    main()
