#pragma once

#include "commons/base_includes.hpp"
#include "corelink_client_constants.hpp"
#include "corelink_network_constants.hpp"
#include "corelink_data_xchg_typedefs.hpp"
#include "utils/json.hpp"

namespace corelink
{
    namespace client
    {
        // forward declaration
        class corelink_classic_client;

        /**
         * @brief Corelink client channels base descriptor. Defines and implements base attributes
         * for all corelink client channels
         */
        struct CORELINK_EXPORT corelink_client_channel_base_descriptor
        {
            /**
             * @brief this attribute describes the corelink channel endpoint type. In essence this can control the
             * behaviour of the channel
             */
            CORELINK_CPP_ATTR_MAYBE_UNUSED
            constants::corelink_component endpoint_type =
                    constants::corelink_component::corelink_service;

            /**
             * @brief Defines the protocol in use by the current corelink client channel
             */
            const core::network::constants::protocols::protocol &protocol;

            /**
             * @brief constructor
             * @param p the protocol in use by the client channel
             * @param ep_type corelink service endpoint type. please refer to corelink::client::constants::corelink_component
             */
            explicit corelink_client_channel_base_descriptor(
                    const core::network::constants::protocols::protocol &p,
                    constants::corelink_component ep_type = constants::corelink_component::corelink_service)
                    : endpoint_type(ep_type),
                      protocol(p)
            {}

            /**
             * @brief Copy constructor
             * @param rhs Object to copy from
             */
            corelink_client_channel_base_descriptor(clvref<corelink_client_channel_base_descriptor> rhs) :
                    endpoint_type(rhs.endpoint_type),
                    protocol(rhs.protocol)
            {}

            /**
             * @brief Move constructor
             * @param rhs Object to move attributes from
             */
            corelink_client_channel_base_descriptor(corelink_client_channel_base_descriptor &&rhs) noexcept:
                    endpoint_type(rhs.endpoint_type),
                    protocol(rhs.protocol)
            {}

            /**
             * @brief Destructor
             */
            ~corelink_client_channel_base_descriptor() = default;
        };

        /**
         * @brief This class houses all the attributes relevant to a corelink control channel
         */
        struct CORELINK_EXPORT corelink_client_control_channel_descriptor
                : public corelink_client_channel_base_descriptor
        {
        public:
            /**
             * @brief stores the auth token associated with a corelink control channel
             */
            std::string auth_token;
            /**
             * @brief stores the public IP of the corelink client system
             */
            std::string client_ip;
            /**
             * @brief helps synchronize reading of multipart control messages
             */
            std::mutex response_stream_sync;
            /**
             * @brief stores/ buffers multipart control messages which maybe be incomplete and cannot be immediately issued
             */
            std::string response_stream;

            /**
             * @brief marks if the control channel is in use for sending a
             */
            bool channel_in_use = false;

            /**
             * @brief constructor
             * @param p the protocol in use by the client channel
             */
            explicit corelink_client_control_channel_descriptor(
                    const core::network::constants::protocols::protocol &p,
                    constants::corelink_component ep_type = constants::corelink_component::corelink_service)
                    : corelink_client_channel_base_descriptor(p, ep_type)
            {
            }

            /**
             * @brief Copy constructor
             * @param rhs instance to copy from
             */
            corelink_client_control_channel_descriptor(const corelink_client_control_channel_descriptor &rhs)
                    : corelink_client_channel_base_descriptor(rhs),
                      auth_token(rhs.auth_token),
                      client_ip(rhs.client_ip),
                      channel_in_use(rhs.channel_in_use)
            {}

            /**
             * @brief Move constructor
             * @param rhs instance to move from
             */
            corelink_client_control_channel_descriptor(corelink_client_control_channel_descriptor &&rhs) noexcept
                    : corelink_client_channel_base_descriptor(std::move(rhs)),
                      auth_token(std::move(rhs.auth_token)),
                      client_ip(std::move(rhs.client_ip)),
                      channel_in_use(rhs.channel_in_use)
            {}
        };

        /**
         * @brief Data channel descriptor
         */
        class CORELINK_EXPORT corelink_client_data_channel_descriptor : public corelink_client_channel_base_descriptor
        {
        private:
            friend class corelink::client::corelink_classic_client;

            /**
             * @brief used to stage partial buffers received from the server. Not to be used directly
             */
            std::vector<uint8_t> incomplete_packet_buffer;

        public:
            /**
             * @brief Corelink server assigned stream ID
             */
            constants::corelink_stream_id_type stream_id{};
            /**
             * @brief Maximum Segment Size supported by the corelink server and network
             */
            uint32_t max_tx_unit{};
            /**
             * @brief stream list in case of a receiver stream
             */
            std::vector<constants::corelink_stream_id_type> stream_list;
            /**
             * @brief user data reception handler
             */
            std::function<
                    void(core::network::channel_id_type,
                         in<constants::corelink_stream_id_type>,
                         in<utils::json>,
                         lvref<std::vector<uint8_t>>
                    )>
                    user_receive_handler = nullptr;

            /**
             * @brief Constructor
             * @param p instance to copy from
             */
            explicit corelink_client_data_channel_descriptor(
                    clvref<core::network::constants::protocols::protocol> p,
                    constants::corelink_component ep_type = constants::corelink_component::corelink_service)
                    : corelink_client_channel_base_descriptor(p, ep_type)
            {
            }

            /**
             * @brief Copy constructor
             * @param rhs instance to copy from
             */
            corelink_client_data_channel_descriptor(clvref<corelink_client_data_channel_descriptor> rhs)
                    : corelink_client_channel_base_descriptor(rhs),
                      stream_id(rhs.stream_id),
                      max_tx_unit(rhs.max_tx_unit),
                      stream_list(rhs.stream_list),
                      user_receive_handler(rhs.user_receive_handler)
            {}

            /**
             * @brief Move constructor
             * @param rhs instance to move from
             */
            corelink_client_data_channel_descriptor(rvref<corelink_client_data_channel_descriptor> rhs) noexcept
                    : corelink_client_channel_base_descriptor(std::move(rhs)),
                      stream_id(rhs.stream_id),
                      max_tx_unit(rhs.max_tx_unit),
                      stream_list(std::move(rhs.stream_list)),
                      user_receive_handler(std::move(rhs.user_receive_handler))
            {}
        };
    }
}