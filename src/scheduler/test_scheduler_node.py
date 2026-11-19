
"""Unit tests for the ROS 2 scheduler node.

Mocks ROS 2 modules so tests can run without ROS 2 installed.
Tests robot state updates, task scheduling, and Action Client behavior.
"""

import sys
import unittest
from types import ModuleType, SimpleNamespace
from unittest.mock import MagicMock, patch


# --------------------------------------------------
# Mock ROS 2 modules before importing scheduler_node
# --------------------------------------------------

rclpy = ModuleType("rclpy")
rclpy_node = ModuleType("rclpy.node")
rclpy_action = ModuleType("rclpy.action")

sensor_msgs = ModuleType("sensor_msgs")
sensor_msgs_msg = ModuleType("sensor_msgs.msg")

nav_msgs = ModuleType("nav_msgs")
nav_msgs_msg = ModuleType("nav_msgs.msg")

hsrn_interfaces = ModuleType("hsrn_interfaces")
hsrn_interfaces_action = ModuleType("hsrn_interfaces.action")


class MockNode:
    def get_logger(self):
        return MagicMock()


class MockExecuteTask:
    class Goal:
        def __init__(self):
            self.target_x = 0.0
            self.target_y = 0.0


rclpy_node.Node = MockNode
rclpy_action.ActionClient = MagicMock

sensor_msgs_msg.BatteryState = type("BatteryState", (), {})
nav_msgs_msg.Odometry = type("Odometry", (), {})
hsrn_interfaces_action.ExecuteTask = MockExecuteTask

mock_modules = {
    "rclpy": rclpy,
    "rclpy.node": rclpy_node,
    "rclpy.action": rclpy_action,
    "sensor_msgs": sensor_msgs,
    "sensor_msgs.msg": sensor_msgs_msg,
    "nav_msgs": nav_msgs,
    "nav_msgs.msg": nav_msgs_msg,
    "hsrn_interfaces": hsrn_interfaces,
    "hsrn_interfaces.action": hsrn_interfaces_action,
}

with patch.dict(sys.modules, mock_modules):
    from scheduler_node import SchedulerNode

from scheduler import Robot, Task, Scheduler, Status


