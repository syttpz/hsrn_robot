#pragma once

#include "commons/base_includes.hpp"
#include "commons/associative_containers_includes.hpp"
#include "commons/reader_writer_lock_shim.hpp"
#include "utils/json.hpp"
#include "utils/concurrent_queue.hpp"
#include "utils/system.hpp"
#include "corelink_data_xchg_udp_proto_manager.hpp"
#include "corelink_data_xchg_tcp_proto_manager.hpp"
#include "corelink_data_xchg_websocket_proto_manager.hpp"
#include "corelink_client_request_response_payload_prototypes.hpp"
#include "corelink_client_channel_descriptors.hpp"
#include "corelink_client_request_response_handlers.hpp"
#include "corelink_client_connection_info.hpp"

namespace corelink
{
    namespace client
    {
        class CORELINK_EXPORT corelink_classic_client
        {
        private:
            struct CORELINK_EXPORT request_completion_pack
            {
                core::network::channel_id_type channel_id{0};
                corelink_functions function;
                request_response_handler request_response_handler_descriptor{};
                std::shared_ptr<request_response::requests::corelink_server_request_base> request{};
                request_response_handler::user_completion_handler_type completion_handler{};

                request_completion_pack()
                {}

                request_completion_pack(clvref<request_completion_pack> rhs) :
                        channel_id(rhs.channel_id), function(rhs.function),
                        request_response_handler_descriptor(rhs.request_response_handler_descriptor),
                        request(rhs.request),
                        completion_handler(rhs.completion_handler)
                {}

                request_completion_pack(rvref<request_completion_pack> rhs) noexcept:
                        channel_id(rhs.channel_id), function(rhs.function),
                        request_response_handler_descriptor(std::move(rhs.request_response_handler_descriptor)),
                        request(std::move(rhs.request)),
                        completion_handler(std::move(rhs.completion_handler))
                {}
            };

            /**
             * @brief All active protocol managers
             */
            std::unordered_map<core::network::constants::protocols::protocol::bit_mask_pos_type,
                    std::shared_ptr<core::network::corelink_data_xchg_protocol>> m_protocol_managers{};

            /**
             * @brief Corelink client channels
             */
            std::unordered_map<core::network::channel_id_type,
                    std::shared_ptr<corelink_client_channel_base_descriptor>> m_corelink_channels{};

#if defined(CORELINK_USE_TCP) || defined(CORELINK_USE_UDP)
            /**
             * @brief Corelink raw socket protocol context managers
             */
            std::shared_ptr<core::network::corelink_data_xchg_raw_socket_protocol_context_manager> m_context_manager{};
#endif

            /**
             * @brief Request response completion handlers
             */
            std::unordered_map<std::string, request_completion_pack> m_completion_handlers{};

            utils::containers::concurrent_queue<request_completion_pack> m_outbound_control_msg_queue;

            void handle_authenticate(core::network::channel_id_type channel_id,
                                     in<utils::json> response,
                                     in<request_completion_pack> rq_rp_params
            )
            {
                auto response_msg = std::make_shared<request_response::responses::corelink_server_response_base>();
                response_msg->message = response.get_str("message");
                response_msg->status_code = response.get_int("statusCode", -1);

                if (response_msg->status_code == 0)
                {
                    auto control_channel_descriptor = std::static_pointer_cast<corelink_client_control_channel_descriptor>(
                            m_corelink_channels.at(channel_id));
                    control_channel_descriptor->auth_token = response()["token"].GetString();
                    control_channel_descriptor->client_ip = response()["IP"].GetString();
                }

                if (rq_rp_params.completion_handler)
                    rq_rp_params.completion_handler(channel_id, "", response_msg);
            }

