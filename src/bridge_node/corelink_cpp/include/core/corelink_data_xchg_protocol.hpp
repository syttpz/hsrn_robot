#pragma once

#include "corelink_data_xchg_typedefs.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            /**
             * @brief The base class for all data exchange communication protocols.
             * This class is a generic interface. Right now it does not implement anything but rather just provides a base class
             * for Data exchange protocols. This is useful when we use non-internet based protocols like RS-232, I2C, etc.
             */
            class CORELINK_EXPORT corelink_data_xchg_protocol
                    : public std::enable_shared_from_this<corelink_data_xchg_protocol>
            {
                // LEAVE THIS EMPTY FOR NOW
            public:
                /**
                 * @brief default constructor
                 */
                corelink_data_xchg_protocol() = default;

                /**
                 * @brief copy constructor
                 */
                corelink_data_xchg_protocol(const corelink_data_xchg_protocol &) = default;

                /**
                 * @brief Move constructor
                 */
                corelink_data_xchg_protocol(corelink_data_xchg_protocol &&) = default;

                /**
                 * @brief Destructor
                 */
                virtual ~corelink_data_xchg_protocol() = default;
            };
        }
    }
}