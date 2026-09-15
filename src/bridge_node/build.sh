#!/usr/bin/env bash
set -eo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$HERE/build"

source /opt/ros/humble/setup.bash
set -u

MULTIARCH="$(gcc -print-multiarch 2>/dev/null || echo x86_64-linux-gnu)"
LIBDIR="/usr/lib/$MULTIARCH"

echo "[build] arch=$MULTIARCH  build_dir=$BUILD_DIR"
cmake -S "$HERE" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCORELINK_LWS_LIB_PATH="$LIBDIR" \
    -DCORELINK_OPENSSL_LIB_PATH="$LIBDIR" \
    -DCORELINK_OPENSSL_BIN_PATH="$LIBDIR"
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "[build] done -> $BUILD_DIR/ros2_bridge_node"
