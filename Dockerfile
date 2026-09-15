#  build on pi
#     docker build -t robot:humble .
#
#  run
#     docker run -it --rm \
#        --network host \
#        --privileged \
#        -v /dev:/dev \
#        -v /dev/bus/usb:/dev/bus/usb \
#        robot:humble

# ros:humble arm64
FROM --platform=linux/arm64 ros:humble-ros-base

ENV DEBIAN_FRONTEND=noninteractive
ENV LANG=en_US.utf8

SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y --no-install-recommends \
        curl \
        wget \
        vim \
        gnupg2 \
        lsb-release \
        ca-certificates \
        software-properties-common \
        udev \
        usbutils \
        python3-pip \
        build-essential \
        cmake \
        pkg-config \
    && rm -rf /var/lib/apt/lists/*


# librealsense sdk
RUN mkdir -p /etc/apt/keyrings && \
    curl -sSf https://librealsense.realsenseai.com/Debian/librealsenseai.asc | \
        gpg --dearmor -o /etc/apt/keyrings/librealsenseai.gpg && \
    echo "deb [signed-by=/etc/apt/keyrings/librealsenseai.gpg] https://librealsense.realsenseai.com/Debian/apt-repo $(. /etc/os-release && echo $VERSION_CODENAME) main" \
        > /etc/apt/sources.list.d/librealsense.list

# librealsense2-dkms (kernel module patch) only for amd64;
# arm64 relies on librealsense2-udev-rules instead for device permissions.
RUN apt-get update && \
    if [ "$(dpkg --print-architecture)" = "amd64" ]; then \
        apt-get install -y --no-install-recommends librealsense2-dkms; \
    else \
        apt-get install -y --no-install-recommends librealsense2-udev-rules; \
    fi && \
    apt-get install -y --no-install-recommends \
        librealsense2-utils \
        librealsense2-dev \
        librealsense2-dbg && \
    rm -rf /var/lib/apt/lists/*



# ros-humble-realsense2-camera auto pulls ros-humble-librealsense2 (RealSense SDK)
RUN apt-get update && apt-get install -y --no-install-recommends \
        # ROS2 wrapper [to be replaced]
        ros-humble-realsense2-* \
        # Rtabmap
        ros-humble-rtabmap-ros \
        # Nav2
        ros-humble-navigation2 \
        ros-humble-nav2-bringup \
        # RPLIDAR A2M12 driver (provides rplidar_node; 256000 baud)
        ros-humble-rplidar-ros \
        # Other tools
        ros-humble-slam-toolbox \
        ros-humble-robot-localization \
        ros-humble-tf2-tools \
        ros-humble-xacro \
        ros-humble-image-transport-plugins \
	ros-humble-teleop-twist-keyboard \
        # DualSense / gamepad teleop
        ros-humble-joy \
        ros-humble-teleop-twist-joy \
        joystick \
        evtest \
        # iRobot Create 3 message definitions (dock/hazard/stop status)
        ros-humble-irobot-create-msgs \
    && rm -rf /var/lib/apt/lists/*

# for camera detection, install udev/rules.d locally
#RUN mkdir -p /etc/udev/rules.d && \
#    curl -fsSL https://raw.githubusercontent.com/IntelRealSense/librealsense/master/config/99-realsense-libusb.rules \
#        -o /etc/udev/rules.d/99-realsense-libusb.rules || \
#    echo "unable to get realsense udev rules"


# corelink build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
        libasio-dev \
        rapidjson-dev \
        libwebsockets-dev \
        libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# workspace
ENV ROS_WS=/root/ros2_ws
RUN mkdir -p ${ROS_WS}/src
WORKDIR ${ROS_WS}


# Auto source
RUN echo "source /opt/ros/humble/setup.bash" >> /root/.bashrc && \
    echo "[ -f ${ROS_WS}/install/setup.bash ] && source ${ROS_WS}/install/setup.bash" >> /root/.bashrc


# entry point to ensure every shell/exec contains the ros env
COPY ros_entrypoint.sh /ros_entrypoint.sh
COPY check_realsense.sh /usr/local/bin/check_realsense.sh
RUN chmod +x /usr/local/bin/check_realsense.sh
RUN chmod +x /ros_entrypoint.sh

# foxglove websocket bridge (camera view / topic inspection from laptop;
# kept in its own layer at the end so adding it doesn't rebuild corelink)
RUN apt-get update && apt-get install -y --no-install-recommends \
        ros-humble-foxglove-bridge \
    && rm -rf /var/lib/apt/lists/*
# ENTRYPOINT ["/ros_entrypoint.sh"]
# CMD ["bash"]
CMD ["/usr/local/bin/check_realsense.sh"]
