#!/bin/bash

ros2 action send_goal /navigate_to_pose nav2_msgs/action/NavigateToPose "{
  pose: {
    header: {frame_id: 'map'},
    pose: {
      position: {x: 0.794, y: -6.372, z: 0.0},
      orientation: {x: 0.0, y: 0.0, z: -0.2936, w: 0.9559}
    }
  }
}" --feedback && \
ros2 action send_goal /dock irobot_create_msgs/action/Dock "{}"
