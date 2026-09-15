#!/usr/bin/env bash
set -eo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ "${1:-}" != "--no-build" && "${SKIP_BUILD:-0}" != "1" ]]; then
  "$HERE/build.sh"
fi

source /opt/ros/humble/setup.bash
set -u

BIN="$HERE/build/ros2_bridge_node"
PARAMS="$HERE/config/robot_side.yaml"
CREDS="$HERE/config/credentials.yaml"

PROTO="${PROTO:-udp}"
REL="${REL:-best_effort}"
QUEUE_MAX="${QUEUE_MAX:-256}"
RGB_PACING="${RGB_PACING:-0}"
DEPTH_PACING="${DEPTH_PACING:-0}"
INFO_PACING="${INFO_PACING:-0}"
FORWARD_RATE="${FORWARD_RATE:-0.0}"
AUTO_RATE="${AUTO_RATE:-6.0}"
AUTO_RESERVE="${AUTO_RESERVE:-0.10}"
BRIDGE_CLOCK="${BRIDGE_CLOCK:-0}"
COMPRESS_RGB="${COMPRESS_RGB:-1}"
COMPRESS_DEPTH="${COMPRESS_DEPTH:-1}"
JPEG_QUALITY="${JPEG_QUALITY:-90}"

[[ -x "$BIN" ]] || { echo "missing $BIN -- run build.sh" >&2; exit 1; }
[[ -f "$CREDS" ]] || { echo "missing $CREDS -- copy credentials.yaml.example" >&2; exit 1; }

pids=()
cleanup() {
  trap - EXIT INT TERM
  [[ ${#pids[@]} -gt 0 ]] && kill "${pids[@]}" 2>/dev/null || true
  wait 2>/dev/null || true
}
trap cleanup EXIT INT TERM

sender() {
  local name="$1" topic="$2" type="$3" pacing="$4" reliability="${5:-$REL}"
  local auto_rate="${6:-$AUTO_RATE}"
  local stream_type="${7:-$topic}"
  local forward_rate="${8:-$FORWARD_RATE}"
  echo "[robot] $name: $topic ($type) --$PROTO--> corelink  rate=${forward_rate}Hz pacing=${pacing}us qos=${reliability}"
  "$BIN" --ros-args \
      -r __node:="$name" \
      --params-file "$PARAMS" \
      --params-file "$CREDS" \
      -p topic.name:="$topic" \
      -p topic.type:="$type" \
      -p topic.direction:=to_corelink \
      -p topic.max_rate:="$forward_rate" \
      -p corelink.stream_type:="$stream_type" \
      -p qos.reliability:="$reliability" \
      -p corelink.data_protocol:="$PROTO" \
      -p send.pacing_us:="$pacing" \
      -p send.auto_target_rate_hz:="$auto_rate" \
      -p send.auto_reserve_fraction:="$AUTO_RESERVE" \
      -p send.pacing_queue_max:="$QUEUE_MAX" &
  pids+=($!)
}

if [[ "$COMPRESS_RGB" = "1" ]]; then
  RGB_RAW=/camera/camera/color/image_raw
  RGB_COMPRESSED="$RGB_RAW/compressed"
  echo "[robot] JPEG encoder: $RGB_RAW -> $RGB_COMPRESSED (quality=$JPEG_QUALITY)"
  ros2 run image_transport republish raw compressed --ros-args \
      -r in:="$RGB_RAW" \
      -r out/compressed:="$RGB_COMPRESSED" \
      -p out.jpeg_quality:="$JPEG_QUALITY" &
  pids+=($!)
  sender color_image_sender "$RGB_COMPRESSED" \
      sensor_msgs/msg/CompressedImage "$RGB_PACING" "$REL" "$AUTO_RATE" "$RGB_RAW"
else
  sender color_image_sender \
      /camera/camera/color/image_raw sensor_msgs/msg/Image "$RGB_PACING"
fi
if [[ "$COMPRESS_DEPTH" = "1" ]]; then
  DEPTH_RAW=/camera/camera/aligned_depth_to_color/image_raw
  DEPTH_COMPRESSED="$DEPTH_RAW/compressedDepth"
  echo "[robot] lossless depth encoder: $DEPTH_RAW -> $DEPTH_COMPRESSED"
  ros2 run image_transport republish raw compressedDepth --ros-args \
      -r in:="$DEPTH_RAW" \
      -r out/compressedDepth:="$DEPTH_COMPRESSED" &
  pids+=($!)
  sender depth_image_sender "$DEPTH_COMPRESSED" \
      sensor_msgs/msg/CompressedImage "$DEPTH_PACING" "$REL" "$AUTO_RATE" "$DEPTH_RAW"
else
  sender depth_image_sender \
      /camera/camera/aligned_depth_to_color/image_raw sensor_msgs/msg/Image "$DEPTH_PACING"
fi
sender camera_info_sender \
    /camera/camera/color/camera_info                 sensor_msgs/msg/CameraInfo "$INFO_PACING"

if [[ "$BRIDGE_CLOCK" = "1" ]]; then
  sender clock_sender /clock rosgraph_msgs/msg/Clock 0 best_effort 0.0 ros2_clock 0.0
fi

if [[ -n "${BAG:-}" ]]; then
  echo "[robot] waiting 8s for corelink streams, then playing $BAG at ${RATE:-1.0}x"
  sleep 8
  clock_args=()
  [[ "$BRIDGE_CLOCK" = "1" ]] && clock_args+=(--clock)
  ros2 bag play "$BAG" --rate "${RATE:-1.0}" "${clock_args[@]}"
  echo "[robot] bag finished; senders still up (ctrl-c to stop)"
fi

wait -n
echo "[robot] a sender exited -- shutting the rest down" >&2
