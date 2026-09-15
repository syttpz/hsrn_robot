#pragma once

#if defined(CORELINK_USE_TCP)

#include "corelink_data_xchg_ip_proto_base.hpp"
#include "corelink_data_xchg_raw_socket_protocol_context_manager.hpp"
#include "corelink_network_global_packet_hold.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            // forward declaration
            class corelink_data_xchg_tcp_protocol_manager;

            /**
             * @brief TCP channel descriptor. Struct mostly contains information initialized to make connections and store callbacks
             * in case of events like (dis)/connect, send, on receive etc.
             */
            struct CORELINK_EXPORT tcp_protocol_per_channel_descriptor :
                    public ip_protocol_channel_descriptor
            {
            private:
                bool start_receiver = false;

                bool dont_batch = false;

                // declare this as a friend class as there are some private features not available to users, but will be accessed
                // by the TCP protocol manager
                friend class corelink_data_xchg_tcp_protocol_manager;

                /**
                 * @brief The ASIO TCP socket instance.
                 */
                std::shared_ptr<asio::ip::tcp::socket> socket;
                /**
                 * @brief The ASIO TCP channel resolver.
                 */
                std::shared_ptr<asio::ip::tcp::resolver> endpoint_resolver;
                /**
                 * @brief The data buffer in which ASIO connection writes information in
                 */
                std::array<uint8_t, 0xffff> receive_buffer{};
                /**
                 * @brief The number of bytes read at each iteration of the async read stage. this is needed as
                 * TCP reads can happen in chunks.
                 */
                size_t bytes_read;

            public:

                /**
                 * @brief Constructor
                 */
                explicit tcp_protocol_per_channel_descriptor(bool start_rx = false, bool no_batch = false) :
                        ip_protocol_channel_descriptor(constants::protocols::tcp),
                        start_receiver(start_rx),
                        dont_batch(no_batch),
                        bytes_read(0)
                {}

                /**
                 * @brief Copy constructor
                 * @param rhs instance to copy from
                 */
                tcp_protocol_per_channel_descriptor(const tcp_protocol_per_channel_descriptor &rhs)
                        : ip_protocol_channel_descriptor(rhs),
                          start_receiver(rhs.start_receiver),
                          dont_batch(rhs.dont_batch),
                          receive_buffer(rhs.receive_buffer),
                          bytes_read(0)
                {}

                /**
                 * @brief Move constructor
                 * @param rhs instance to move from
                 */
                tcp_protocol_per_channel_descriptor(tcp_protocol_per_channel_descriptor &&rhs) noexcept
                        : ip_protocol_channel_descriptor(std::move(rhs)),
                          start_receiver(rhs.start_receiver),
                          dont_batch(rhs.dont_batch),
                          socket(std::move(rhs.socket)),
                          receive_buffer(rhs.receive_buffer),
                          bytes_read(rhs.bytes_read)
                {}

                /**
                 * @brief Destructor
                 */
                virtual ~tcp_protocol_per_channel_descriptor() = default;

                /**
                 * @brief Get the TCP endpoint
                 * @param local if set to true, the function returns the local endpoint of the connection. if false it returns
                 * the remote endpoint
                 * @return ip:port formatted as string
                 */
                std::string get_endpoint(bool local = false) const
                {
                    std::stringstream ep;
                    auto my_endpoint = local ? socket->local_endpoint() : socket->remote_endpoint();
                    ep << my_endpoint.address() << ":" << my_endpoint.port();
                    return ep.str();
                }
            };

            /**
             * @brief The TCP protocol manager
             * @details This class manages all the TCP connections made via the add_channel and init methods.
             * It allows for disconnecting from those endpoints via the teardown function.
             */
            class CORELINK_EXPORT corelink_data_xchg_tcp_protocol_manager
                    : public corelink_data_xchg_ip_proto_base
            {
            private:
                /**
                 * @brief ASIO io_context manager wrapper instance
                 */
                std::shared_ptr<corelink_data_xchg_raw_socket_protocol_context_manager> m_context_manager;

            public:
                /**
                 * @brief Parameterized constructor
                 * @param context_manager The context manager instance
                 */
                explicit corelink_data_xchg_tcp_protocol_manager(
                        const std::shared_ptr<corelink_data_xchg_raw_socket_protocol_context_manager> &context_manager
                );

                /**
                 * @brief Copy constructor
                 * @param rhs instance to copy from. Note that the pointer to the context manager is copied not the manager itself.
                 * Hence, if the previous instance was destroyed or stopped running, the same will reflect here.
                 */
                corelink_data_xchg_tcp_protocol_manager(const corelink_data_xchg_tcp_protocol_manager &rhs);

                /**
                 * @brief Move constructor
                 * @param rhs instance to move from. Note that the pointer to the context manager is moved not the manager itself.
                 * Hence, if the previous instance was destroyed or stopped running, the same will reflect here.
                 */
                corelink_data_xchg_tcp_protocol_manager(corelink_data_xchg_tcp_protocol_manager &&rhs) noexcept;

                /**
                 * @brief Destructor
                 */
                virtual ~corelink_data_xchg_tcp_protocol_manager() = default;

                /**
                 * @brief Initialise the connection to the channel
                 * @details This function does 2 things primarily. It first resolves the endpoint supplied through a DNS lookup
                 * and then it tries to establish a TCP connection to the resolved endpoint. Note that DNS resolution can return
                 * multiple endpoints, and the first successful connection is made. If you need to specify a specific IP port pair, you should do so.
                 * Also, note that this function is async in nature. i.e. you cannot rely on the return value of this function directly.
                 * Instead, on successful connection to the remote channel, it invokes a callback on the on_connect function defined as per the channel descriptor
                 * @param channel_id The channel id returned from the add_channel function.
                 * @return true if the init was okay.
                 */
                bool init(channel_id_type channel_id);

                /**
                 * @brief Teardown the connection to the channel.
                 * @details This function is responsible for destroying the connection to a remote channel
                 * @param channel_id channel ID to which the connection needs to be closed.
                 */
                void teardown(channel_id_type channel_id);

                /**
                 * @brief Send data to the remote channel to which we are connected
                 * @param data the data which needs to be sent out
                 * @param channel_id the channel ID to which the connection is made.
                 * @param expect_reply Should we expect a response from the channel. Leave this is as true if you are not sure.
                 * If left false, it will not listen to responses from the server
                 */
                void send_data(std::vector<uint8_t> &&data, channel_id_type channel_id);

                /**
                 * @brief Prime the receiver to start listening for incoming data.
                 * @param channel_id the channel ID for the connection made.
                 * @param reentrant Should the receiver function keep reading data over and over again. leave this as default if you don't know.
                 */
                void start_receiver(channel_id_type channel_id, bool reentrant = true);
            };
        }
    }
}

#endif // CORELINK_USE_TCP