            void handle_create_sender(core::network::channel_id_type /*channel_id*/,
                                      in<utils::json> response,
                                      in<request_completion_pack> rq_rp_params,
                                      in<std::string> remote_server_ep)
            {
                auto mod_data_stream_req =
                        std::static_pointer_cast<request_response::requests::modify_data_stream_request_base>(
                                rq_rp_params.request);
                auto proto_manager = m_protocol_managers[mod_data_stream_req->protocol()];
                corelink_client_data_channel_descriptor data_channel_descriptor(mod_data_stream_req->protocol);
                data_channel_descriptor.stream_id = response.get_int("streamID");
                data_channel_descriptor.max_tx_unit = response.get_int("MTU");

                std::shared_ptr<core::network::ip_protocol_channel_descriptor> descriptor;
                switch (mod_data_stream_req->protocol())
                {
#ifdef CORELINK_USE_WEBSOCKET
                    case core::network::constants::protocols::websocket():
                    {

                        descriptor = std::make_shared<core::network::websocket_protocol_per_channel_descriptor>(
                                mod_data_stream_req->client_certificate_path);
                        break;
                    }
#endif
#ifdef CORELINK_USE_TCP
                    case core::network::constants::protocols::tcp():
                    {
                        descriptor = std::make_shared<core::network::tcp_protocol_per_channel_descriptor>();
                        break;
                    }
#endif
#ifdef CORELINK_USE_UDP
                    case core::network::constants::protocols::udp():
                    {
                        descriptor = std::make_shared<core::network::udp_protocol_per_channel_descriptor>();
                        break;
                    }
#endif
                    default:
                        break;
                }
                descriptor->on_init = mod_data_stream_req->on_init;
                descriptor->on_uninit = mod_data_stream_req->on_uninit;
                auto sender_stream_req = std::static_pointer_cast<request_response::requests::modify_sender_stream_request>(
                        mod_data_stream_req);
                descriptor->on_send = sender_stream_req->on_send;
                descriptor->on_receive = nullptr;
                descriptor->port_number = response.get_int("port");
                descriptor->endpoint = remote_server_ep.empty() ? response.get_str("IP") : remote_server_ep;
                descriptor->on_error = mod_data_stream_req->on_error;
                core::network::channel_id_type data_channel_channel_id;
                std::dynamic_pointer_cast<core::network::corelink_data_xchg_ip_proto_base>(
                        proto_manager)->add_and_init_channel(std::move(descriptor), data_channel_channel_id);

                m_corelink_channels.insert(
                        {
                                data_channel_channel_id,
                                std::make_shared<corelink_client_data_channel_descriptor>(
                                        std::move(data_channel_descriptor))
                        });

                if (rq_rp_params.completion_handler)
                {
                    rq_rp_params.completion_handler(data_channel_channel_id, "", nullptr);
                }
            }

