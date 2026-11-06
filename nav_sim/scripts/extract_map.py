#!/usr/bin/env python3
"""Turn an RTAB-Map database into a floor-relative Nav2 map (<out>.pgm + <out>.yaml).

RTAB-Map's own 2D grid flattens every 3D cell it stored, so shelves, ceilings and
tilted floor all become walls. This script rebuilds the grid instead:

  1. RTAB-Map, running on a temporary copy of the database, optimizes the pose graph.
  2. Each node's stored 3D cells are measured against that node's own fitted floor
     plane, which makes heights immune to z drift in the map.
  3. Cells near the floor are free, cells in the obstacle band are occupied, and
     anything higher (ceilings, overhead cable trays) is ignored.
  4. A lane around the robot's recorded path is cleared: it physically drove there,
     so anything marked as a wall in that lane is depth noise.
"""

import argparse
import os
import shutil
import signal
import sqlite3
import struct
import subprocess
import sys
import tempfile
import time
import zlib

import numpy as np
import rclpy
import yaml
from rtabmap_msgs.msg import MapGraph
from rtabmap_msgs.srv import PublishMap

RESOLUTION = 0.05          # m per map cell
FLOOR_BAND = 0.03          # m; |height| up to this counts as floor
MAX_OBSTACLE_HEIGHT = 0.40 # m; higher points are ignored
MAX_RANGE = 4.0            # m from the robot; depth is too noisy for walls beyond this
MIN_OBSTACLE_NODES = 3     # a cell must be seen as an obstacle from this many nodes...
MIN_OBSTACLE_RATIO = 0.3   # ...and in at least this fraction of the nodes that saw it
DRIVEN_CLEARANCE = 0.25    # m around the recorded path that is always free
MAP_MARGIN = 0.5           # m of unknown space around the map

TIMEOUT_S = 180.0
RTABMAP_PARAMS = """\
rtabmap:
  ros__parameters:
    database_path: "{db}"
    frame_id: base_link
    subscribe_depth: false
    subscribe_rgb: false
    subscribe_rgbd: false
    subscribe_scan: false
    subscribe_odom_info: false
    publish_tf: false
    Mem/IncrementalMemory: "false"
    Mem/InitWMWithAllNodes: "true"
"""


def request_poses():
    """Ask the running RTAB-Map node for its optimized graph: {node id: (R, t)}."""
    rclpy.init()
    node = rclpy.create_node('extract_map')
    graphs = []
    node.create_subscription(MapGraph, 'mapGraph', graphs.append, 1)
    client = node.create_client(PublishMap, 'rtabmap/publish_map')
    try:
        if not client.wait_for_service(timeout_sec=TIMEOUT_S):
            raise RuntimeError('RTAB-Map did not start')
        client.call_async(PublishMap.Request(global_map=True, optimized=True, graph_only=True))
        deadline = time.time() + TIMEOUT_S
        while not graphs and time.time() < deadline:
            rclpy.spin_once(node, timeout_sec=0.5)
        if not graphs:
            raise RuntimeError('RTAB-Map published no graph')
        return {i: pose_to_transform(p) for i, p in zip(graphs[0].poses_id, graphs[0].poses)}
    finally:
        node.destroy_node()
        rclpy.shutdown()


def pose_to_transform(pose):
    x, y, z, w = (pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w)
    rotation = np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
        [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
        [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)],
    ])
    return rotation, np.array([pose.position.x, pose.position.y, pose.position.z])


def decode_cells(blob):
    """Decode an RTAB-Map cell blob (zlib data + rows, cols, type) to (N, 3) x, y, z points."""
    if not blob:
        return np.empty((0, 3))
    channels = (struct.unpack('<i', blob[-4:])[0] >> 3) + 1
    cells = np.frombuffer(zlib.decompress(blob[:-12]), np.float32).reshape(-1, channels)[:, :3]
    return cells[np.isfinite(cells).all(axis=1)].astype(np.float64)


def fit_floor(ground):
    """Fit z = a*x + b*y + c to floor points, dropping tables and other outliers. None if too few."""
    inliers = ground
    if len(inliers) < 3:
        return None
    plane = np.array([0.0, 0.0, np.median(ground[:, 2])])
    for tolerance in (0.10, 0.05, 0.03):
        inliers = ground[np.abs(height_above(ground, plane)) < tolerance]
        if len(inliers) < 50:
            return None
        design = np.column_stack([inliers[:, 0], inliers[:, 1], np.ones(len(inliers))])
        plane = np.linalg.lstsq(design, inliers[:, 2], rcond=None)[0]
    return plane


def height_above(points, plane):
    return points[:, 2] - (plane[0] * points[:, 0] + plane[1] * points[:, 1] + plane[2])


def to_cells(points, transform):
    """Project node-frame points to unique (x, y) map cell indices."""
    rotation, translation = transform
    world = points @ rotation.T + translation
    return np.unique(np.floor(world[:, :2] / RESOLUTION).astype(np.int64), axis=0)


