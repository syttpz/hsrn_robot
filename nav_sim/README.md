# nav_sim

Nav2 simulation for the data center robot: **A\*** (NavFn) plans a path and **Regulated Pure Pursuit** follows it, on a map extracted from an RTAB-Map database. Runs in Docker and shows RViz in your browser.

## Run

Start Docker Desktop, then:

```bash
cd nav_sim
DB=370J_7th.db docker compose up --build   # or DB=rtabmap.db
```

The database must be in `../maps/`. Open <http://localhost:6080/vnc.html?autoconnect=1&resize=remote>, click **2D Goal Pose** in the RViz toolbar, then click and drag on the map to set a goal and heading.

The robot starts at the most open spot on the map. To choose the start yourself, set `START_X` and `START_Y` in metres. If port 6080 is taken, set `PORT` and use that port in the URL.

## The map

The first run on a database writes `../maps/<name>.pgm` and `.yaml` next to it. The map is rebuilt only if it is missing or older than the database; the database itself is never changed.

`scripts/extract_map.py` uses RTAB-Map's optimized poses, but not its 2D grid, which turns shelves, ceilings and tilted floor into walls. Instead, each node's stored 3D cells are measured against that node's own fitted floor plane:

| Height above floor | Result |
|---|---|
| Up to 0.03 m | Free |
| 0.03 to 0.40 m | Occupied |
| Above 0.40 m | Ignored |

A 0.25 m lane around the robot's recorded path is then cleared: it physically drove there, so anything marked as a wall in that lane is depth noise.

The thresholds are constants at the top of the script. Delete the map files after changing them so the map gets rebuilt.

## Tuning

Planner, controller and costmap settings are in `config/nav2_params.yaml`:

- **Speed and lookahead:** `desired_linear_vel`, `lookahead_dist`
- **Robot size and wall padding:** `robot_radius`, `inflation_radius`
- **A\*:** `use_astar`, and `allow_unknown` (off, so paths stay on observed floor)

After changing them, run `docker compose up --build` again.

## What this does not simulate

There are no sensors and no localization: the robot knows its exact position and sees only the static map. It won't react to obstacles missing from the map, and the arm isn't modelled, only a 0.18 m radius circle. The real robot needs a depth obstacle layer in the costmaps and a localization source.

## Files

```
Dockerfile, docker-compose.yml
entrypoint.sh            database -> map -> browser desktop -> launch
config/nav2_params.yaml  A* + Regulated Pure Pursuit
config/nav2.rviz         RViz layout
launch/sim.launch.py     sim robot, map server, Nav2, RViz
scripts/extract_map.py   database -> floor-relative map
scripts/sim_robot.py     simulated differential-drive robot
```
