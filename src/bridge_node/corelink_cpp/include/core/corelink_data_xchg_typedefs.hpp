#pragma once

#include "commons/base_includes.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            /**
             * Corelink client channel ID type
             */
            using channel_id_type = uint64_t;
            /**
             * On send function parameter prototype
             */
            using on_send_type_1_params = void(channel_id_type, size_t);
            /**
             * On receive function parameter prototype
             */
            using on_receive_type_1_params = void(channel_id_type, std::vector<uint8_t> &);
            /**
             * On error function parameter prototype
             */
            using on_error_type_1_params = void(channel_id_type, const std::string &);
            /**
             * On initialise/ un-initialise function parameter prototype
             */
            using on_init_type_1_params = void(channel_id_type);
            /**
             * @brief on init callback function typedef
             * @details The alias defines a callback function type that is used to declare on_init functions.
             * Functions with this type will be called typically once in the lifetime of a channel when the channel is initialised
             */
            using on_init_type_1 = std::function<on_init_type_1_params>;
            /**
             * @brief on init callback function typedef
             * @details The alias defines a callback function type that is used to declare on_uninit functions.
             * Functions with this type will be called typically once in the lifetime of a channel when the channel is uninitialised
             */
            using on_uninit_type_1 = std::function<on_init_type_1_params>;
            /**
             * @brief on send callback function typedef
             * @details This alias defines a callback function type that is used to declare on_send functions
             * Functions with this type will be called when a comm protocol manager sends data successfully
             */
            using on_send_type_1 = std::function<on_send_type_1_params>;
            /**
             * @brief on receive callback function typedef
             * @details This alias defines a callback function type that is used to declare on_receive functions
             * Functions with this type will be called when a comm protocol manager receives data successfully
             */
            using on_receive_type_1 = std::function<on_receive_type_1_params>;
            /**
             * @brief on error callback function typedef
             * @details This alias defines a callback function type that is used to declare on_error functions
             * Functions with this type will be called when a comm protocol manager encounters an error on a specific connection
             */
            using on_error_type_1 = std::function<on_error_type_1_params>;
        }
    }
}