            void handle_create_receiver(core::network::channel_id_type /*channel_id*/,
                                        in<utils::json> response,
                                        in<request_completion_pack> rq_rp_params,
                                        in<std::string> remote_server_ep)
            {
                auto receiver_stream_req =
                        std::static_pointer_cast<request_response::requests::modify_receiver_stream_request>(
                                rq_rp_params.request);
                corelink_client_data_channel_descriptor data_channel_descriptor(receiver_stream_req->protocol);
                auto proto_manager = m_protocol_managers[receiver_stream_req->protocol()];
                data_channel_descriptor.stream_id = response.get_int("streamID");
                data_channel_descriptor.max_tx_unit = response.get_int("MTU");

                if (response().HasMember("streamList"))
                {
                    auto modified_streams = response()["streamList"].GetArray();
                    for (auto &stream: modified_streams)
                    {
                        data_channel_descriptor.stream_list.emplace_back(
                                stream.HasMember("streamID") ? stream["streamID"].GetInt() : 0);
                    }
                }

                std::shared_ptr<core::network::ip_protocol_channel_descriptor> descriptor;
                switch (receiver_stream_req->protocol())
                {
#ifdef CORELINK_USE_WEBSOCKET
                    case core::network::constants::protocols::websocket():
                    {
                        descriptor = std::make_shared<core::network::websocket_protocol_per_channel_descriptor>(
                                receiver_stream_req->client_certificate_path);
                        break;
                    }
#endif
#ifdef CORELINK_USE_TCP
                    case core::network::constants::protocols::tcp():
                    {
                        descriptor = std::make_shared<core::network::tcp_protocol_per_channel_descriptor>(true);
                        break;
                    }
#endif
#ifdef CORELINK_USE_UDP
                    case core::network::constants::protocols::udp():
                    {
                        descriptor = std::make_shared<core::network::udp_protocol_per_channel_descriptor>(true);
                        break;
                    }
#endif
                    default:
                        break;
                }
                descriptor->on_send = nullptr;
                data_channel_descriptor.user_receive_handler = receiver_stream_req->on_receive;
                auto self = this;
                descriptor->on_receive = [self]
                        (core::network::channel_id_type receiving_channel_id,
                         out<std::vector<uint8_t>> stream_data)
                {
                    auto channel = self->m_corelink_channels.find(receiving_channel_id);
                    if (channel == self->m_corelink_channels.end())
                        return; // if we cannot find the channel, we cannot do anything. return

                    auto data_channel_dsc = std::static_pointer_cast<corelink_client_data_channel_descriptor>(
                            channel->second);
                    bool can_have_multipart_frame = false;
#ifdef CORELINK_USE_TCP
                    can_have_multipart_frame =
                            data_channel_dsc->protocol == core::network::constants::protocols::tcp;
#endif

                    // drop packet when protocal is not TCP, packet size less than 8 bytes
                    // is essentially malformed packet
                    if (!can_have_multipart_frame && stream_data.size() < 8)
                        return;

                    std::vector<uint8_t> &buffer = can_have_multipart_frame ?
                                                   data_channel_dsc->incomplete_packet_buffer : stream_data;

                    if (can_have_multipart_frame)
                    {
                        buffer.insert(buffer.end(), stream_data.begin(), stream_data.end());
                    }

                    constants::corelink_stream_id_type stream_id = 0;
                    uint16_t header_size = 0, data_size = 0;
                    size_t pkt_pos = 0;

                    while (buffer.size() - pkt_pos >= 8)
                    {
                        header_size = corelink::utils::system::from_bytes<
                                decltype(header_size),
                                utils::system::endianness::little>
                                (std::vector<uint8_t>(
                                         buffer.begin() + pkt_pos,
                                         buffer.begin() + pkt_pos + 2
                                 )
                                );
                        data_size = corelink::utils::system::from_bytes<
                                decltype(data_size),
                                utils::system::endianness::little>
                                (std::vector<uint8_t>(
                                         buffer.begin() + pkt_pos + 2,
                                         buffer.begin() + pkt_pos + 4
                                 )
                                );

                        stream_id = corelink::utils::system::from_bytes<
                                decltype(stream_id),
                                utils::system::endianness::little>
                                (std::vector<uint8_t>(
                                         buffer.begin() + pkt_pos + 4,
                                         buffer.begin() + pkt_pos + 8
                                 )
                                );

                        if ((pkt_pos + 8 + header_size + data_size) > buffer.size())
                            break;

                        std::string header_str;
                        if ((header_size > 0))
                        {
                            header_str = std::string(buffer.begin() + pkt_pos + 8,
                                                     buffer.begin() + pkt_pos + 8 + header_size);
                        }

                        std::vector<uint8_t> data_buff;
                        if (data_size > 0)
                        {
                            data_buff.insert(data_buff.end(),
                                             (buffer.begin() + pkt_pos + 8 + header_size),
                                             buffer.begin() + pkt_pos + 8 + header_size + data_size);
                        }

                        // call the user defined receive function
                        if (data_channel_dsc->user_receive_handler)
                        {
                            // call the user callback
                            data_channel_dsc->user_receive_handler(
                                    receiving_channel_id,
                                    stream_id,
                                    utils::json(header_str),
                                    data_buff
                            );
                        }
                        if (!can_have_multipart_frame) break;

                        // set the packet pos to next packet
                        pkt_pos += 8 + header_size + data_size;
                    }

                    if (can_have_multipart_frame)
                    {
                        buffer.erase(buffer.begin(), buffer.begin() + pkt_pos);
                    }
                };

                descriptor->port_number = response.get_int("port");
                descriptor->endpoint = remote_server_ep.empty() ? response.get_str("IP") : remote_server_ep;
                descriptor->on_error = receiver_stream_req->on_error;
                descriptor->on_init = [receiver_stream_req, self]
                        (core::network::channel_id_type ch_id)
                {
                    // ping the server
                    self->send_data(ch_id, std::vector<uint8_t>(), utils::json());
                    if (receiver_stream_req->on_init)
                        receiver_stream_req->on_init(ch_id);
                };
                descriptor->on_uninit = receiver_stream_req->on_uninit;
                core::network::channel_id_type data_channel_channel_id;
                if (!std::dynamic_pointer_cast<core::network::corelink_data_xchg_ip_proto_base>(
                        proto_manager)->add_and_init_channel(std::move(descriptor), data_channel_channel_id))
                {
                    rq_rp_params.completion_handler(data_channel_channel_id, "Failed to create data channel!", nullptr);
                    return;
                }
                m_corelink_channels.insert(
                        {
                                data_channel_channel_id,
                                std::make_shared<corelink_client_data_channel_descriptor>(
                                        std::move(data_channel_descriptor))
                        });
                if (rq_rp_params.completion_handler)
                {
                    rq_rp_params.completion_handler(data_channel_channel_id, "", nullptr);
                }
            }

