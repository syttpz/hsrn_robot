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
REL="${REL:-best_effort}"

echo "[server] corelink --$PROTO--> republish $TOPIC ($TYPE)  [qos.reliability=$REL]"
exec "$HERE/build/ros2_bridge_node" --ros-args \
    --params-file "$HERE/config/server_side.yaml" \
    --params-file "$HERE/config/credentials.yaml" \
    -p topic.name:="$TOPIC" \
    -p topic.type:="$TYPE" \
    -p topic.direction:=from_corelink \
    -p qos.reliability:="$REL" \
    -p corelink.data_protocol:="$PROTO"
