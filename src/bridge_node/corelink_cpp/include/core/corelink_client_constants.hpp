#pragma once

#include "commons/base_includes.hpp"

namespace corelink
{
    namespace client
    {
        namespace constants
        {
            /**
             * @typedef corelink_stream_id_type
             * @brief defines a type for the stream ID returned by the corelink server
             */
            using corelink_stream_id_type = int32_t;
            /**
             * @enum corelink_component
             * @brief defines a list of corelink peer components which will be used in the future.
             */
            CORELINK_EXPORT enum class corelink_component
            {
                /**
                 * @brief corelink monolithic service
                 */
                corelink_service,
                /**
                 * @brief corelink control service. currently points to @ref corelink_service
                 */
                control_service = corelink_service,
                /**
                 * @brief corelink relay service. currently points to @ref corelink_service
                 */
                relay_service = corelink_service,
                /**
                 * @brief corelink diagnostic and MF service. most likely will denote pointing to the
                 * control service through heartbeat APIs, but currently does nothing
                 */
                diagnostic_service = corelink_service,
                /**
                 * @brief corelink plugin services. most likely will denote pointing to a specific
                 * plugin service through control APIs, but currently does nothing
                 */
                plugin_service = corelink_service,
                /**
                 * @brief external services. most likely will denote pointing to a specific
                 * external service, but currently does nothing
                 */
                external_service = corelink_service,
                /**
                 * @brief peer client. most likely will denote pointing to a set of peer corelink clients through
                 * control APIs, but currently does nothing
                 */
                peer_client = corelink_service
            };
        }
    }
}