def driven_lane(poses, low, shape):
    """Mask of the cells within DRIVEN_CLEARANCE of the path through the poses, in node order."""
    radius = int(DRIVEN_CLEARANCE / RESOLUTION)
    rows, cols = np.ogrid[-radius:radius + 1, -radius:radius + 1]
    disk = rows ** 2 + cols ** 2 <= radius ** 2

    path = np.array([poses[i][1][:2] for i in sorted(poses)]) / RESOLUTION - low
    lane = np.zeros(shape, dtype=bool)
    for start, end in zip(path, path[1:]):
        steps = int(np.abs(end - start).max()) + 1
        for col, row in np.linspace(start, end, steps).astype(np.int64):
            lane[row - radius:row + radius + 1, col - radius:col + radius + 1] |= disk
    return lane


def build_grid(db_path, poses):
    """Return (occupied, free, origin_xy); rows run from the bottom of the map upward."""
    db = sqlite3.connect(db_path)
    rows = db.execute('SELECT id, ground_cells, obstacle_cells FROM Data')
    nodes = [(i, decode_cells(g), decode_cells(o)) for i, g, o in rows if i in poses]
    planes = {i: fit_floor(ground) for i, ground, _ in nodes}
    fitted = [p for p in planes.values() if p is not None]
    if not fitted:
        raise RuntimeError('could not find the floor in any node')
    typical_plane = np.median(fitted, axis=0)
    print(f'[extract_map] {len(nodes)} nodes, floor fitted in {len(fitted)}', flush=True)

    floor_hits, obstacle_hits = [], []
    for i, ground, obstacles in nodes:
        points = np.vstack([ground, obstacles])
        points = points[np.hypot(points[:, 0], points[:, 1]) < MAX_RANGE]
        plane = planes[i] if planes[i] is not None else typical_plane
        height = height_above(points, plane)
        floor_hits.append(to_cells(points[np.abs(height) <= FLOOR_BAND], poses[i]))
        band = (height > FLOOR_BAND) & (height <= MAX_OBSTACLE_HEIGHT)
        obstacle_hits.append(to_cells(points[band], poses[i]))
    floor_hits, obstacle_hits = np.vstack(floor_hits), np.vstack(obstacle_hits)

    margin = int(MAP_MARGIN / RESOLUTION)
    every_hit = np.vstack([floor_hits, obstacle_hits])
    low = every_hit.min(axis=0) - margin
    width, height = every_hit.max(axis=0) - low + margin + 1
    floor_count, obstacle_count = np.zeros((2, height, width), dtype=np.int32)
    np.add.at(floor_count, (floor_hits[:, 1] - low[1], floor_hits[:, 0] - low[0]), 1)
    np.add.at(obstacle_count, (obstacle_hits[:, 1] - low[1], obstacle_hits[:, 0] - low[0]), 1)

    occupied = (obstacle_count >= MIN_OBSTACLE_NODES) & \
        (obstacle_count >= MIN_OBSTACLE_RATIO * (obstacle_count + floor_count))
    lane = driven_lane(poses, low, occupied.shape)
    return occupied & ~lane, ((floor_count > 0) & ~occupied) | lane, low * RESOLUTION


def save_map(occupied, free, origin, out):
    """Write map_saver's trinary format: free 254, occupied 0, unknown 205."""
    image = np.full(occupied.shape, 205, dtype=np.uint8)
    image[free] = 254
    image[occupied] = 0
    with open(f'{out}.pgm', 'wb') as f:
        f.write(b'P5\n%d %d\n255\n' % (image.shape[1], image.shape[0]))
        f.write(np.flipud(image).tobytes())  # image row 0 is the top of the map
    with open(f'{out}.yaml', 'w') as f:
        yaml.safe_dump({
            'image': f'{os.path.basename(out)}.pgm',
            'mode': 'trinary',
            'resolution': RESOLUTION,
            'origin': [float(origin[0]), float(origin[1]), 0.0],
            'negate': 0,
            'occupied_thresh': 0.65,
            'free_thresh': 0.196,  # must stay below unknown grey: (255 - 205) / 255 = 0.19608
        }, f, default_flow_style=None, sort_keys=False)
    print(f'[extract_map] wrote {out}.pgm/.yaml: {image.shape[1]}x{image.shape[0]} cells, '
          f'{occupied.sum()} occupied, {free.sum()} free', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--db', required=True, help='RTAB-Map database')
    parser.add_argument('--out', required=True, help='output path without extension')
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix='extract_map_', ignore_cleanup_errors=True) as workdir:
        db_copy = shutil.copy(args.db, workdir)
        params = os.path.join(workdir, 'params.yaml')
        with open(params, 'w') as f:
            f.write(RTABMAP_PARAMS.format(db=db_copy))

        print(f'[extract_map] reading {args.db} with RTAB-Map...', flush=True)
        log_path = os.path.join(workdir, 'rtabmap.log')
        with open(log_path, 'w') as log:
            rtabmap = subprocess.Popen(
                ['ros2', 'run', 'rtabmap_slam', 'rtabmap', '--ros-args', '--params-file', params],
                stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
        try:
            poses = request_poses()
        except RuntimeError as error:
            with open(log_path) as log:
                sys.exit(f'[extract_map] {error}. RTAB-Map log:\n{log.read()[-3000:]}')
        finally:
            os.killpg(rtabmap.pid, signal.SIGKILL)
            rtabmap.wait()

        save_map(*build_grid(db_copy, poses), args.out)


if __name__ == '__main__':
    main()
