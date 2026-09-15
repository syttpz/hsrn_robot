#pragma once

#include <cstring>

#include "commons/platform_macros.hpp"
#include "commons/base_includes.hpp"
#include "commons/stream_includes.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            /**
             * @brief Corelink network constants namespace
             */
            namespace constants
            {
                /**
                 * @brief Loop back address IPv4 representation
                 */
                CORELINK_CPP_ATTR_MAYBE_UNUSED constexpr CORELINK_EXPORT ptr_to_const_val<char> IPV4_LOOP_BACK_ADDRESS{
                        "127.0.0.1"};
                /**
                 * @brief Loop back address IPv6 representation
                 */
                CORELINK_CPP_ATTR_MAYBE_UNUSED constexpr CORELINK_EXPORT ptr_to_const_val<char> IPV6_LOOP_BACK_ADDRESS{
                        "::1"};

                /**
                 * @brief Holds all members, types and functions pertaining to protocol related items.
                 * Note that this is largely for Corelink, rather than the networking libraries themselves
                 */
                namespace protocols
                {
                    /**
                     * @brief A short protocol wrapper to allow for string representation of corelink supported protocols,
                     * with a few extra attributes which are used inside the client.
                     */
                    struct CORELINK_EXPORT protocol
                    {
                    public:
                        /**
                         * @brief Bit mask position type
                         */
                        using bit_mask_pos_type = uint16_t;
                        /**
                         * @brief Bit mask value type
                         */
                        using bit_mask_type = std::bitset<sizeof(bit_mask_pos_type) * 8>;

                    private:
                        /**
                         * @brief Short name of the protocol identified by corelink
                         */
                        ptr_to_const_val<char> name{};
                        /**
                         * @brief bit mask position to mark the protocol type
                         */
                        const bit_mask_pos_type bit_pos{};

                    public:
                        /**
                         * @brief constructor
                         * @tparam sz auto deduced. do not supply
                         * @param s protocol short name
                         * @param pos protocol bit mask position
                         */
                        template<size_t sz>
                        constexpr protocol(const char(&s)[sz], bit_mask_pos_type pos): name(s), bit_pos(pos)
                        {}

                        /**
                         * @brief Equality comparison. check if 2 protocol objects are similar
                         * @param rhs protocol to compare 'this' protocol against
                         * @return true if the names and bit pos match. note that comparison is case sensitive
                         */
                        inline bool operator==(const protocol &rhs) const
                        {
                            return (std::strcmp(name, rhs.name) == 0) && bit_pos == rhs.bit_pos;
                        }

                        /**
                         * Inequality comparison. check if 2 protocol objects are NOT similar
                         * @param rhs protocol to compare 'this' protocol against
                         * @return true if either the names and bit pos don't  match. note that comparison is case sensitive
                         */
                        inline bool operator!=(const protocol &rhs) const
                        {
                            return !((*this) == rhs);
                        }

                        /**
                         * @brief Overloaded () operator
                         * @return the bit mask position
                         */
                        constexpr inline bit_mask_pos_type operator()() const
                        {
                            return bit_pos;
                        }

                        /**
                         * @brief Type conversion operator
                         * @return name of the protocol
                         */
                        inline ptr_to_const_val<char> proto_name() const
                        {
                            return name;
                        }

                        /**
                         * @brief overloaded ostream operator
                         * @param os std::ostream instance
                         * @param p protocol instance
                         * @return std::ostream instance
                         */
                        inline friend std::ostream &operator<<(std::ostream &os, const protocol &p)
                        {
                            os << "Name: " << p.name << ", Bit mask position: " << p.bit_pos;
                            return os;
                        }
                    };

                    /**
                     * @brief Marker for unknown protocol to mark default protocol values
                     */
                    static constexpr protocol unknown{"unknown",
                                                                      8 * sizeof(protocol::bit_mask_pos_type)};

#ifdef CORELINK_USE_TCP
                    /**
                     * @brief TCP protocol object for corelink protocols
                     */
                    static constexpr protocol tcp{"tcp", 0};
#endif
#ifdef CORELINK_USE_UDP
                    /**
                     * @brief UDP protocol object for corelink protocols
                     */
                    static constexpr protocol udp{"udp", 1};
#endif
#ifdef CORELINK_USE_WEBSOCKET
                    /**
                     * @brief Websocket protocol object for corelink protocols
                     */
                    static constexpr protocol websocket{"ws", 2};
#endif //CORELINK_USE_WEBSOCKET
                }

                /**
                 * @enum state
                 * @brief Corelink channel states. Each value denotes a unique position in the bitmask
                 */
                enum class state : uint8_t
                {
                    initialised = 0,
                    connected = 1,
                    stale = 2,
                    in_error = 3,
                };
            }
        }
    }
}