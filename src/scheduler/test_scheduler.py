import unittest
from scheduler import Robot, Task, Scheduler, Status, Weights


class SchedulerTests(unittest.TestCase):
    def test_non_preemptive_ignores_busy(self):
        busy_task = Task("old", "inspection", 2)
        r1 = Robot("R1", 90, (0, 0), set(), Status.BUSY, busy_task)
        r2 = Robot("R2", 60, (5, 0), set())
        self.assertEqual(Scheduler().select([r1, r2], Task("new", "inspection", 3, (0, 0))).selected_robot, "R2")

    def test_capability_filter(self):
        r1 = Robot("R1", 90, (0, 0), {"temperature_humidity"})
        r2 = Robot("R2", 90, (0, 0), {"webcam"})
        task = Task("t", "sensor", 2, (0, 0), {"temperature_humidity"})
        self.assertEqual(Scheduler().select([r1, r2], task).selected_robot, "R1")

    def test_low_battery_pending(self):
        r = Robot("R1", 5, (0, 0), set())
        t = Task("t", "inspection", 2)
        d = Scheduler().dispatch([r], t)
        self.assertIsNone(d.selected_robot)
        self.assertEqual(t.status, "pending")

    def test_priority_aware_preemption(self):
        old = Task("old", "inspection", 2, status="assigned")
        r = Robot("R1", 90, (0, 0), set(), Status.BUSY, old)
        scheduler = Scheduler(mode="priority_aware")
        self.assertIsNone(scheduler.select([r], Task("low", "inspection", 2)).selected_robot)
        new = Task("new", "check server", 3)
        d = scheduler.dispatch([r], new)
        self.assertEqual(d.interrupted_task, "old")
        self.assertEqual(old.status, "pending")
        self.assertEqual(new.status, "assigned")

    def test_priority_weight_example(self):
        old = Task("old", "inspection", 2, status="assigned")
        r1 = Robot("R1", 80, (0, 0), set())
        r2 = Robot("R2", 60, (4, 0), set(), Status.BUSY, old)
        task = Task("new", "check server", 3, (6, 0))
        d = Scheduler(mode="priority_aware", max_distance=10).select([r1, r2], task)
        self.assertAlmostEqual(d.scores["R1"], 0.30)
        self.assertAlmostEqual(d.scores["R2"], 0.40)
        self.assertEqual(d.selected_robot, "R1")

    def test_priority_order(self):
        r1 = Robot("R1", 90, (0, 0), set())
        r2 = Robot("R2", 90, (0, 0), set())
        tasks = [Task("low", "inspection", 2), Task("high", "server", 3)]
        decisions = Scheduler().dispatch_queue([r1, r2], tasks)
        self.assertEqual([d.task_id for d in decisions], ["high", "low"])

    def test_tie_breaker(self):
        robots = [Robot("R2", 80, (0, 0), set()), Robot("R1", 80, (0, 0), set())]
        self.assertEqual(Scheduler().select(robots, Task("t", "inspection", 2)).selected_robot, "R1")


if __name__ == "__main__":
    unittest.main()
