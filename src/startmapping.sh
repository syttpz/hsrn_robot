#!/usr/bin/env bash
set -o pipefail

RECORD=0
for _arg in "$@"; do
  case "$_arg" in
    --record|-r) RECORD=1 ;;
    -h|--help)
      echo "Usage: $(basename "$0") [--record]"
      echo "  --record   also record a rosbag into the run folder (default: off)"
      exit 0 ;;
    *) echo "[startmapping] unknown argument: $_arg (use --record or --help)"; exit 2 ;;
  esac
done

export ROS_DOMAIN_ID=0
CAM_NS="/camera/camera"
BASE_FRAME="base_link"
MAP_TOPIC="/map"
MAP_DIR="/root/ros2_ws/maps"
MAP_NAME="my_map"

CAM_X=0.15;  CAM_Y=0.0;  CAM_Z=0.05
CAM_ROLL=0.0; CAM_PITCH=0.0; CAM_YAW=0.0

DB_SRC="$HOME/.ros/rtabmap.db"   # rtabmap's live working database

source /opt/ros/humble/setup.bash
[ -f /root/ros2_ws/install/setup.bash ] && source /root/ros2_ws/install/setup.bash
mkdir -p "$MAP_DIR"

RUN_TS="$(date +%Y%m%d_%H%M%S)"
RUN_DIR="$MAP_DIR/map_$RUN_TS"
BAG_DIR="$RUN_DIR/bag"
DB_OUT="$RUN_DIR/rtabmap.db"
MAP_OUT="$RUN_DIR/$MAP_NAME"
mkdir -p "$RUN_DIR"

CAM_PID=""; TF_PID=""; RTAB_PID=""; BAG_PID=""; SAVED=0

save_and_shutdown() {
  [ "$SAVED" = 1 ] && return; SAVED=1
  trap '' INT TERM    # absorb repeated Ctrl-C so the save can't be interrupted
  echo
  echo "==================================================================="
  echo "[startmapping] STOPPING — saving map + database. Do NOT press Ctrl-C."
  echo "==================================================================="

  if [ -n "$BAG_PID" ]; then
    kill -INT -"$BAG_PID" 2>/dev/null
    for _ in $(seq 1 5); do kill -0 "$BAG_PID" 2>/dev/null || break; sleep 1; done
    kill -KILL -"$BAG_PID" 2>/dev/null
    echo "[startmapping]   saved rosbag -> $BAG_DIR"
  fi

  if [ -n "$RTAB_PID" ] && kill -0 "$RTAB_PID" 2>/dev/null; then
    echo "[startmapping]   saving 2D grid from $MAP_TOPIC ..."
    timeout 20 ros2 run nav2_map_server map_saver_cli \
        -t "$MAP_TOPIC" -f "$MAP_OUT" \
        --ros-args -p save_map_timeout:=8.0 </dev/null \
      || echo "[startmapping]   WARN: 2D map save failed/timed out"
  fi

  echo "[startmapping]   stopping rtabmap (flushing database)..."
  if [ -n "$RTAB_PID" ]; then
    kill -INT -"$RTAB_PID" 2>/dev/null
    for _ in $(seq 1 12); do kill -0 "$RTAB_PID" 2>/dev/null || break; sleep 1; done
    kill -KILL -"$RTAB_PID" 2>/dev/null
  fi

  if [ -f "$DB_SRC" ]; then
    cp -f "$DB_SRC" "$DB_OUT"
    echo "[startmapping]   saved database -> $DB_OUT"
  else
    echo "[startmapping]   WARN: $DB_SRC not found — database not saved"
  fi

  for _p in "$CAM_PID" "$TF_PID"; do [ -n "$_p" ] && kill -INT -"$_p" 2>/dev/null; done
  sleep 2
  for _p in "$CAM_PID" "$TF_PID"; do [ -n "$_p" ] && kill -KILL -"$_p" 2>/dev/null; done

  echo "[startmapping] done. Outputs in $RUN_DIR (host: ./maps/)"
  exit 0
}
trap save_and_shutdown INT TERM

echo "[startmapping] 1/3 launching RealSense with aligned depth..."
setsid ros2 launch realsense2_camera rs_launch.py \
    depth_module.depth_profile:=640x480x15 \
    rgb_camera.color_profile:=640x480x15 \
    align_depth.enable:=true \
    enable_gyro:=false enable_accel:=false \
    "camera.camera.aligned_depth_to_color.image_raw.disable_pub_plugins:=[image_transport/compressed]" &
CAM_PID=$!

echo "[startmapping] 2/3 static TF ${BASE_FRAME} -> camera_link (edit CAM_* above)..."
setsid ros2 run tf2_ros static_transform_publisher \
    --x "$CAM_X" --y "$CAM_Y" --z "$CAM_Z" \
    --roll "$CAM_ROLL" --pitch "$CAM_PITCH" --yaw "$CAM_YAW" \
    --frame-id "$BASE_FRAME" --child-frame-id camera_link &
TF_PID=$!

echo "[startmapping] waiting for camera to come up..."
sleep 6

if [ "$RECORD" = 1 ]; then
  echo "[startmapping] --record: recording source rosbag -> $BAG_DIR"
  setsid ros2 bag record -o "$BAG_DIR" \
      "$CAM_NS/color/image_raw" \
      "$CAM_NS/aligned_depth_to_color/image_raw" \
      "$CAM_NS/color/camera_info" \
      "$CAM_NS/aligned_depth_to_color/camera_info" \
      /tf \
      /tf_static \
      /odom \
      /imu \
      /cmd_vel \
      /wheel_ticks \
      /wheel_vels \
      /wheel_status \
      /battery_state \
      /kidnap_status \
      /slip_status &
  BAG_PID=$!
else
  echo "[startmapping] rosbag recording OFF (pass --record to enable)"
fi


echo "[startmapping] 3/3 launching rtabmap (mapping mode, fresh database)..."
setsid ros2 launch rtabmap_launch rtabmap.launch.py \
    rtabmap_args:="--delete_db_on_start" \
    frame_id:="$BASE_FRAME" \
    visual_odometry:=false \
    odom_topic:="/odom" \
    subscribe_depth:=true \
    rgb_topic:="$CAM_NS/color/image_raw" \
    depth_topic:="$CAM_NS/aligned_depth_to_color/image_raw" \
    camera_info_topic:="$CAM_NS/color/camera_info" \
    approx_sync:=true \
    topic_queue_size:=30 \
    sync_queue_size:=30 \
    qos:=2 \
    rviz:=false \
    rtabmap_viz:=false \
    use_sim_time:=false &
RTAB_PID=$!

echo "[startmapping] mapping... drive the robot around slowly. Ctrl-C to save & stop."
wait "$RTAB_PID"          # if rtabmap dies on its own, still save what we have
save_and_shutdown
