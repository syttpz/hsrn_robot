FROM ros:humble-ros-base
ENV DEBIAN_FRONTEND=noninteractive
SHELL ["/bin/bash", "-c"]
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config python3-colcon-common-extensions \
    libasio-dev rapidjson-dev libwebsockets-dev libssl-dev \
    ros-humble-rtabmap-ros ros-humble-image-transport-plugins \
    ros-humble-nav2-map-server  \
    && rm -rf /var/lib/apt/lists/*
ENV ROS_WS=/ros_ws
WORKDIR ${ROS_WS}
COPY src/bridge_node src/bridge_node
COPY src/rtabmap_offload src/rtabmap_offload

COPY ros_entrypoint.sh /ros_entrypoint.sh
RUN source /opt/ros/humble/setup.bash && \
    colcon build --symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Release
RUN chmod +x /ros_entrypoint.sh && \
    echo 'source /opt/ros/humble/setup.bash' >> /root/.bashrc && \
    echo "source ${ROS_WS}/install/setup.bash" >> /root/.bashrc
ENTRYPOINT ["/ros_entrypoint.sh"]
CMD ["ros2", "launch", "rtabmap_offload", "offload.launch.py"]