            void destroy()
            {
                for (auto &proto_mgr: m_protocol_managers)
                {
                    std::dynamic_pointer_cast<core::network::corelink_data_xchg_ip_proto_base>(
                            proto_mgr.second)->teardown_all();
                }
#if defined(CORELINK_USE_TCP) || defined (CORELINK_USE_UDP)
                m_context_manager->stop_context_manager();
#endif
            }

        public:
            corelink_classic_client() = default;

            corelink_classic_client(clvref<corelink_classic_client>) = delete;

            corelink_classic_client(rvref<corelink_classic_client>) = delete;

            out<corelink_classic_client> operator=(rvref<corelink_classic_client>) = delete;

            out<corelink_classic_client> operator=(clvref<corelink_classic_client>) = delete;

            /**
             * Get corelink control or data channel parameters
             * @param channel_id channel ID
             * @return pointer to a channel if channel is present.
             */
            inline clvref<std::shared_ptr<corelink_client_channel_base_descriptor>>
            get_channel(core::network::channel_id_type channel_id) const
            {
                return m_corelink_channels.at(channel_id);
            }

            /**
             * @brief Initializes all the protocols that the client will use
             * @param protocols bitmask pattern exposed as part of protocols under core::network::constants::protocols::*
             * by default, tcp, udp are init. if websocket is enabled, then that will be included too.
             * @return true if protocols are initialized correctly. false otherwise.
             */
            bool init_protocols(
                    in<core::network::constants::protocols::protocol::bit_mask_type> protocols =
                    0
                    #ifdef CORELINK_USE_TCP
                    | (1 << core::network::constants::protocols::tcp())
                    #endif
                    #ifdef CORELINK_USE_UDP
                    | (1 << core::network::constants::protocols::udp())
                    #endif
                    #ifdef CORELINK_USE_WEBSOCKET
                    | (1 << core::network::constants::protocols::websocket())
#endif
            )
            {
                if (protocols == 0)
                    return false;

                // check if it is a raw socket protocol from ASIO C++
                // if yes, then switch on the ASIO context manager "service"

                if (false
                    #if defined (CORELINK_USE_TCP)
                    || protocols.test(core::network::constants::protocols::tcp())
                    #endif
                    #if defined (CORELINK_USE_UDP)
                    || protocols.test(core::network::constants::protocols::udp())
#endif
                        )
                {
                    // create context manager
                    m_context_manager = std::make_shared<core::network::corelink_data_xchg_raw_socket_protocol_context_manager>();
                    // start it
                    m_context_manager->start_context_manager();
                }

#if defined(CORELINK_USE_TCP)
                // check if we want to use TCP protocol
                if (protocols.test(core::network::constants::protocols::tcp()))
                {
                    m_protocol_managers.insert(
                            {
                                    core::network::constants::protocols::tcp(),
                                    std::make_shared<core::network::corelink_data_xchg_tcp_protocol_manager>(
                                            m_context_manager)
                            });
                }
#endif
#if defined(CORELINK_USE_UDP)
                // check if we want to use the UDP protocol
                if (protocols.test(core::network::constants::protocols::udp()))
                {
                    m_protocol_managers.insert(
                            {
                                    core::network::constants::protocols::udp(),
                                    std::make_shared<core::network::corelink_data_xchg_udp_protocol_manager>(
                                            m_context_manager)
                            });
                }
#endif
#ifdef CORELINK_USE_WEBSOCKET
                // check if we want to use the Websocket protocol.
                if (protocols.test(core::network::constants::protocols::websocket()))
                {

                    auto ws_proto_manager = std::make_shared<core::network::corelink_data_xchg_websocket_protocol_manager>();
                    ws_proto_manager->start_context_manager();
                    m_protocol_managers.insert(
                            {
                                    core::network::constants::protocols::websocket(),
                                    std::move(ws_proto_manager)
                            });
                }
#endif

                return true;
            }


