#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "corelink_all.hpp"

namespace ros2_bridge_node
{

class CorelinkTransport
{
public:
    using ReadyCallback = std::function<void(bool ok, const std::string &message)>;
    using DisconnectCallback = std::function<void(const std::string &message)>;
    using StreamReadyCallback = std::function<void(corelink::core::network::channel_id_type channel_id)>;
    using ReceiveCallback = std::function<void(
            const corelink::utils::json &headers,
            const std::vector<uint8_t> &data)>;


    CorelinkTransport(
            std::string endpoint,
            uint16_t port,
            std::string username,
            std::string password,
            std::string certificate_path,
            const corelink::core::network::constants::protocols::protocol &control_protocol);

    void connect(ReadyCallback on_ready, DisconnectCallback on_disconnect = nullptr);

    void keepControlAlive(ReadyCallback on_result);

    // creates a sender stream (workspace/stream_type == ROS2 topic name by
    // convention) using the given data-channel protocol
    void createSender(
            const std::string &workspace,
            const std::string &stream_type,
            const corelink::core::network::constants::protocols::protocol &data_protocol,
            StreamReadyCallback on_ready);

    // creates a receiver stream subscribing to stream_type
    void createReceiver(
            const std::string &workspace,
            const std::string &stream_type,
            const corelink::core::network::constants::protocols::protocol &data_protocol,
            ReceiveCallback on_data,
            StreamReadyCallback on_ready);

    void sendData(
            corelink::core::network::channel_id_type data_channel_id,
            std::vector<uint8_t> data);

private:
    corelink::client::corelink_classic_client m_client;
    corelink::client::corelink_client_connection_info m_connection_info;
    corelink::core::network::channel_id_type m_control_channel_id{};
};

} 
