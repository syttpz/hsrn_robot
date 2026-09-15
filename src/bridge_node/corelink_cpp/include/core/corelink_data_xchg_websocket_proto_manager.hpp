#pragma once

#ifdef CORELINK_USE_WEBSOCKET

#include "utils/concurrent_queue.hpp"
#include "corelink_data_xchg_ip_proto_base.hpp"
#include "libwebsockets.h"

// this is only in place because the Windows version of LWS does not seem to define this macro. at least
// in the package fetched from vcpkg
#ifndef LWS_PROTOCOL_LIST_TERM
#define LWS_PROTOCOL_LIST_TERM { NULL, NULL, 0, 0, 0, NULL, 0 }
#endif

namespace corelink
{
    namespace core
    {
        namespace network
        {
            class corelink_data_xchg_websocket_protocol_manager; // <-- forward declaration. used in the following lines

            extern "C"
            {
            int event_callback(ptr<lws> wsi,
                               lws_callback_reasons reason,
                               ptr<void> user,
                               ptr<void> in,
                               size_t len);
            }

            static void set_your_ws(
                    clvref<std::shared_ptr<corelink_data_xchg_websocket_protocol_manager>> ptr
            ); // <-- forward declaration.

            /**
             * @struct websocket_protocol_per_channel_descriptor
             * @brief Websocket channel descriptor. Struct mostly contains information initialized to make connections and store callbacks
             * in case of events like send, on receive etc. This is entirely user defined
             */
            struct CORELINK_EXPORT websocket_protocol_per_channel_descriptor
                    : public ip_protocol_channel_descriptor
            {
            private:

                friend class corelink_data_xchg_websocket_protocol_manager;

                friend int
                event_callback(ptr<lws> wsi, lws_callback_reasons reason, ptr<void> user, ptr<void> in, size_t len);

                /**
                 * @brief DO NOT ACCESS THIS
                 */
                ptr<lws> client_wsi = nullptr;
                /**
                 * @brief DO NOT ACCESS THIS
                 */
                bool initiate_client_connection_shutdown = false;
                /**
                 * @brief Path to the ca-cert.pem file
                 */
                std::string certificate_path;
            public:

                explicit websocket_protocol_per_channel_descriptor(in<std::string>
                                                                   cert_path);

                websocket_protocol_per_channel_descriptor(in<websocket_protocol_per_channel_descriptor> rhs);

                websocket_protocol_per_channel_descriptor(
                        rvref<websocket_protocol_per_channel_descriptor> rhs) noexcept;

                ~websocket_protocol_per_channel_descriptor() = default;
            };


            class CORELINK_EXPORT corelink_data_xchg_websocket_protocol_manager
                    : public corelink_data_xchg_ip_proto_base
            {
                enum class client_events
                {
                    create_client,
                    send_data,
                    close_connection
                };

                std::array<lws_protocols, 2> m_protocols{};
            public:
                /**
                 * @brief enumeration to define how data should be transported across on the websocket. essentially, the
                 * encoding the websocket library should choose to send this data.
                 */
                enum class data_traits
                {
                    bin,
                    text
                };

            private:
                friend int event_callback(ptr<lws>, lws_callback_reasons, ptr<void>, ptr<void>, size_t);

                ptr<lws_context> m_context;
                lws_context_creation_info m_context_create_info{};
                bool m_shutdown{};
                corelink_thread m_ws_context_runner;
                std::shared_ptr<corelink::utils::containers::concurrent_queue<
                        std::tuple<channel_id_type, data_traits, size_t>>> m_data_queue;
                std::shared_ptr<corelink::utils::containers::concurrent_queue<
                        std::tuple<channel_id_type, client_events>>> m_event_queue;

                inline void setup_protocols();

                bool make_context();

                void run();

            public:
                std::shared_ptr<websocket_protocol_per_channel_descriptor> get_channel_descriptor_by_wsi(lws *wsi);

                corelink_data_xchg_websocket_protocol_manager();

                corelink_data_xchg_websocket_protocol_manager(lvref<corelink_data_xchg_websocket_protocol_manager> rhs);

                corelink_data_xchg_websocket_protocol_manager(
                        rvref<corelink_data_xchg_websocket_protocol_manager> rhs) noexcept;

                ~corelink_data_xchg_websocket_protocol_manager() = default;

                bool start_context_manager();

                bool init(channel_id_type channel_id);

                void teardown(channel_id_type channel_id);

                void send_data(std::vector<uint8_t> &&data, channel_id_type channel_id);

                /**
                 * send data via the websocket with specified encoding
                 * @param data buffered data
                 * @param trait encoding trait
                 */
                void
                send_data(std::vector<uint8_t> &&data, channel_id_type channel_id, data_traits trait);

                // I hate writing this. But what happens is that LWS loop thread waits on poll timeout,
                // as a result, operations get delayed till the time the thread comes out of lws_service()
                // This is an external push to make the thread leave the service function.
                // Especially in reader type situations.
                void nudge();
            };

            /**
             * DO NOT ACCESS THIS.
             */
            static std::shared_ptr<corelink_data_xchg_websocket_protocol_manager> p_ws_proto_mgr;

            /**
             * DO NOT ACCESS THIS.
             */
            static inline void set_your_ws(clvref<std::shared_ptr<corelink_data_xchg_websocket_protocol_manager>> ptr)
            {
                p_ws_proto_mgr = ptr;
            }
        }
    }
}
#endif