#include <rclcpp/rclcpp.hpp>

#include "ros2_bridge_node/bridge_node.hpp"

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ros2_bridge_node::BridgeNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
