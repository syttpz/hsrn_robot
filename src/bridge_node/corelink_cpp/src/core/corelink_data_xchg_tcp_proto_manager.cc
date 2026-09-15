#include "core/corelink_data_xchg_tcp_proto_manager.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            corelink_data_xchg_tcp_protocol_manager::corelink_data_xchg_tcp_protocol_manager(
                    const std::shared_ptr<corelink_data_xchg_raw_socket_protocol_context_manager> &context_manager)
                    : corelink_data_xchg_ip_proto_base(),
                      m_context_manager(context_manager)
            {}

            corelink_data_xchg_tcp_protocol_manager::corelink_data_xchg_tcp_protocol_manager(
                    const corelink::core::network::corelink_data_xchg_tcp_protocol_manager &rhs)
                    : corelink_data_xchg_ip_proto_base(rhs),
                      m_context_manager(rhs.m_context_manager)
            {}

            corelink_data_xchg_tcp_protocol_manager::corelink_data_xchg_tcp_protocol_manager(
                    corelink::core::network::corelink_data_xchg_tcp_protocol_manager &&rhs) noexcept
                    : corelink_data_xchg_ip_proto_base(std::move(rhs)),
                      m_context_manager(std::move(rhs.m_context_manager))
            {}

            bool corelink_data_xchg_tcp_protocol_manager::init(corelink::core::network::channel_id_type channel_id)
            {
                // get the channel descriptor
                auto channel_descriptor = std::dynamic_pointer_cast<tcp_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                // perform null checks on the channel descriptor
                if (channel_descriptor)
                {
                    // create an endpoint resolve object
                    channel_descriptor->endpoint_resolver =
                            std::make_shared<asio::ip::tcp::resolver>(m_context_manager->get_context());

                    // resolve the endpoint async. the handler will be called when resolved.
                    auto self = std::dynamic_pointer_cast<corelink_data_xchg_tcp_protocol_manager>(
                            shared_from_this());
                    channel_descriptor->endpoint_resolver->async_resolve(
                            channel_descriptor->endpoint,
                            std::to_string(channel_descriptor->port_number),
                            [
                                    self,
                                    channel_descriptor
                            ](const asio::error_code &code,
                              const asio::ip::tcp::resolver::results_type &resolver_result)
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

                                // create the socket
                                channel_descriptor->socket =
                                        std::make_shared<asio::ip::tcp::socket>(
                                                self->m_context_manager->get_context());

                                // so the resolver worked ok. Now, try to connect to ALL the resolved endpoints.
                                // and stop when a successful connection is established. If not, then return with an appropriate error message
                                asio::async_connect(
                                        *channel_descriptor->socket,
                                        resolver_result,
                                        [channel_descriptor, self](
                                                const asio::error_code &code,
                                                const asio::ip::tcp::endpoint &/*ep*/)
                                        {
                                            // check if the connection worked
                                            if (code)
                                            {
                                                if (channel_descriptor->on_error)
                                                {
                                                    // connection failed to all endpoints. capture the error and notify the user
                                                    std::stringstream error;
                                                    error.clear();
                                                    error << "Connection error: Code [" << code.value() << "] Name["
                                                          << code.category().name()
                                                          << "] Message: "
                                                          << code.message() << "\n";
                                                    channel_descriptor->on_error(channel_descriptor->channel_id,
                                                                                 error.str());
                                                }
                                                channel_descriptor->set_state(constants::state::in_error);
                                                return;
                                            }

                                            if (channel_descriptor->dont_batch)
                                                channel_descriptor->socket->set_option(
                                                        asio::ip::tcp::no_delay(true));

                                            // connection was successful. invoke the user callback for connection.
                                            channel_descriptor->set_state(constants::state::connected);
                                            if (channel_descriptor->start_receiver)
                                                self->start_receiver(channel_descriptor->channel_id);

                                            if (channel_descriptor->on_init)
                                                channel_descriptor->on_init(channel_descriptor->channel_id);
                                        });
                            });
                    channel_descriptor->set_state(constants::state::initialised);
                    return true;
                }
                return false;
            }

            void corelink_data_xchg_tcp_protocol_manager::teardown(corelink::core::network::channel_id_type channel_id)
            {
                auto channel_descriptor = std::dynamic_pointer_cast<tcp_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                if (channel_descriptor != nullptr)
                {
                    if (channel_descriptor->is_set(constants::state::connected) &&
                        channel_descriptor->socket->is_open())
                    {
                        try
                        {
                            // this can throw an exception but that is okay. we can safely ignore it.
                            channel_descriptor->socket->close();
                            channel_descriptor->socket.reset();
                            channel_descriptor->reset_state(constants::state::initialised);
                            channel_descriptor->reset_state(constants::state::connected);
                        }
                        catch (std::exception &)
                        {}
                    }
                }
            }

            void corelink_data_xchg_tcp_protocol_manager::send_data(
                    std::vector<uint8_t> &&data,
                    corelink::core::network::channel_id_type channel_id)
            {
                auto channel_descriptor = std::dynamic_pointer_cast<tcp_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                if (channel_descriptor != nullptr)
                {
                    auto self = std::dynamic_pointer_cast<corelink_data_xchg_tcp_protocol_manager>(
                            shared_from_this());
                    // check if the channel is supposed to transmit
                    // if the socket is connected and open, then send data asynchronously.
                    if (channel_descriptor->socket->is_open() &&
                        channel_descriptor->is_set(constants::state::connected))
                    {
                        auto data_slot_id = memory::assign_or_override_slot(std::move(data));
                        auto &held_data = memory::get_packet(data_slot_id);
                        asio::async_write(
                                *channel_descriptor->socket,
                                asio::buffer(held_data, held_data.size()),
                                [
                                        self,
                                        channel_descriptor,
                                        data_slot_id
                                ](
                                        const asio::error_code &error,
                                        const std::size_t &bytes_transferred
                                )
                                {
                                    // capture any errors that might have occurred while sending data
                                    if (error)
                                    {
                                        std::stringstream error_str;
                                        error_str << "Write Error: Code [" << error.value() << "] Name["
                                                  << error.category().name()
                                                  << "] Message: "
                                                  << error.message() << "\n";
                                        if (channel_descriptor->on_error)
                                            channel_descriptor->on_error(channel_descriptor->channel_id,
                                                                         error_str.str());
                                    }
                                        // if the on_send callback is valid, invoke it and send the number of bytes that were transmitted
                                    else if (channel_descriptor->on_send)
                                    {
                                        auto _ = std::async(
                                                std::launch::async | std::launch::deferred,
                                                [channel_descriptor](size_t d)
                                                {
                                                    channel_descriptor->on_send(channel_descriptor->channel_id, d);
                                                },
                                                bytes_transferred);
                                    }
                                    memory::clear_packet(data_slot_id);
                                });
                        return;
                    }
                    // if the socket was not open or connected, report an error
                    if (channel_descriptor->on_error)
                    {
                        std::stringstream error;
                        error << "Either channel was not found, or, the socket is not open for sending data.\n \
                         Please check if socket was init ok based on the channel connect or error callbacks.\n";
                        channel_descriptor->on_error(channel_descriptor->channel_id, error.str());
                    }
                }
            }

            void corelink_data_xchg_tcp_protocol_manager::start_receiver(
                    corelink::core::network::channel_id_type channel_id,
                    bool reentrant)
            {
                auto channel_descriptor = std::dynamic_pointer_cast<tcp_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                if (channel_descriptor != nullptr)
                {
                    // check if the socket is open and connected. if not, return
                    if (!(channel_descriptor->socket->is_open() &&
                          channel_descriptor->is_set(constants::state::connected)))
                    {
                        if (channel_descriptor->on_error)
                            channel_descriptor->on_error(channel_descriptor->channel_id,
                                                         "Socket is not connected yet. Either connect it or wait for its connection");
                        return;
                    }

                    auto self = std::dynamic_pointer_cast<corelink_data_xchg_tcp_protocol_manager>(
                            shared_from_this());
                    // keep reading data till there is nothing more to read
                    channel_descriptor->socket->async_read_some(
                            asio::buffer(channel_descriptor->receive_buffer),
                            [reentrant, channel_descriptor, self]
                                    (
                                            in<asio::error_code> error,
                                            in<size_t> bytes_transferred
                                    )
                            {
                                // this means that the connection was terminated by either party.
                                if (error == asio::error::eof)
                                {
                                    if (channel_descriptor->on_error)
                                    {
                                        std::stringstream error_str;
                                        error_str << "Connection termination: Code [" << error.value() << "] Name["
                                                  << error.category().name()
                                                  << "] Message: "
                                                  << error.message() << "\n";
                                        channel_descriptor->on_error(channel_descriptor->channel_id,
                                                                     error_str.str());
                                    }
                                    return;
                                }
                                // this is some other error, perhaps. capture it
                                if (error)
                                {
                                    if (channel_descriptor->on_error)
                                    {
                                        std::stringstream error_str;
                                        error_str << "Read Error: Code [" << error.value() << "] Name["
                                                  << error.category().name()
                                                  << "] Message: "
                                                  << error.message() << "\n";
                                        auto s = error_str.str();
                                        auto _ = std::async(
                                                std::launch::async | std::launch::deferred,
                                                [channel_descriptor, s]()
                                                {
                                                    channel_descriptor->on_error(channel_descriptor->channel_id, s);
                                                }
                                        );
                                    }
                                }
                                else
                                {
                                    // data was read fine. read ahead if more data is available
                                    if (channel_descriptor->on_receive)
                                    {
                                        std::vector<uint8_t>
                                                buff_copy(channel_descriptor->receive_buffer.begin(),
                                                          channel_descriptor->receive_buffer.begin() +
                                                          bytes_transferred);
                                        // if read all data, accumulate it and call the user function and pass it the data
                                        channel_descriptor->on_receive(channel_descriptor->channel_id, buff_copy);
                                        channel_descriptor->bytes_read = 0;
                                    }
                                }
                                if (reentrant)
                                    self->start_receiver(channel_descriptor->channel_id);
                            }
                    );
                }
            }
        }
    }
}