#pragma once

#include "commons/string_includes.hpp"
#include "commons/platform_macros.hpp"
#include "corelink_network_constants.hpp"

namespace corelink
{
    namespace client
    {
        struct CORELINK_EXPORT corelink_client_connection_info
        {
            /**
             * @brief Corelink default remote hostname
             */
            static const_ptr_to_const_val<char> CORELINK_REMOTE_HOSTNAME;
            /**
             * @brief Corelink default TCP control port
             */
            static constexpr uint16_t TCP_CONTROL_PORT = 20010;
            /**
             * @brief Corelink default Websocket control port
             */
            static constexpr uint16_t WEBSOCKET_CONTROL_PORT = 20012;
            /**
             * @brief Corelink default username
             */
            static const_ptr_to_const_val<char> DEFAULT_USERNAME;
            /**
             * @brief Corelink default password
             */
            static const_ptr_to_const_val<char> DEFAULT_PASSWORD;
            /**
             * @brief Corelink server connecting user. Will be defaulted if left empty
             */
            std::string username = DEFAULT_USERNAME;
            /**
             * @brief Corelink server connecting user's password. Will be defaulted if left empty
             */
            std::string password = DEFAULT_PASSWORD;
            /**
             * @brief IP address of the host
             */
            std::string endpoint = CORELINK_REMOTE_HOSTNAME;
            /**
             * @brief Port number of the remote machine
             */
            uint16_t port_number = WEBSOCKET_CONTROL_PORT;
            //"/Users/sarthaktickoo/Work/hsrn/corelink/repos/corelink/config/ca-crt.pem"
            /**
             * @brief Client certificate path for TLS based protocols. Currently used for Websocket secure
             */
            std::string client_certificate_path = "./ca-crt.pem";
            /**
             * @brief Communication protocol to use. By default set to Websockets
             */
            clvref<core::network::constants::protocols::protocol> protocol;

            /**
             * Default constructor. it defaults the protocol to Websocket if enabled. otherwise TCP is used
             */
            corelink_client_connection_info() : protocol(
#ifdef CORELINK_USE_WEBSOCKET
                    core::network::constants::protocols::websocket
#else
                    core::network::constants::protocols::tcp
#endif
            )
            {}

            /**
             * parameterized constructor
             * @param proto network protocol to use
             */
            CORELINK_CPP_ATTR_MAYBE_UNUSED explicit corelink_client_connection_info(
                    in<core::network::constants::protocols::protocol> proto)
                    : protocol(proto)
            {
                switch (protocol())
                {
#ifdef CORELINK_USE_WEBSOCKET
                    case core::network::constants::protocols::websocket():
                        set_port_number(WEBSOCKET_CONTROL_PORT);
                        break;
#endif
#ifdef CORELINK_USE_TCP
                    case core::network::constants::protocols::tcp():
                        set_port_number(TCP_CONTROL_PORT);
                        break;
#endif
                }
            }

            /**
             * Copy constructor
             * @param rhs object to copy from
             */
            corelink_client_connection_info(lvref<corelink_client_connection_info> rhs)
                    : username(rhs.username),
                      password(rhs.password),
                      endpoint(rhs.endpoint),
                      port_number(rhs.port_number),
                      client_certificate_path(rhs.client_certificate_path),
                      protocol(rhs.protocol)
            {}

            /**
             * Move constructor
             * @param rhs object to move
             */
            corelink_client_connection_info(rvref<corelink_client_connection_info> rhs) noexcept
                    : username(std::move(rhs.username)),
                      password(std::move(rhs.password)),
                      endpoint(std::move(rhs.endpoint)),
                      port_number(rhs.port_number),
                      client_certificate_path(std::move(rhs.client_certificate_path)),
                      protocol(rhs.protocol)
            {}

            /**
             * Sets the username on the current object. This function can be chained with other functions
             * @param _ username
             * @return self
             */
            inline out<corelink_client_connection_info> set_username(in<std::string> _)
            {
                this->username = _;
                return *this;
            }

            /**
             * Sets the password on the current object. This function can be chained with other functions
             * @param _ password
             * @return self
             */
            inline out<corelink_client_connection_info> set_password(in<std::string> _)
            {
                this->password = _;
                return *this;
            }

            /**
             * Sets the host name on the current object. This function can be chained with other functions
             * @param _ host name
             * @return self
             */
            inline out<corelink_client_connection_info> set_endpoint(in<std::string> _)
            {
                this->endpoint = _;
                return *this;
            }

            /**
             * Sets the port number on the current object. This function can be chained with other functions
             * @param _ port number
             * @return self
             */
            inline out<corelink_client_connection_info> set_port_number(uint16_t _)
            {
                this->port_number = _;
                return *this;
            }

            /**
             * Sets the certificate path on the current object. This function can be chained with other functions
             * @param _ certificate path
             * @return self
             */
            inline out<corelink_client_connection_info> set_certificate_path(in<std::string> _)
            {
                this->client_certificate_path = _;
                return *this;
            }
        };

        inline const_ptr_to_const_val<char> corelink_client_connection_info::CORELINK_REMOTE_HOSTNAME = "corelink.hsrn.nyu.edu";
        inline const_ptr_to_const_val<char> corelink_client_connection_info::DEFAULT_USERNAME = "Testuser";
        inline const_ptr_to_const_val<char> corelink_client_connection_info::DEFAULT_PASSWORD = "Testpassword";
    }
}