class TestSchedulerNode(unittest.TestCase):

    def setUp(self):
        # Create node without starting real ROS 2 subscriptions
        self.node = SchedulerNode.__new__(SchedulerNode)

        self.node.robot = Robot(
            robot_id="R1",
            battery=100.0,
            position=(0.0, 0.0),
            capabilities={"depth_camera", "temperature_humidity"},
            status=Status.IDLE
        )

        self.node.scheduler = Scheduler(
            mode="non_preemptive",
            max_distance=10
        )

        self.node.task_queue = []

        self.node.latest_battery_msg = None
        self.node.latest_odom_msg = None

        self.node.battery_received = False
        self.node.odom_received = False

        self.node.pending_goal_task = None

        # Mock Action Client
        self.node.action_client = MagicMock()
        self.node.action_client.server_is_ready.return_value = True

        # Mock logger
        self.logger = MagicMock()
        self.node.get_logger = MagicMock(return_value=self.logger)

    # --------------------------------------------------
    # Test helpers
    # --------------------------------------------------

    def create_task(self, task_id="T1"):
        return Task(
            task_id=task_id,
            task_type="temperature collection",
            priority=2,
            target=(5.0, 0.0),
            required_capabilities={"temperature_humidity"}
        )

    def create_odom_message(self):
        return SimpleNamespace(
            pose=SimpleNamespace(
                pose=SimpleNamespace(
                    position=SimpleNamespace(x=2.5, y=3.0)
                )
            ),
            twist=SimpleNamespace(
                twist=SimpleNamespace(
                    linear=SimpleNamespace(x=0.5, y=0.2)
                )
            )
        )

    def make_robot_data_ready(self):
        self.node.battery_received = True
        self.node.odom_received = True

    # --------------------------------------------------
    # Battery tests
    # --------------------------------------------------

    def test_battery_message_storage(self):
        msg = SimpleNamespace(percentage=0.85)

        self.node.battery_callback(msg)

        self.assertIs(self.node.latest_battery_msg, msg)
        self.assertAlmostEqual(self.node.robot.battery, 85.0)
        self.assertTrue(self.node.battery_received)

    def test_battery_message_update(self):
        first_msg = SimpleNamespace(percentage=0.90)
        second_msg = SimpleNamespace(percentage=0.60)

        self.node.battery_callback(first_msg)
        self.node.battery_callback(second_msg)

        self.assertIs(self.node.latest_battery_msg, second_msg)
        self.assertAlmostEqual(self.node.robot.battery, 60.0)

    def test_invalid_battery_percentage(self):
        msg = SimpleNamespace(percentage=-1.0)

        self.node.battery_callback(msg)

        self.assertIs(self.node.latest_battery_msg, msg)
        self.assertAlmostEqual(self.node.robot.battery, 100.0)
        self.assertFalse(self.node.battery_received)
        self.logger.warning.assert_called_once()

    # --------------------------------------------------
    # Odometry tests
    # --------------------------------------------------

    def test_odometry_message_storage(self):
        msg = self.create_odom_message()

        self.node.odom_callback(msg)

        self.assertIs(self.node.latest_odom_msg, msg)
        self.assertEqual(self.node.robot.position, (2.5, 3.0))
        self.assertEqual(self.node.robot.velocity, (0.5, 0.2))
        self.assertTrue(self.node.odom_received)

    # --------------------------------------------------
    # Task queue tests
    # --------------------------------------------------

    def test_receive_task(self):
        task = self.create_task()

        self.node.receive_task(task)

        self.assertIn(task, self.node.task_queue)
        self.assertEqual(task.status, "pending")

    def test_wait_for_robot_data(self):
        task = self.create_task()
        self.node.receive_task(task)

        # Only battery data is available
        self.node.battery_callback(
            SimpleNamespace(percentage=0.80)
        )

        self.assertIn(task, self.node.task_queue)
        self.assertEqual(task.status, "pending")
        self.assertEqual(self.node.robot.status, Status.IDLE)

        self.node.action_client.send_goal_async.assert_not_called()

    # --------------------------------------------------
    # Action Client tests
    # --------------------------------------------------

    def test_send_target_coordinates(self):
        task = self.create_task()
        self.make_robot_data_ready()

        self.node.receive_task(task)

        self.node.action_client.send_goal_async.assert_called_once()

        # Inspect the goal sent to the Action Server
        args, kwargs = (
            self.node.action_client.send_goal_async.call_args
        )

        goal = args[0]

        self.assertAlmostEqual(goal.target_x, 5.0)
        self.assertAlmostEqual(goal.target_y, 0.0)

        # Not assigned until the Action Server accepts
        self.assertEqual(task.status, "pending")
        self.assertIs(self.node.pending_goal_task, task)

    def test_goal_accepted(self):
        task = self.create_task()
        self.make_robot_data_ready()

        self.node.receive_task(task)

        # Simulate Action Server accepting the goal
        goal_handle = SimpleNamespace(accepted=True)

        future = MagicMock()
        future.result.return_value = goal_handle

        self.node.goal_response_callback(future)

        self.assertEqual(task.status, "assigned")
        self.assertEqual(self.node.robot.status, Status.BUSY)
        self.assertIs(self.node.robot.current_task, task)
        self.assertNotIn(task, self.node.task_queue)
        self.assertIsNone(self.node.pending_goal_task)

    def test_goal_rejected(self):
        task = self.create_task()
        self.make_robot_data_ready()

        self.node.receive_task(task)

        # Simulate Action Server rejecting the goal
        goal_handle = SimpleNamespace(accepted=False)

        future = MagicMock()
        future.result.return_value = goal_handle

        self.node.goal_response_callback(future)

        self.assertEqual(task.status, "pending")
        self.assertIn(task, self.node.task_queue)
        self.assertEqual(self.node.robot.status, Status.IDLE)
        self.assertIsNone(self.node.robot.current_task)
        self.assertIsNone(self.node.pending_goal_task)

    def test_action_server_unavailable(self):
        self.node.action_client.server_is_ready.return_value = False

        task = self.create_task()
        self.make_robot_data_ready()

        self.node.receive_task(task)

        self.node.action_client.send_goal_async.assert_not_called()

        self.assertEqual(task.status, "pending")
        self.assertIn(task, self.node.task_queue)
        self.assertIsNone(self.node.pending_goal_task)

    def test_no_duplicate_goal_while_waiting(self):
        task = self.create_task()
        self.make_robot_data_ready()

        self.node.receive_task(task)

        # First goal has been sent
        self.node.action_client.send_goal_async.assert_called_once()

        # Simulate additional robot state updates
        self.node.battery_callback(
            SimpleNamespace(percentage=0.75)
        )

        self.node.odom_callback(
            self.create_odom_message()
        )

        # Should not send the same goal again
        self.node.action_client.send_goal_async.assert_called_once()

        self.assertIs(self.node.pending_goal_task, task)

    # --------------------------------------------------
    # Scheduler integration tests
    # --------------------------------------------------

    def test_low_battery_task_remains_pending(self):
        task = self.create_task()

        self.node.receive_task(task)

        self.node.battery_callback(
            SimpleNamespace(percentage=0.10)
        )

        self.node.odom_callback(
            self.create_odom_message()
        )

        self.assertEqual(task.status, "pending")
        self.assertIn(task, self.node.task_queue)
        self.assertEqual(self.node.robot.status, Status.IDLE)

        self.node.action_client.send_goal_async.assert_not_called()

    def test_no_duplicate_dispatch_after_acceptance(self):
        task = self.create_task()
        self.make_robot_data_ready()

        self.node.receive_task(task)

        future = MagicMock()
        future.result.return_value = SimpleNamespace(
            accepted=True
        )

        self.node.goal_response_callback(future)

        # New robot messages arrive after acceptance
        self.node.battery_callback(
            SimpleNamespace(percentage=0.75)
        )

        self.node.odom_callback(
            self.create_odom_message()
        )

        self.node.action_client.send_goal_async.assert_called_once()
        self.assertEqual(task.status, "assigned")
        self.assertEqual(self.node.robot.status, Status.BUSY)
        self.assertEqual(len(self.node.task_queue), 0)


if __name__ == "__main__":
    unittest.main()