            inline core::network::channel_id_type add_control_channel(
                    in<client::corelink_client_connection_info> conn_info,
                    in<core::network::on_error_type_1> on_error = nullptr,
                    in<core::network::on_init_type_1> on_connection_init = nullptr,
                    in<core::network::on_uninit_type_1> on_connection_uninit = nullptr
            )
            {
                return add_control_channel(
                        conn_info.protocol,
                        conn_info.endpoint,
                        conn_info.port_number,
                        conn_info.client_certificate_path,
                        on_error,
                        on_connection_init,
                        on_connection_uninit
                );
            }

            inline core::network::channel_id_type add_control_channel(
                    in<core::network::constants::protocols::protocol> channel_protocol,
                    in<std::string> endpoint,
                    in<uint16_t> port,
                    in<std::string> cert_path = "",
                    in<core::network::on_error_type_1> on_error = nullptr,
                    in<core::network::on_init_type_1> on_connection_init = nullptr,
                    in<core::network::on_uninit_type_1> on_connection_uninit = nullptr
            )
            {
                // step 1: find the protocol using which we need to set up the control channel.
                auto proto_manager = std::dynamic_pointer_cast<core::network::corelink_data_xchg_ip_proto_base>(
                        m_protocol_managers[channel_protocol()]);

                if (!proto_manager && (on_error != nullptr))
                {
                    on_error(0,
                             "Error: Could not locate a proto manager for supplied control channel protocol\n Skipping creation of control channel");
                    return 0;
                }

                // step 2: construct the appropriate descriptor
                std::shared_ptr<corelink::core::network::ip_protocol_channel_descriptor> descriptor;
                core::network::channel_id_type channel_id = 0;
                switch (channel_protocol())
                {
#ifdef CORELINK_USE_WEBSOCKET
                    case core::network::constants::protocols::websocket():
                    {
                        descriptor = std::make_shared<corelink::core::network::websocket_protocol_per_channel_descriptor>(
                                cert_path);
                        break;
                    }
#endif
#ifdef CORELINK_USE_TCP
                    case core::network::constants::protocols::tcp():
                    {
                        descriptor = std::make_shared<corelink::core::network::tcp_protocol_per_channel_descriptor>(
                                true,
                                true);
                        break;
                    }
#endif
                    default:
                        descriptor = nullptr;
                        break;
                }

                if (descriptor == nullptr)
                {
                    throw std::runtime_error(
                            "The chosen protocol is not supported to construct a control channel");
                }
                // assign all the common parameters
                descriptor->port_number = port;
                descriptor->endpoint = endpoint;
                descriptor->on_init = on_connection_init;
                descriptor->on_error = on_error;
                descriptor->on_uninit = on_connection_uninit;
                // this is slightly involved, but I have tried to make it is as straightforward for a first write.
                // I am sure I can make this less verbose in a future version.
                auto self = this;
                descriptor->on_receive = [self](
                        core::network::channel_id_type channel_id,
                        in<std::vector<uint8_t>> data
                )
                {
                    if (data.empty())
                        return;

                    auto descriptor = std::static_pointer_cast<corelink_client_control_channel_descriptor>(
                            self->m_corelink_channels.at(channel_id));

                    auto channel_descriptor =
                            std::dynamic_pointer_cast<core::network::corelink_data_xchg_ip_proto_base>(
                                    self->m_protocol_managers[descriptor->protocol()])->get_channel(channel_id);

                    // the data that we got from the server is not guaranteed to be the whole message.
                    // or in fact it could be a mixture of multiple responses
                    // we collect it in a stream, and then, split it based on some obvious JSON parsing rules.

                    corelink::commons::exclusive_lock lock(descriptor->response_stream_sync);
                    std::string response_string(data.begin(), data.end());
                    descriptor->response_stream += response_string;

                    int16_t count = 0, current_idx = 0;
                    try
                    {
                        for (;; ++current_idx)
                        {
                            if (current_idx >=
                                static_cast<decltype(current_idx)>(descriptor->response_stream.size()))
                                break;
                            if (descriptor->response_stream[current_idx] == '{') ++count;
                            else if (descriptor->response_stream[current_idx] == '}') --count;

                            if (count == 0 && (descriptor->response_stream.find('{') != std::string::npos))
                            {
                                std::string complete_response_str =
                                        current_idx ==
                                        static_cast<decltype(current_idx)>(descriptor->response_stream.size() - 1) ?
                                        descriptor->response_stream :
                                        descriptor->response_stream.substr(0, current_idx + 1);

                                descriptor->response_stream.erase(0, current_idx + 1);
                                current_idx = -1;

                                // on receiving a response, the first step is to try and read the JSON response.
                                // This might be partial, but we need to make a try.
                                utils::json response(complete_response_str);
                                // locate a response handler
                                std::string handler_id = response.get_str("ID");
                                // check if this is a response to a request, or a server init function
                                if (handler_id.empty()) handler_id = response.get_str("function");
                                if (handler_id.empty()) return; // if handler is empty at this point that means there is something wrong. return

                                auto handler = self->m_completion_handlers.find(handler_id);
                                if (handler == self->m_completion_handlers.end())
                                {
                                    channel_descriptor->on_error(channel_id,
                                                                 "Could not locate a handler for the response sent by the server.");
                                    return;
                                }

                                switch (handler->second.function)
                                {
                                    case corelink_functions::create_sender:
                                    {
                                        self->handle_create_sender(
                                                channel_id,
                                                response,
                                                handler->second,
                                                channel_descriptor->endpoint
                                        );
                                        break;
                                    }
                                    case corelink_functions::create_receiver:
                                    {
                                        self->handle_create_receiver(
                                                channel_id,
                                                response,
                                                handler->second,
                                                channel_descriptor->endpoint
                                        );
                                        break;
                                    }
                                    case corelink_functions::authenticate:
                                    {
                                        self->handle_authenticate(
                                                channel_id,
                                                response,
                                                handler->second
                                        );
                                        break;
                                    }
                                    default:
                                    {
                                        if (handler->second.request_response_handler_descriptor.response_handler)
                                        {
                                            handler->second.request_response_handler_descriptor.response_handler(
                                                    channel_id,
                                                    response,
                                                    handler->second.completion_handler);
                                        }
                                        break;
                                    }
                                }

                                if (response().HasMember("ID"))
                                {
                                    self->m_completion_handlers.erase(handler_id);
                                    descriptor->channel_in_use = false;
                                }
                            }
                        }

                        if (!self->m_outbound_control_msg_queue.empty())
                        {
                            auto &req = self->m_outbound_control_msg_queue.peek();
                            self->request(req.channel_id, req.function, req.request, req.completion_handler);
                            self->m_outbound_control_msg_queue.pop();
                        }
                    }
                    catch (lvref<std::exception> ex)
                    {
                        channel_descriptor->on_error(
                                1,
                                std::string("Error while processing control channel message received: ") + ex.what()
                        );
                    }
                };

                proto_manager->add_and_init_channel(descriptor, channel_id);
                if (m_corelink_channels.find(channel_id) == m_corelink_channels.end())
                {
                    m_corelink_channels.insert(
                            {channel_id,
                             std::make_shared<corelink_client_control_channel_descriptor>(channel_protocol)});
                }

                return channel_id;
            }

