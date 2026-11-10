
"""HSRN Preliminary Multi-Robot Task Scheduler.

This is a local prototype with no ROS 2 dependencies.
Robot states and incoming tasks are currently hardcoded in demo().
The scheduler uses Euclidean distance as a placeholder for
map-based navigation distance.

MOCK DATA / FUTURE ROS 2 INTEGRATION
------------------------------------

Robot state data:
1. battery (float, 0-100)
   - Used for eligibility filtering and weighted scoring.
   - Future source: battery state topic.

2. position (x, y)
   - Used to calculate distance to the task target.
   - Future source: robot pose / odometry topic.
   - Coordinates must be in the same reference frame as the target.

3. status (idle / busy / unavailable)
   - Used to determine robot availability.
   - Future source: robot status topic or scheduler-managed state.

4. current_task (Task or None)
   - Used for priority-aware scheduling and preemption.
   - Future source: scheduler state or task management interface.

5. capabilities (set[str])
   - Used to filter robots by required hardware capabilities.
   - Future source: robot configuration or capability topic.

6. velocity (vx, vy)
   - Defined in Robot but NOT currently used by the scheduler.
   - No ROS 2 subscription required for the current algorithm.

Incoming task data:
7. task_id, task_type, priority, target,
   required_capabilities
   - Used for task ordering, eligibility, scoring, and dispatch.
   - Future source: task request interface or task queue.

Scheduler configuration (NOT ROS 2 subscriptions):
- min_battery: minimum eligible battery percentage.
- max_distance: distance normalization parameter.
- max_priority: priority normalization parameter.
- weights: battery, distance, and priority weights.
- mode: non_preemptive or priority_aware.

Current limitations:
- No ROS 2 publishers or subscribers.
- No Nav2 integration or map-based path planning.
- Priority-aware preemption is preliminary.
- Interrupted tasks require external requeuing.
- Task dispatch currently updates local Python objects only.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from math import dist
from typing import Optional


class Status(str, Enum):
    IDLE = "idle"
    BUSY = "busy"
    UNAVAILABLE = "unavailable"


@dataclass
class Task:
    task_id: str
    task_type: str
    priority: int
    target: Optional[tuple[float, float]] = None
    required_capabilities: set[str] = field(default_factory=set)
    status: str = "pending"

    def __post_init__(self):
        if self.priority < 0:
            raise ValueError("Task priority must be nonnegative")


@dataclass
class Robot:
    robot_id: str
    battery: float  # percentage, 0..100
    position: tuple[float, float]
    capabilities: set[str]
    status: Status = Status.IDLE
    current_task: Optional[Task] = None
    velocity: tuple[float, float] = (0.0, 0.0)

    def __post_init__(self):
        if not 0 <= self.battery <= 100:
            raise ValueError("Battery must be between 0 and 100")


@dataclass(frozen=True)
class Weights:
    battery: float
    distance: float
    priority: float = 0.0

    def __post_init__(self):
        if min(self.battery, self.distance, self.priority) < 0:
            raise ValueError("Weights cannot be negative")
        if abs(self.battery + self.distance + self.priority - 1) > 1e-9:
            raise ValueError("Weights must sum to 1")


@dataclass
class Decision:
    task_id: str
    selected_robot: Optional[str]
    scores: dict[str, float]
    reason: str
    interrupted_task: Optional[str] = None


class Scheduler:
    def __init__(self, *, mode: str = "non_preemptive", min_battery: float = 20,
                 max_distance: float = 20, max_priority: int = 3,
                 weights: Optional[Weights] = None):
        if mode not in ("non_preemptive", "priority_aware"):
            raise ValueError("Unknown scheduling mode")
        if not 0 <= min_battery <= 100 or max_distance <= 0 or max_priority <= 0:
            raise ValueError("Invalid normalization or battery parameters")
        self.mode = mode
        self.min_battery = min_battery
        self.max_distance = max_distance
        self.max_priority = max_priority
        self.weights = weights or (Weights(0.5, 0.5) if mode == "non_preemptive"
                                   else Weights(0.3, 0.4, 0.3))
        if mode == "non_preemptive" and self.weights.priority != 0:
            raise ValueError("Non-preemptive mode must have zero priority weight")

    def eligible(self, robot: Robot, task: Task) -> bool:
        if robot.status == Status.UNAVAILABLE or robot.battery < self.min_battery:
            return False
        if not task.required_capabilities.issubset(robot.capabilities):
            return False
        if robot.status == Status.IDLE:
            return True
        # Preliminary preemption rule: only a strictly higher-priority task
        # can interrupt a busy robot. Otherwise, it cannot be dispatched.
        return (self.mode == "priority_aware" and robot.status == Status.BUSY
                and robot.current_task is not None
                and task.priority > robot.current_task.priority)

    def score(self, robot: Robot, task: Task) -> float:
        battery_cost = 1 - robot.battery / 100
        if task.target is None:
            distance_cost = 0.0
        else:
            distance_cost = min(dist(robot.position, task.target) / self.max_distance, 1.0)
        current_priority = robot.current_task.priority if robot.current_task else 0
        priority_cost = min(current_priority / self.max_priority, 1.0)
        w = self.weights
        return (w.battery * battery_cost + w.distance * distance_cost
                + w.priority * priority_cost)

    def select(self, robots: list[Robot], task: Task) -> Decision:
        candidates = [robot for robot in robots if self.eligible(robot, task)]
        if not candidates:
            return Decision(task.task_id, None, {}, "No eligible robot; task remains pending")
        scores = {r.robot_id: round(self.score(r, task), 6) for r in candidates}
        # Stable deterministic tie-breaking by robot ID.
        chosen = min(candidates, key=lambda r: (self.score(r, task), r.robot_id))
        interrupted = chosen.current_task.task_id if chosen.status == Status.BUSY else None
        return Decision(task.task_id, chosen.robot_id, scores, "Selected lowest eligible score", interrupted)

    def dispatch(self, robots: list[Robot], task: Task) -> Decision:
        decision = self.select(robots, task)
        if decision.selected_robot is None:
            return decision
        robot = next(r for r in robots if r.robot_id == decision.selected_robot)
        if robot.current_task is not None and robot.status == Status.BUSY:
            robot.current_task.status = "pending"  # caller must requeue it
        robot.current_task = task
        robot.status = Status.BUSY
        task.status = "assigned"
        return decision

    def dispatch_queue(self, robots: list[Robot], tasks: list[Task]) -> list[Decision]:
        """Dispatch pending tasks in descending priority, stable by input order."""
        decisions = []
        for task in sorted((t for t in tasks if t.status == "pending"),
                           key=lambda t: -t.priority):
            decisions.append(self.dispatch(robots, task))
        return decisions


def demo() -> None:
    robots = [
        Robot("R1", 80, (0, 0), {"depth_camera", "temperature_humidity"}),
        Robot("R2", 60, (8, 0), {"webcam", "object_detection"}),
    ]
    scheduler = Scheduler(mode="non_preemptive", max_distance=10)
    tasks = [
        Task("T1", "temperature collection", 2, (5, 0), {"temperature_humidity"}),
        Task("T2", "object detection", 3, (9, 0), {"object_detection"}),
    ]
    for decision in scheduler.dispatch_queue(robots, tasks):
        print(f"Task {decision.task_id}: robot={decision.selected_robot}, "
              f"scores={decision.scores}, reason={decision.reason}")
    print("Final robot states:", [(r.robot_id, r.status.value, r.current_task.task_id if r.current_task else None) for r in robots])


if __name__ == "__main__":
    demo()
