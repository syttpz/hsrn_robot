#include "core/corelink_data_xchg_websocket_proto_manager.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            websocket_protocol_per_channel_descriptor::websocket_protocol_per_channel_descriptor(
                    in<std::string> cert_path)
                    : ip_protocol_channel_descriptor(constants::protocols::websocket),
                      certificate_path(cert_path)
            {}

            websocket_protocol_per_channel_descriptor::websocket_protocol_per_channel_descriptor(
                    in<corelink::core::network::websocket_protocol_per_channel_descriptor> rhs)
                    : ip_protocol_channel_descriptor(rhs),
                      client_wsi(nullptr),
                      certificate_path(rhs.certificate_path)
            {}

            websocket_protocol_per_channel_descriptor::websocket_protocol_per_channel_descriptor(
                    rvref<websocket_protocol_per_channel_descriptor> rhs) noexcept
                    : ip_protocol_channel_descriptor(std::move(rhs)),
                      client_wsi(rhs.client_wsi),
                      certificate_path(std::move(rhs.certificate_path))
            {}

            corelink_data_xchg_websocket_protocol_manager::corelink_data_xchg_websocket_protocol_manager()
                    : corelink_data_xchg_ip_proto_base(), m_context(nullptr)
            {
                m_data_queue = std::make_shared<corelink::utils::containers::concurrent_queue<std::tuple<channel_id_type, data_traits, size_t>>>();
                m_event_queue = std::make_shared<corelink::utils::containers::concurrent_queue<std::tuple<channel_id_type, client_events>>>();
            }

            corelink_data_xchg_websocket_protocol_manager::corelink_data_xchg_websocket_protocol_manager(
                    lvref<corelink::core::network::corelink_data_xchg_websocket_protocol_manager> rhs)
                    : corelink_data_xchg_ip_proto_base(rhs),
                      m_context(rhs.m_context),
                      m_context_create_info(rhs.m_context_create_info)
            {
                m_data_queue = std::make_shared<
                        utils::containers::concurrent_queue<
                                std::tuple<channel_id_type, data_traits, size_t>>>();
                m_event_queue = std::make_shared<
                        utils::containers::concurrent_queue<
                                std::tuple<channel_id_type, client_events>>>();
            }

            corelink_data_xchg_websocket_protocol_manager::corelink_data_xchg_websocket_protocol_manager(
                    rvref<corelink::core::network::corelink_data_xchg_websocket_protocol_manager> rhs) noexcept
                    :
                    corelink_data_xchg_ip_proto_base(std::move(rhs)),
                    m_context(rhs.m_context),
                    m_context_create_info(rhs.m_context_create_info),
                    m_ws_context_runner(std::move(rhs.m_ws_context_runner)),
                    m_data_queue(std::move(rhs.m_data_queue))
            {}

            void corelink_data_xchg_websocket_protocol_manager::setup_protocols()
            {
                m_protocols[0].name = "corelink-websocket-default-protocol";
                m_protocols[0].callback = event_callback;
                m_protocols[0].per_session_data_size = 0;
                m_protocols[0].rx_buffer_size = 0;
                m_protocols[0].id = 0;
                m_protocols[0].user = nullptr;
                m_protocols[0].tx_packet_size = 0;
                m_protocols[1] = LWS_PROTOCOL_LIST_TERM;
            }

            bool corelink_data_xchg_websocket_protocol_manager::make_context()
            {
                setup_protocols();
                memset(&m_context_create_info, 0,
                       sizeof(m_context_create_info)); /* otherwise, uninitialized garbage */

                m_context_create_info.protocols = m_protocols.data();
                m_context_create_info.port = CONTEXT_PORT_NO_LISTEN;
                m_context_create_info.ka_time = 2;
                m_context_create_info.ka_probes = 2;
                m_context_create_info.ka_interval = 2;
                m_context_create_info.timeout_secs = 60;
#if defined(LWS_LIBRARY_VERSION_NUMBER) && LWS_LIBRARY_VERSION_NUMBER >= 4001000
                // lws_context_creation_info::connect_timeout_secs does not exist before
                // libwebsockets ~4.1 (e.g. Ubuntu Jammy ships 4.0.20 via libwebsockets-dev).
                // See ros2_bridge_node/CORELINK_PATCHES.md for why this is guarded instead
                // of just always set.
                m_context_create_info.connect_timeout_secs = 60;
#endif
                m_context_create_info.options = LWS_SERVER_OPTION_EXPLICIT_VHOSTS |
                                                LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
                m_context_create_info.fd_limit_per_thread = 1 + 1 + 1;
                m_context = lws_create_context(&m_context_create_info);

                if (m_context == nullptr)
                {
                    if (on_protocol_error)
                    {
                        std::stringstream ss;
                        ss << __FUNCTION__
                           << ": Context set/ creation failed. \nPlease contact the Corelink team for more information";
                        on_protocol_error(ss.str());
                    }
                    return false;
                }
                return true;
            }

            void corelink_data_xchg_websocket_protocol_manager::run()
            {
                if (!make_context())
                    return;

                int n = 0;
                while (n >= 0 && !m_shutdown)
                {
                    // check for events
                    if (!m_event_queue->empty())
                    {
                        auto event = m_event_queue->peek();
                        channel_id_type channel_id = std::get<0>(event);
                        client_events event_type = std::get<1>(event);
                        auto channel_impl = std::dynamic_pointer_cast<websocket_protocol_per_channel_descriptor>(
                                get_channel(channel_id));

                        switch (event_type)
                        {
                            case client_events::create_client:
                            {
                                if (channel_impl != nullptr)
                                {
                                    lws_client_connect_info client_connect_info{};
                                    memset(&client_connect_info, 0, sizeof(client_connect_info));
                                    if (!channel_impl->certificate_path.empty())
                                    {
                                        m_context_create_info.client_ssl_ca_filepath = channel_impl->certificate_path.c_str();
                                    }
                                    client_connect_info.ssl_connection |= LCCSCF_USE_SSL;
//#ifdef CORELINK_WEBSOCKET_CONNECT_LOCAL_SERVER
                                    client_connect_info.ssl_connection |= LCCSCF_ALLOW_SELFSIGNED
                                                                          | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK
                                                                          | LCCSCF_ALLOW_INSECURE
                                                                          | LCCSCF_ALLOW_EXPIRED;
//#endif
                                    client_connect_info.context = m_context;
                                    client_connect_info.address = channel_impl->endpoint.c_str();
                                    client_connect_info.host = channel_impl->endpoint.c_str();
                                    client_connect_info.origin = channel_impl->endpoint.c_str();
                                    client_connect_info.port = channel_impl->port_number;
                                    client_connect_info.path = "/";
                                    client_connect_info.protocol = m_protocols[0].name;
                                    client_connect_info.pwsi = &channel_impl->client_wsi;
                                    client_connect_info.alpn = "h2;http/1.1";
                                    client_connect_info.vhost = lws_create_vhost(m_context,
                                                                                 &m_context_create_info);

                                    lws_client_connect_via_info(&client_connect_info);
                                    if (channel_impl->client_wsi == nullptr)
                                    {
                                        on_protocol_error(__FUNCTION__ + std::string(
                                                ": Websocket client set/ creation failed. \nPlease contact the Corelink team for more information"));
                                    }
                                    else
                                    {
                                        channel_impl->set_state(constants::state::initialised);
                                        nudge();
                                    }
                                }/**/
                                break;
                            }
                            case client_events::send_data:
                            {
                                if (channel_impl)
                                {
                                    lws_callback_on_writable(channel_impl->client_wsi);
                                }
                                break;
                            }
                            case client_events::close_connection:
                            {
                                if (channel_impl)
                                {
                                    channel_impl->initiate_client_connection_shutdown = true;
                                }
                                break;
                            }
                        }
                        m_event_queue->pop();
                    }
                    // continue with the service loop
                    n = lws_service(m_context, 0);
                }
                lws_context_destroy(m_context);
            }

            std::shared_ptr<websocket_protocol_per_channel_descriptor>
            corelink_data_xchg_websocket_protocol_manager::get_channel_descriptor_by_wsi(lws *wsi)
            {
                for (auto &channel: m_channel_descriptors)
                {
                    auto ptr = std::static_pointer_cast<websocket_protocol_per_channel_descriptor>(channel.second);
                    if (ptr && ptr->client_wsi && ptr->client_wsi == wsi)
                        return ptr;
                }
                return nullptr;
            }

            bool corelink_data_xchg_websocket_protocol_manager::start_context_manager()
            {
                if (m_context != nullptr)
                    return true;

                set_your_ws(
                        std::dynamic_pointer_cast<corelink_data_xchg_websocket_protocol_manager>(
                                shared_from_this())
                );

                lws_set_log_level(LLL_WARN | LLL_ERR, nullptr);
                m_ws_context_runner = corelink_thread(&corelink_data_xchg_websocket_protocol_manager::run, this);
                m_ws_context_runner.detach();
                return true;
            }

            bool
            corelink_data_xchg_websocket_protocol_manager::init(corelink::core::network::channel_id_type channel_id)
            {
                auto channel_impl = std::dynamic_pointer_cast<websocket_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                if (channel_impl == nullptr)
                    return false;
                m_event_queue->push(std::make_tuple(channel_id, client_events::create_client));
                return true;
            }

            void
            corelink_data_xchg_websocket_protocol_manager::teardown(corelink::core::network::channel_id_type channel_id)
            {
                // first disconnect
                m_event_queue->push(std::make_tuple(channel_id, client_events::close_connection));
                // then, make sure everything else de-allocated and set to null
            }

            void corelink_data_xchg_websocket_protocol_manager::send_data(std::vector<uint8_t> &&data,
                                                                          corelink::core::network::channel_id_type channel_id)
            {
                send_data(std::move(data), channel_id, data_traits::bin);
            }

            void corelink_data_xchg_websocket_protocol_manager::send_data(
                    std::vector<uint8_t> &&data,
                    corelink::core::network::channel_id_type channel_id,
                    corelink::core::network::corelink_data_xchg_websocket_protocol_manager::data_traits trait)
            {
                auto channel_impl = std::dynamic_pointer_cast<websocket_protocol_per_channel_descriptor>(
                        get_channel(channel_id));
                if (channel_impl == nullptr)
                    return;
                for (int count = 0; count < LWS_SEND_BUFFER_PRE_PADDING; ++count)
                    data.insert(data.begin(), 0);

                auto data_slot_id = memory::assign_or_override_slot(std::move(data));
                m_event_queue->push(std::make_tuple(channel_id, client_events::send_data));
                m_data_queue->push(std::make_tuple(channel_id, trait, data_slot_id));
                nudge();
            }

            void corelink_data_xchg_websocket_protocol_manager::nudge()
            {
                lws_cancel_service(m_context);
            }

            /**
             * @brief the callback function called by LWS based on the event.
             * @param wsi the current websocket client
             * @param reason the callback reason
             * @param user user info
             * @param in data based on the context
             * @param len length of the data based on the context
             * @return a return code based on the processing.
             */
            int
            event_callback(ptr<lws> wsi, lws_callback_reasons reason, ptr<void> user, ptr<void> in, size_t len)
            {
                switch (reason)
                {
                    case LWS_CALLBACK_PROTOCOL_INIT:
                    {
                        break;
                    }
                        /* because we are protocols[0] ... */
                    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
                    {
                        std::shared_ptr<websocket_protocol_per_channel_descriptor> channel_descriptor = p_ws_proto_mgr->get_channel_descriptor_by_wsi(
                                wsi);

                        channel_descriptor->set_state(constants::state::in_error);
                        std::stringstream error_ss;
                        error_ss << "Client connection error: " << (in ? static_cast<char *>(in) : "UNKNOWN");
                        if (channel_descriptor->on_error)
                            channel_descriptor->on_error(channel_descriptor->channel_id, error_ss.str());
                        channel_descriptor->client_wsi = nullptr;
                        break;
                    }

                    case LWS_CALLBACK_CLIENT_ESTABLISHED:
                    {
                        std::shared_ptr<websocket_protocol_per_channel_descriptor> channel_descriptor = p_ws_proto_mgr->get_channel_descriptor_by_wsi(
                                wsi);

                        channel_descriptor->set_state(constants::state::connected);

                        if (channel_descriptor->on_init)
                            channel_descriptor->on_init(channel_descriptor->channel_id);
                        break;
                    }

                    case LWS_CALLBACK_CLIENT_RECEIVE:
                    {
                        std::shared_ptr<websocket_protocol_per_channel_descriptor> channel_descriptor = p_ws_proto_mgr->get_channel_descriptor_by_wsi(
                                wsi);
                        if (channel_descriptor)
                        {
                            ptr_to_const_val<uint8_t> casted = static_cast<uint8_t *>(in);
                            std::vector<uint8_t> data(casted, casted + len);
                            if (channel_descriptor->on_receive)
                                channel_descriptor->on_receive(channel_descriptor->channel_id, data);
                        }
                        break;
                    }

                    case LWS_CALLBACK_CLIENT_CLOSED:
                    {
                        std::shared_ptr<websocket_protocol_per_channel_descriptor> channel_descriptor = p_ws_proto_mgr->get_channel_descriptor_by_wsi(
                                wsi);

                        channel_descriptor->client_wsi = nullptr;
                        channel_descriptor->reset_state(constants::state::connected);
                        channel_descriptor->reset_state(constants::state::initialised);
                        if (channel_descriptor->on_uninit)
                            channel_descriptor->on_uninit(channel_descriptor->channel_id);
                        break;
                    }

                    case LWS_CALLBACK_CLIENT_WRITEABLE:
                    {
                        try
                        {
                            std::shared_ptr<websocket_protocol_per_channel_descriptor> channel_descriptor =
                                    p_ws_proto_mgr->get_channel_descriptor_by_wsi(wsi);

                            if (channel_descriptor->initiate_client_connection_shutdown)
                                return -1;

                            if (!p_ws_proto_mgr->m_data_queue->empty())
                            {
                                auto data_with_trait = p_ws_proto_mgr->m_data_queue->peek();
                                lws_write_protocol write_proto;
                                switch (std::get<1>(data_with_trait))
                                {
                                    case corelink_data_xchg_websocket_protocol_manager::data_traits::bin:
                                        write_proto = LWS_WRITE_BINARY;
                                        break;
                                    default:
                                    case corelink_data_xchg_websocket_protocol_manager::data_traits::text:
                                        write_proto = LWS_WRITE_TEXT;
                                        break;
                                }
                                auto &data = memory::get_packet(std::get<2>(data_with_trait));
                                int ret_val = lws_write(wsi,
                                                        data.data() + LWS_SEND_BUFFER_PRE_PADDING,
                                                        data.size() - LWS_SEND_BUFFER_PRE_PADDING,
                                                        write_proto);
                                if (channel_descriptor->on_send)
                                {
                                    auto _ = std::async(
                                            std::launch::async,
                                            [channel_descriptor](size_t d)
                                            {
                                                channel_descriptor->on_send(channel_descriptor->channel_id, d);
                                            },
                                            ret_val
                                    );
                                }
                                p_ws_proto_mgr->m_data_queue->pop();
                            }
                            lws_callback_on_writable(wsi);
                        }
                        catch (std::exception &ex)
                        {
                            std::cout << ex.what() << "\n";
                        }
                        break;
                    }
                    default:
                        break;

                }
                return lws_callback_http_dummy(wsi, reason, user, in, len);
            }
        }
    }
}