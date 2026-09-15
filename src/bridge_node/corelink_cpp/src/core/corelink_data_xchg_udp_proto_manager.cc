#include "core/corelink_data_xchg_udp_proto_manager.hpp"


namespace corelink
{
    namespace core
    {
        namespace network
        {
            namespace
            {
                // Kernel send queue for a data channel socket. Without this the
                // socket gets net.core.wmem_default (208 KB on stock Ubuntu /
                // Raspberry Pi OS), which is smaller than a single burst of
                // application fragments: a 921 KB frame sliced at 16 KB is 58
                // datagrams enqueued back to back in microseconds, ~907 KiB,
                // and everything past the 13th is silently dropped
                // (netstat -su "send buffer errors"). Measured on a Pi 5 over
                // WiFi: 2,277 of 15,022 fragments dropped this way, costing 42
                // of 259 frames. Sized to hold one such burst twice over; going
                // much larger only deepens the queue and adds latency, which for
                // a live sensor stream is worse than dropping.
                constexpr int k_udp_send_buffer_bytes = 2 * 1024 * 1024;

                void apply_socket_buffer_options(const std::shared_ptr<asio::ip::udp::socket> &socket)
                {
                    if (socket == nullptr || !socket->is_open())
                        return;
                    // Best effort: a kernel that clamps this (net.core.wmem_max)
                    // or refuses it must not take the data channel down with it.
                    asio::error_code ignored;
                    socket->set_option(asio::socket_base::send_buffer_size(k_udp_send_buffer_bytes), ignored);
                }
            }

            corelink_data_xchg_udp_protocol_manager::corelink_data_xchg_udp_protocol_manager(
                    const std::shared_ptr<corelink_data_xchg_raw_socket_protocol_context_manager> &context_manager)
                    : corelink_data_xchg_ip_proto_base(),
                      m_context_manager(context_manager)
            {}

            corelink_data_xchg_udp_protocol_manager::corelink_data_xchg_udp_protocol_manager(
                    const corelink::core::network::corelink_data_xchg_udp_protocol_manager &rhs) :
                    corelink_data_xchg_ip_proto_base(rhs),
                    m_context_manager(rhs.m_context_manager)
            {}

            corelink_data_xchg_udp_protocol_manager::corelink_data_xchg_udp_protocol_manager(
                    corelink::core::network::corelink_data_xchg_udp_protocol_manager &&rhs) noexcept:
                    corelink_data_xchg_ip_proto_base(std::move(rhs)),
                    m_context_manager(std::move(rhs.m_context_manager))
            {}

            bool corelink_data_xchg_udp_protocol_manager::init(
                    corelink::core::network::channel_id_type channel_id)
            {
                auto channel_descriptor = std::dynamic_pointer_cast<udp_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                if (channel_descriptor != nullptr)
                {
                    channel_descriptor->endpoint_resolver =
                            std::make_shared<asio::ip::udp::resolver>(m_context_manager->get_context());

                    auto self = std::dynamic_pointer_cast<corelink_data_xchg_udp_protocol_manager>(
                            shared_from_this());
                    channel_descriptor->endpoint_resolver->async_resolve(
                            channel_descriptor->endpoint,
                            std::to_string(channel_descriptor->port_number),
                            [
                                    self,
                                    channel_descriptor
                            ](
                                    const asio::error_code &code,
                                    const asio::ip::udp::resolver::results_type &resolver_result)
                            {
                                // check if the resolution was successful
                                if (code)
                                {
                                    if (channel_descriptor->on_error)
                                    {
                                        std::stringstream error;
                                        // NOT. return now and tell the user what went wrong
                                        error << "channel resolution error: Code [" << code.value() << "] Name["
                                              << code.category().name()
                                              << "] Message: "
                                              << code.message() << "\n";
                                        channel_descriptor->on_error(channel_descriptor->channel_id, error.str());
                                    }
                                    channel_descriptor->set_state(constants::state::in_error);
                                    return;
                                }

                                // create the socket object
                                channel_descriptor->socket =
                                        std::make_shared<asio::ip::udp::socket>(
                                                self->m_context_manager->get_context().get_executor()
                                        );

                                channel_descriptor->remote_endpoint = *resolver_result.begin();

                                if (channel_descriptor->start_receiver)
                                    self->start_receiver(channel_descriptor->channel_id);


                                if (channel_descriptor->on_init)
                                    channel_descriptor->on_init(channel_descriptor->channel_id);
                            }
                    );
                    channel_descriptor->set_state(constants::state::initialised);
                    return true;
                }
                return false;
            }

            void corelink_data_xchg_udp_protocol_manager::teardown(
                    corelink::core::network::channel_id_type channel_id)
            {
                auto channel_descriptor = std::dynamic_pointer_cast<udp_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                if (channel_descriptor != nullptr)
                {
                    try
                    {
                        // this can throw an exception but that is okay. we can safely ignore it.
                        channel_descriptor->socket->close();
                        channel_descriptor->socket.reset();
                        channel_descriptor->reset_state(constants::state::initialised).reset_state(
                                constants::state::connected);
                    }
                    catch (std::exception &)
                    {}
                }
            }

