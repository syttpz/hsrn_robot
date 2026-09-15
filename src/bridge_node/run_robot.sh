#!/usr/bin/env bash
set -eo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ "${1:-}" != "--no-build" && "${SKIP_BUILD:-0}" != "1" ]]; then
  "$HERE/build.sh"
fi

source /opt/ros/humble/setup.bash
set -u

TOPIC="${TOPIC:-/camera/camera/color/image_raw/compressed}"
TYPE="${TYPE:-sensor_msgs/msg/CompressedImage}"
PROTO="${PROTO:-udp}"          
REL="${REL:-best_effort}"      # camera best effort qos

echo "[robot] $TOPIC ($TYPE) --$PROTO--> corelink  [qos.reliability=$REL]"
exec "$HERE/build/ros2_bridge_node" --ros-args \
    --params-file "$HERE/config/robot_side.yaml" \
    --params-file "$HERE/config/credentials.yaml" \
    -p topic.name:="$TOPIC" \
    -p topic.type:="$TYPE" \
    -p topic.direction:=to_corelink \
    -p qos.reliability:="$REL" \
    -p corelink.data_protocol:="$PROTO"