            /**
             * @brief Initiate or setup a corelink request or handler for a specific type of a function
             * @param control_channel_channel_id which control server do you wish to send the data to
             * @param func corelink client or server function
             * @param request_params Request parameters. Please refer to the documentation to figure out the corresponding request
             * @param completion_handler Handler that will be called back upon completion of request
             * @param bypass_enqueue : DO NOT OVERRIDE THIS PARAMETER.
             */
            void request(
                    core::network::channel_id_type control_channel_channel_id,
                    corelink_functions func,
                    in<std::shared_ptr<request_response::requests::corelink_server_request_base>> request_params,
                    in<request_response_handler::user_completion_handler_type> completion_handler
            )
            {
                // locate the request response handler.
                auto function_descriptor = corelink_functions_request_response_handlers.find(func);
                if (function_descriptor == corelink_functions_request_response_handlers.end())
                    throw std::runtime_error("The requested function is invalid");

                // locate the control channel we wish to send a control message on
                auto control_channel_descriptor = std::static_pointer_cast<corelink_client_control_channel_descriptor>(
                        m_corelink_channels.at(control_channel_channel_id));
                // locate the protocol manager
                auto proto_manager = std::dynamic_pointer_cast<core::network::corelink_data_xchg_ip_proto_base>(
                        m_protocol_managers.at(control_channel_descriptor->protocol()));
                // locate the channel based on the
                auto channel_descriptor =
                        std::dynamic_pointer_cast<corelink::core::network::ip_protocol_channel_descriptor>(
                                proto_manager->get_channel(control_channel_channel_id));

                // setup data for managing the callback.
                request_completion_pack p;
                p.function = func;
                p.request_response_handler_descriptor = function_descriptor->second;
                p.request = request_params;
                p.completion_handler = completion_handler;
                p.channel_id = control_channel_channel_id;

                switch (func)
                {
                    case corelink_functions::server_callback_on_dropped:
                    {
                        m_completion_handlers.insert({"dropped", std::move(p)});
                        break;
                    }
                    case corelink_functions::server_callback_on_update:
                    {
                        m_completion_handlers.insert({"update", std::move(p)});
                        break;
                    }
                    case corelink_functions::server_callback_on_subscribed:
                    {
                        m_completion_handlers.insert({"subscriber", std::move(p)});
                        break;
                    }
                    case corelink_functions::server_callback_on_stale:
                    {
                        m_completion_handlers.insert({"stale", std::move(p)});
                        break;
                    }
                    default:
                    {
                        // Okay I detest the next 4 lines, however, currently this is the only solution I have, without opening
                        // multiple sockets
                        // this is specifically required for TCP based control channel, but I don't want
                        // because of 2 reasons. First, if you rapidly send out multiple requests, TCP write buffer doesn't flush
                        // and the kernel sometimes bunches messages together (c.f. Nagel's algorithm).
                        // As a result the server sometimes ignores our messages
                        // so what we do is build a request-response style mechanism in such a way that we only dispatch our next
                        // request when we get a response. It seems to work well without any significant drawbacks.
                        if (control_channel_descriptor->channel_in_use)
                        {
                            m_outbound_control_msg_queue.push(p);
                            return;
                        }

                        control_channel_descriptor->channel_in_use = true;
                        if (function_descriptor->second.request_handler)
                        {
                            // generate request data
                            auto request = function_descriptor->second.request_handler(control_channel_descriptor,
                                                                                       request_params);

                            // check if the request data has the ID. If not, add a randomly generated ID
                            std::string request_id;
                            if (!request().HasMember("ID"))
                            {
                                do
                                {
                                    request_id = std::to_string(utils::random_numbers::get_random_int());
                                } while (m_completion_handlers.find(request_id) != m_completion_handlers.end());

                                request.append("ID", request_id);
                            }
                            else
                            {
                                request_id = request()["ID"].GetString();
                            }
                            // generate the request string
                            in<std::string> request_string = request.to_string();
                            m_completion_handlers.insert({request_id, std::move(p)});
                            // post the request to the control channel protocol manager
                            proto_manager->send_data(
                                    std::vector<uint8_t>(request_string.begin(), request_string.end()),
                                    control_channel_channel_id
                            );
                        }
                        break;
                    }
                }
            }

