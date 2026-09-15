cmake_minimum_required(VERSION 3.20)
project(ros2_bridge_node)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

set(CORELINK_CPP_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/corelink_cpp" CACHE PATH
    "Path to the vendored corelink C++ client directory")

set(CORELINK_ASIO_CPP_PATH "/usr/include" CACHE PATH "Path containing asio.hpp")
set(CORELINK_RAPID_JSON_CPP_PATH "/usr/include" CACHE PATH "Path containing rapidjson/")
set(CORELINK_LWS_INCLUDE_PATH "/usr/include" CACHE PATH "Path containing libwebsockets.h")
set(CORELINK_OPENSSL_INCLUDE_PATH "/usr/include" CACHE PATH "Path containing openssl/")
set(CORELINK_LWS_LIB_PATH "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}" CACHE PATH "Path containing libwebsockets.so")
set(CORELINK_OPENSSL_LIB_PATH "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}" CACHE PATH "Path containing libssl.so/libcrypto.so")
set(CORELINK_OPENSSL_BIN_PATH "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}" CACHE PATH "")

add_subdirectory(${CORELINK_CPP_SRC_DIR} corelink_cpp_build)

add_executable(ros2_bridge_node
  src/main.cpp
  src/bridge_node.cpp
  src/corelink_transport.cpp
  src/fragment.cpp
)

target_include_directories(ros2_bridge_node PRIVATE
  include
)

target_link_libraries(ros2_bridge_node
  corelinkclient
)

ament_target_dependencies(ros2_bridge_node
  rclcpp
  std_msgs
)

set_target_properties(ros2_bridge_node PROPERTIES
  INSTALL_RPATH "$ORIGIN/../../lib"
  BUILD_WITH_INSTALL_RPATH FALSE
)

install(TARGETS ros2_bridge_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(TARGETS corelinkclient
  LIBRARY DESTINATION lib
)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
  OPTIONAL
)

ament_package()