            void corelink_data_xchg_udp_protocol_manager::start_receiver(
                    corelink::core::network::channel_id_type channel_id,
                    bool reentrant)
            {
                auto channel_impl = std::dynamic_pointer_cast<udp_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                // teardown() does socket.reset(), so the pointer is null after
                // it runs. The completion handler below re-enters here after
                // reporting the "Operation aborted" that teardown itself
                // caused, which dereferenced that null pointer and crashed the
                // process on every shutdown. See CORELINK_PATCHES.md patch 5.
                if (channel_impl != nullptr && channel_impl->socket != nullptr)
                {
                    if (!channel_impl->socket->is_open())
                    {
                        channel_impl->socket->open(channel_impl->remote_endpoint.protocol());
                        apply_socket_buffer_options(channel_impl->socket);
                    }

                    auto self = std::dynamic_pointer_cast<corelink_data_xchg_udp_protocol_manager>(
                            shared_from_this());
                    channel_impl->socket->async_receive(
                            asio::buffer(channel_impl->receive_buffer),
                            [
                                    reentrant,
                                    channel_impl,
                                    self
                            ]
                                    (
                                            const asio::error_code &error,
                                            const size_t &bytes_transferred
                                    )
                            {
                                if (error == asio::error::eof)
                                {
                                    if (channel_impl->on_error)
                                    {
                                        std::stringstream error_str;
                                        error_str << "Connection termination: Code [" << error.value() << "] Name["
                                                  << error.category().name()
                                                  << "] Message: "
                                                  << error.message() << "\n";
                                        channel_impl->on_error(channel_impl->channel_id, error_str.str());
                                    }
                                    return;
                                }
                                if (error)
                                {
                                    if (channel_impl->on_error)
                                    {
                                        std::stringstream error_str;
                                        error_str << "Error: Code [" << error.value() << "] Name["
                                                  << error.category().name()
                                                  << "] Message: "
                                                  << error.message() << "\n";
                                        channel_impl->on_error(channel_impl->channel_id, error_str.str());
                                    }
                                }
                                else
                                {
                                    // preserve datagram boundaries
                                    if (channel_impl->on_receive && bytes_transferred > 0)
                                    {
                                        std::vector<uint8_t>
                                                buff_copy(channel_impl->receive_buffer.data(),
                                                          channel_impl->receive_buffer.data() +
                                                          bytes_transferred);
                                        channel_impl->on_receive(channel_impl->channel_id, buff_copy);
                                    }
                                }
                                if (reentrant)
                                    self->start_receiver(channel_impl->channel_id);
                            }
                    );
                }
            }

            void corelink_data_xchg_udp_protocol_manager::send_data(
                    std::vector<uint8_t> &&data,
                    corelink::core::network::channel_id_type channel_id)
            {
                auto channel_impl = std::dynamic_pointer_cast<udp_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                // Same null socket as in start_receiver: a send racing teardown
                // must be dropped, not dereferenced.
                if (channel_impl != nullptr && channel_impl->socket != nullptr)
                {
                    if (!channel_impl->socket->is_open())
                    {
                        channel_impl->socket->open(channel_impl->remote_endpoint.protocol());
                        apply_socket_buffer_options(channel_impl->socket);
                    }

                    if (channel_impl->socket->is_open())
                    {
                        auto self = std::dynamic_pointer_cast<corelink_data_xchg_udp_protocol_manager>(
                                shared_from_this());

                        auto data_slot_id = corelink::core::memory::assign_or_override_slot(std::move(data));
                        auto &held_data = memory::get_packet(data_slot_id);

                        channel_impl->socket->async_send_to(
                                asio::buffer(held_data, held_data.size()),
                                channel_impl->remote_endpoint,
                                [
                                        self,
                                        channel_impl,
                                        data_slot_id
                                ](
                                        const asio::error_code &error,
                                        const std::size_t &bytes_transferred
                                )
                                {
                                    if (error)
                                    {
                                        std::stringstream error_str;
                                        error_str << "Error: Code [" << error.value() << "] Name["
                                                  << error.category().name()
                                                  << "] Message: "
                                                  << error.message() << "\n";
                                        if (channel_impl->on_error)
                                            channel_impl->on_error(channel_impl->channel_id, error_str.str());
                                    }
                                    else if (channel_impl->on_send)
                                    {
                                        auto _ = std::async(std::launch::async | std::launch::deferred,
                                                            [channel_impl](size_t d)
                                                            {
                                                                channel_impl->on_send(channel_impl->channel_id, d);
                                                            },
                                                            bytes_transferred);
                                    }
                                    memory::clear_packet(data_slot_id);
                                });
                        return;
                    }
                    if (channel_impl->on_error)
                    {
                        std::stringstream error;
                        error << "Either channel was not found, or, the socket is not open for sending data.\n \
                         Please check the output from the connect function.\n";
                        channel_impl->on_error(channel_impl->channel_id, error.str());
                    }
                }
            }
        }
    }
}