            /**
             * @brief Send data through a data channel
             * @details
             *
             * @param data_channel_channel_id data channel channel ID. Obtained via the response handler for create_sender request.
             * @param data array (std::vector here) of uint8_t (unsigned char)
             * @param headers JSON headers to be sent along with your data packet
             */
            void send_data(
                    core::network::channel_id_type data_channel_channel_id,
                    rvref<std::vector<uint8_t>> packet,
                    rvref<utils::json> headers
            )
            {
                // locate the control channel we wish to send a control message on
                auto data_channel_descriptor = std::static_pointer_cast<corelink_client_data_channel_descriptor>(
                        m_corelink_channels.at(data_channel_channel_id));

                if (!data_channel_descriptor)
                {

                }
                // locate the protocol manager
                auto proto_manager = std::dynamic_pointer_cast<core::network::corelink_data_xchg_ip_proto_base>(
                        m_protocol_managers.at(data_channel_descriptor->protocol()));
                // locate the channel based on the
                auto channel_descriptor =
                        std::dynamic_pointer_cast<corelink::core::network::ip_protocol_channel_descriptor>(
                                proto_manager->get_channel(data_channel_channel_id));

                const std::string header_string = headers.to_string();
                const size_t data_size = packet.size();

                // if we try to send more data than the MTU, then flag the packet and drop it.
                if ((data_size + header_string.size() + 8) > data_channel_descriptor->max_tx_unit)
                {
                    if (channel_descriptor->on_error)
                    {
                        std::stringstream err;
                        err << "Unable to send data packet as MTU for this channel is "
                            << data_channel_descriptor->max_tx_unit
                            << ", whereas attempted to pass " << (data_size + header_string.size())
                            << " bytes of data excluding corelink headers.\n";
                        channel_descriptor->on_error(1, err.str());
                    }
                    return;
                }

                auto header_size_bytes = corelink::utils::system::to_bytes<
                        decltype(header_string.size()),
                        2,
                        corelink::utils::system::endianness::little>
                        (header_string.size());
                auto data_size_bytes = corelink::utils::system::to_bytes<
                        decltype(data_size),
                        2,
                        corelink::utils::system::endianness::little>
                        (data_size);

                auto stream_id_bytes = corelink::utils::system::to_bytes<
                        decltype(data_channel_descriptor->stream_id),
                        4,
                        corelink::utils::system::endianness::little>
                        (data_channel_descriptor->stream_id);


                // We are stacking elements. The order is
                // |2 bytes   |2 bytes |4 bytes  |header_len bytes |data_len bytes|
                // |header_len|data_len|stream_id|header           |data          |
                // All data has to be LE aligned.
                if (!header_string.empty())
                {
                    packet.insert(
                            packet.begin(),
                            std::make_move_iterator(header_string.begin()),
                            std::make_move_iterator(header_string.end())
                    );
                }
                // insert the stream ID
                packet.insert(packet.begin(), std::make_move_iterator(stream_id_bytes.begin()),
                              std::make_move_iterator(stream_id_bytes.end()));
                // insert data size
                packet.insert(packet.begin(), std::make_move_iterator(data_size_bytes.begin()),
                              std::make_move_iterator(data_size_bytes.end()));
                // insert header size
                packet.insert(packet.begin(), std::make_move_iterator(header_size_bytes.begin()),
                              std::make_move_iterator(header_size_bytes.end()));
                proto_manager->send_data(std::move(packet), data_channel_channel_id);
            }

            ~corelink_classic_client()
            {
                destroy();
            }
        };
    }
}
