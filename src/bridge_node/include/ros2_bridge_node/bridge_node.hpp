#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <rclcpp/generic_publisher.hpp>
#include <rclcpp/generic_subscription.hpp>
#include <std_msgs/msg/empty.hpp>

#include "ros2_bridge_node/corelink_transport.hpp"
#include "ros2_bridge_node/fragment.hpp"
#include "ros2_bridge_node/pacing.hpp"

namespace ros2_bridge_node
{

// to_corelink
// from_corelink

class BridgeNode : public rclcpp::Node
{
public:
    BridgeNode();
    ~BridgeNode() override;

private:
    void setupToCorelink();
    void setupFromCorelink();

    void onLocalMessage(std::shared_ptr<rclcpp::SerializedMessage> message);
    void onCorelinkMessage(const corelink::utils::json &headers, const std::vector<uint8_t> &data);


    void dispatchFragments(std::vector<std::vector<uint8_t>> &&packets);
    void pacerLoop();

    void startKeepalive();
    void startPeerActivityWatchdog();
    void failReceiver(const std::string &reason);

    std::string m_topic_name;
    std::string m_topic_type;
    std::string m_direction;
    std::string m_workspace;
    std::string m_stream_type;

    std::unique_ptr<CorelinkTransport> m_transport;

    // outgoing frames are sliced into MTU-sized packets,

    bridge_node::Slicer m_slicer;
    bridge_node::Reassembler m_reassembler;

    double m_max_rate_hz{0.0};
    std::chrono::steady_clock::time_point m_last_forwarded_at{};

    // Manual pacing in microseconds. 
    int64_t m_pacing_us{0};
    double m_auto_target_rate_hz{0.0};
    double m_auto_reserve_fraction{0.1};
    std::size_t m_pacer_queue_max{0};

    struct PacedFragment
    {
        std::vector<uint8_t> packet;
        int64_t gap_after_us{0};
    };

    std::thread m_pacer_thread;
    std::mutex m_pacer_mutex;
    std::condition_variable m_pacer_cv;
    std::deque<PacedFragment> m_pacer_queue;         
    bool m_pacer_stop{false};                        
    std::size_t m_pacer_dropped_frames{0};           
    
    bool m_diag_packet_sizes{false};
    std::size_t m_diag_packets_seen{0};
    std::map<std::size_t, std::size_t> m_diag_size_histogram;

    

    //k8s keepalive, required for UDP 
    int64_t m_keepalive_s{0};
    std::atomic<bool> m_data_channel_ready{false};
    std::atomic<bool> m_receiver_failed{false};
    rclcpp::TimerBase::SharedPtr m_keepalive_timer;
    rclcpp::OnShutdownCallbackHandle m_shutdown_handle;

    int64_t m_peer_activity_timeout_s{0};
    std::atomic<bool> m_received_data{false};
    bool m_peer_watchdog_armed{false};
    std::chrono::steady_clock::time_point m_peer_watchdog_deadline{};
    rclcpp::Publisher<std_msgs::msg::Empty>::SharedPtr m_activity_publisher;
    rclcpp::Subscription<std_msgs::msg::Empty>::SharedPtr m_activity_subscription;
    rclcpp::TimerBase::SharedPtr m_activity_watchdog_timer;

    corelink::core::network::channel_id_type m_data_channel_id{};
    rclcpp::GenericSubscription::SharedPtr m_local_subscription;
    rclcpp::GenericPublisher::SharedPtr m_local_publisher;
};

} // namespace ros2_bridge_node
