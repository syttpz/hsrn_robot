
"""ROS 2 scheduler node with Action Client support.

Receives robot battery and odometry data, schedules pending tasks,
sends target coordinates to the robot's Action Server, and
confirms whether each goal was accepted.

The robot-side Action Server is responsible for executing the task.
"""

import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient

from sensor_msgs.msg import BatteryState
from nav_msgs.msg import Odometry
from hsrn_interfaces.action import ExecuteTask

from scheduler import Robot, Task, Scheduler, Status


class SchedulerNode(Node):

    def __init__(self):
        super().__init__("scheduler_node")

        # One robot for initial testing
        self.robot = Robot(
            robot_id="R1",
            battery=100.0,
            position=(0.0, 0.0),
            capabilities={"depth_camera", "temperature_humidity"},
            status=Status.IDLE
        )

        self.scheduler = Scheduler(
            mode="non_preemptive",
            max_distance=10
        )

        # Pending tasks
        self.task_queue = []

        # Latest robot data
        self.latest_battery_msg = None
        self.latest_odom_msg = None
        self.battery_received = False
        self.odom_received = False

        # Task currently waiting for an Action Server response
        self.pending_goal_task = None

        # ROS 2 subscriptions
        self.battery_sub = self.create_subscription(
            BatteryState,
            "/battery_state",
            self.battery_callback,
            10
        )

        self.odom_sub = self.create_subscription(
            Odometry,
            "/odom",
            self.odom_callback,
            10
        )

        # Action Client for robot R1
        self.action_client = ActionClient(
            self,
            ExecuteTask,
            "/robot/r1/execute_task"
        )

        self.get_logger().info("Scheduler node started")

        # Initial test task
        test_task = Task(
            task_id="T1",
            task_type="temperature collection",
            priority=2,
            target=(5.0, 0.0),
            required_capabilities={"temperature_humidity"}
        )

        self.receive_task(test_task)

    # --------------------------------------------------
    # Robot state callbacks
    # --------------------------------------------------

    def battery_callback(self, msg):
        self.latest_battery_msg = msg

        if not 0.0 <= msg.percentage <= 1.0:
            self.get_logger().warning(
                "Invalid battery percentage received"
            )
            return

        self.robot.battery = msg.percentage * 100.0
        self.battery_received = True

        self.get_logger().info(
            f"Battery updated: {self.robot.battery:.1f}%"
        )

        self.try_dispatch()

    def odom_callback(self, msg):
        self.latest_odom_msg = msg

        position = msg.pose.pose.position
        velocity = msg.twist.twist.linear

        self.robot.position = (
            position.x,
            position.y
        )

        self.robot.velocity = (
            velocity.x,
            velocity.y
        )

        self.odom_received = True

        self.get_logger().info(
            f"Position updated: {self.robot.position}"
        )

        self.try_dispatch()

    # --------------------------------------------------
    # Task reception
    # --------------------------------------------------

    def receive_task(self, task):
        """Add a new task to the pending queue."""

        self.task_queue.append(task)

        self.get_logger().info(
            f"Task received: {task.task_id}, "
            f"target={task.target}"
        )

        self.try_dispatch()

    # --------------------------------------------------
    # Scheduling
    # --------------------------------------------------

    def try_dispatch(self):
        """Select a robot and initiate task delivery."""

        # Wait until both robot data sources are available
        if not self.battery_received or not self.odom_received:
            return

        # Do not send another goal while waiting for a response
        if self.pending_goal_task is not None:
            return

        if not self.task_queue:
            return

        # Higher-priority tasks first
        pending_tasks = sorted(
            (
                task for task in self.task_queue
                if task.status == "pending"
            ),
            key=lambda task: -task.priority
        )

        for task in pending_tasks:
            decision = self.scheduler.select(
                [self.robot],
                task
            )

            if decision.selected_robot is None:
                self.get_logger().info(
                    f"Task {task.task_id} remains pending: "
                    f"{decision.reason}"
                )
                continue

            if task.target is None:
                self.get_logger().warning(
                    f"Task {task.task_id} has no target"
                )
                continue

            self.get_logger().info(
                f"Selected {decision.selected_robot} "
                f"for task {task.task_id}"
            )

            self.send_task(task)
            break

    # --------------------------------------------------
    # Action Client
    # --------------------------------------------------

    def send_task(self, task):
        """Send target coordinates to the robot Action Server."""

        if self.pending_goal_task is not None:
            return

        if not self.action_client.server_is_ready():
            self.get_logger().warning(
                "Action Server is not available yet"
            )
            return

        goal = ExecuteTask.Goal()
        goal.target_x = float(task.target[0])
        goal.target_y = float(task.target[1])

        self.pending_goal_task = task

        self.get_logger().info(
            f"Sending task {task.task_id} to R1: "
            f"target=({goal.target_x}, {goal.target_y})"
        )

        try:
            future = self.action_client.send_goal_async(goal)
            future.add_done_callback(
                self.goal_response_callback
            )
        except Exception as exc:
            self.pending_goal_task = None
            self.get_logger().error(
                f"Failed to send task {task.task_id}: {exc}"
            )

    def goal_response_callback(self, future):
        """Handle Accepted / Rejected response."""

        task = self.pending_goal_task

        if task is None:
            return

        try:
            goal_handle = future.result()
        except Exception as exc:
            self.pending_goal_task = None
            self.get_logger().error(
                f"Goal response failed for {task.task_id}: {exc}"
            )
            return

        self.pending_goal_task = None

        if not goal_handle.accepted:
            self.get_logger().warning(
                f"Task {task.task_id} REJECTED by R1"
            )

            # Keep task pending for future scheduling
            return

        # Goal accepted by the robot's Action Server
        task.status = "assigned"
        self.robot.current_task = task
        self.robot.status = Status.BUSY

        if task in self.task_queue:
            self.task_queue.remove(task)

        self.get_logger().info(
            f"Task {task.task_id} ACCEPTED by R1"
        )

        # This confirms acceptance, not task completion.
        # Execution and result handling belong to the robot side.

    # --------------------------------------------------
    # Main
    # --------------------------------------------------


def main(args=None):
    rclpy.init(args=args)

    node = SchedulerNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
