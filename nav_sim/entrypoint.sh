#!/usr/bin/env bash
# Container start: database -> Nav2 map -> desktop in the browser -> simulation.
set -eo pipefail
source /opt/ros/humble/setup.bash

APP=/opt/nav_sim
DB_PATH="/maps/$DB"
MAP="/maps/${DB%.db}"

[[ -f "$DB_PATH" ]] || { echo "[nav_sim] database not found: $DB_PATH" >&2; exit 1; }

if [[ ! -f "$MAP.yaml" || "$DB_PATH" -nt "$MAP.yaml" ]]; then
  python3 "$APP/scripts/extract_map.py" --db "$DB_PATH" --out "$MAP"
fi
[[ "${EXTRACT_ONLY:-0}" == "1" ]] && exit 0

export DISPLAY=:1
rm -f /tmp/.X1-lock /tmp/.X11-unix/X1
Xtigervnc :1 -geometry 1600x1000 -depth 24 -SecurityTypes None -localhost yes \
    -rfbport 5901 -AlwaysShared >/tmp/xvnc.log 2>&1 &
until [[ -e /tmp/.X11-unix/X1 ]]; do sleep 0.1; done
openbox >/tmp/openbox.log 2>&1 &
websockify --web /usr/share/novnc 6080 localhost:5901 >/tmp/novnc.log 2>&1 &
echo "[nav_sim] open http://localhost:6080/vnc.html?autoconnect=1&resize=remote"

exec ros2 launch "$APP/launch/sim.launch.py" map:="$MAP.yaml"
