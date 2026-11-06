# Nav2 (A* + Regulated Pure Pursuit) with a simulated robot on an RTAB-Map database.
FROM ros:humble-ros-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ros-humble-navigation2 \
        ros-humble-nav2-bringup \
        ros-humble-rtabmap-ros \
        ros-humble-rviz2 \
        tigervnc-standalone-server \
        novnc \
        python3-websockify \
        openbox \
        libgl1-mesa-dri \
        fonts-dejavu-core \
        python3-numpy \
        python3-yaml \
    && rm -rf /var/lib/apt/lists/*

ENV LIBGL_ALWAYS_SOFTWARE=1 \
    ROS_DOMAIN_ID=42

COPY . /opt/nav_sim
RUN chmod +x /opt/nav_sim/entrypoint.sh \
    && echo 'source /opt/ros/humble/setup.bash' >> /root/.bashrc

WORKDIR /maps
CMD ["/opt/nav_sim/entrypoint.sh"]
