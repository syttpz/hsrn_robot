#!/bin/bash
set -e

source /opt/ros/humble/setup.bash

if [ -f "${ROS_WS}/install/setup.bash" ]; then
    source "${ROS_WS}/install/setup.bash"
fi

exec "$@"
