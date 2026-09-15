#pragma once

#if defined(CORELINK_USE_UDP)

#include "corelink_data_xchg_ip_proto_base.hpp"
#include "corelink_data_xchg_raw_socket_protocol_context_manager.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            // forward declare
            class corelink_data_xchg_udp_protocol_manager;

            /**
             * @brief UDP channel descriptor. Struct mostly contains information initialized to make connections and store callbacks
             * in case of events like send, on receive etc. This is entirely user defined
             */
            struct CORELINK_EXPORT udp_protocol_per_channel_descriptor : ip_protocol_channel_descriptor
            {
            private:
                // friend declaration
                friend class corelink_data_xchg_udp_protocol_manager;

                bool start_receiver = false;

                /**
                 * @brief ASIO UDP socket
                 */
                std::shared_ptr<asio::ip::udp::socket> socket;
                /**
                 * @brief ASIO UDP endpoint resolver
                 */
                std::shared_ptr<asio::ip::udp::resolver> endpoint_resolver;
                /**
                 * @brief Remote endpoint to connect to
                 */
                asio::ip::udp::endpoint remote_endpoint;
                /**
                 * @brief The data buffer in which ASIO connection writes information in
                 */
                std::vector<uint8_t> receive_buffer;
                /**
                 * @brief The number of bytes read at each iteration of the async read stage. this is needed as
                 * TCP reads can happen in chunks.
                 */
                size_t bytes_read;

            public:
                /**
                 * @brief Constructor
                 */
                explicit udp_protocol_per_channel_descriptor(bool start_rx = false)
                        : ip_protocol_channel_descriptor(constants::protocols::udp),
                          start_receiver(start_rx),
                          receive_buffer(0xffff),
                          bytes_read(0)
                {}

                /**
                 * @brief Copy constructor
                 * @param rhs instance to copy from
                 */
                udp_protocol_per_channel_descriptor(const udp_protocol_per_channel_descriptor &rhs)
                        : ip_protocol_channel_descriptor(rhs),
                          receive_buffer(0xffff),
                          bytes_read(0)
                {}

                /**
                 * @brief Move constructor
                 * @param rhs instance to move from
                 */
                udp_protocol_per_channel_descriptor(udp_protocol_per_channel_descriptor &&rhs) noexcept
                        : ip_protocol_channel_descriptor(std::move(rhs)),
                          socket(std::move(rhs.socket)),
                          receive_buffer(std::move(rhs.receive_buffer)),
                          bytes_read(rhs.bytes_read)
                {}

                /**
                 * @brief Destructor
                 */
                virtual ~udp_protocol_per_channel_descriptor() = default;

                /**
                 * @brief Get the UDP endpoint
                 * @param local if set to true, the function returns the local endpoint of the connection. if false it returns
                 * the remote endpoint
                 * @return ip:port formatted as string
                 */
                inline std::string get_endpoint(bool local = false) const
                {
                    std::stringstream ep;
                    auto my_endpoint = local ? socket->local_endpoint() : socket->remote_endpoint();
                    ep << my_endpoint.address() << ":" << my_endpoint.port();
                    return ep.str();
                }
            };

            /**
             * @brief The UDP protocol manager
             * @details This class manages all the UDP protocol and data movement made via the add_channel and init methods.
             * It allows for removing UDP channels and killing active transactions.
             */
            class CORELINK_EXPORT corelink_data_xchg_udp_protocol_manager :
                    public corelink_data_xchg_ip_proto_base
            {
            private:
                /**
                 * @brief ASIO IO context manager
                 */
                std::shared_ptr<corelink_data_xchg_raw_socket_protocol_context_manager> m_context_manager;

            public:
                /**
                 * @brief Constructor
                 * @param context_manager shared pointer to ASIO io context manager instance
                 */
                explicit corelink_data_xchg_udp_protocol_manager(
                        const std::shared_ptr<corelink_data_xchg_raw_socket_protocol_context_manager> &context_manager);

                /**
                 * @brief Copy constructor
                 * @param rhs instance to copy from. Note that the pointer to the context manager is copied not the manager itself.
                 * Hence, if the previous instance was destroyed or stopped running, the same will reflect here.
                 */
                corelink_data_xchg_udp_protocol_manager(const corelink_data_xchg_udp_protocol_manager &rhs);

                /**
                 * @brief Move constructor
                 * @param rhs instance to move from. Note that the pointer to the context manager is moved not the manager itself.
                 * Hence, if the previous instance was destroyed or stopped running, the same will reflect here.
                 */
                corelink_data_xchg_udp_protocol_manager(corelink_data_xchg_udp_protocol_manager &&rhs) noexcept;

                /**
                 * @brief Destructor
                 */
                virtual ~corelink_data_xchg_udp_protocol_manager() = default;

                /**
                 * @brief Initialise the connection to the channel
                 * @details This function does 2 things primarily. It first resolves the endpoint supplied through a DNS lookup
                 * and then it tries to establish a UDP socket to the resolved endpoint. Note that DNS resolution can return
                 * multiple endpoints, and the first successful connection is made. If you need to specify a specific IP port pair, you should do so.
                 * Also, note that this function is async in nature. i.e. you cannot rely on the return value of this function directly.
                 * Instead, on successful socket creation, it invokes a callback on the on_init function defined as per the channel descriptor
                 * Obviously, UDP is connectionless, but it is a pseudo call to make sure that the interface is consistent
                 * @param channel_id The channel id returned from the add_channel function.
                 * @return true if the init was okay.
                 */
                bool init(channel_id_type channel_id);

                void teardown(channel_id_type channel_id);

                void start_receiver(channel_id_type channel_id, bool reentrant = true);

                void send_data(std::vector<uint8_t> &&data, channel_id_type channel_id);
            };
        }
    }
}

#endif // CORELINK_USE_UDP