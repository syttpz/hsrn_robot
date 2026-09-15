#!/usr/bin/env bash
set -eo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN="$HERE/build/ros2_bridge_node"

PROTO="${1:-udp}"
TOPIC="/${2:-probe_$PROTO}"
DELAY="${DELAY:-5}"
COUNT="${COUNT:-30}"
WAIT="${WAIT:-1}"

case "$PROTO" in udp|tcp|ws) ;; *) echo "protocol must be udp|tcp|ws" >&2; exit 1;; esac
[[ -x "$BIN" ]] || { echo "missing $BIN -- run ./build.sh" >&2; exit 1; }

source /opt/ros/humble/setup.bash
set -u

cat <<EOF
================================================================
 Receiver command for the pod ($([ "$WAIT" = 1 ] && echo "run it FIRST" || echo "run it once this side is publishing")):

   kubectl exec -it -n hsrn-robot \$POD -- bash
   ros2 run ros2_bridge_node ros2_bridge_node --ros-args \\
       -r __node:=probe_rx \\
       -p topic.name:=$TOPIC \\
       -p topic.type:=std_msgs/msg/String \\
       -p topic.direction:=from_corelink \\
       -p corelink.data_protocol:=$PROTO
================================================================
EOF

if [[ "$WAIT" == 1 ]]; then
  read -r -p "receiver up in the pod? [enter] "
fi

pids=()
cleanup() {
  trap - EXIT INT TERM
  [[ ${#pids[@]} -gt 0 ]] && kill "${pids[@]}" 2>/dev/null || true
  wait 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "[probe] sender: $TOPIC over $PROTO"
"$BIN" --ros-args \
    -r __node:=probe_tx \
    -p topic.name:="$TOPIC" \
    -p topic.type:=std_msgs/msg/String \
    -p topic.direction:=to_corelink \
    -p corelink.data_protocol:="$PROTO" &
pids+=($!)

echo "[probe] waiting ${DELAY}s before the first message"
sleep "$DELAY"

echo "[probe] publishing $COUNT messages at 1 Hz"
for i in $(seq 1 "$COUNT"); do
  ros2 topic pub --once "$TOPIC" std_msgs/msg/String \
      "{data: 'probe $PROTO $i'}" >/dev/null
  sleep 1
done

echo "[probe] done -- $COUNT sent. Check the pod for 'Reassembled' lines."
