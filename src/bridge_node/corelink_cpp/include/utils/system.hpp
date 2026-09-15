#pragma once

#include "commons/platform_macros.hpp"
#include "commons/typedefs.hpp"
#include "commons/base_includes.hpp"

namespace corelink
{
    namespace utils
    {
        namespace system
        {
            namespace _internals
            {
                template<typename t, size_t sz = sizeof(t)>
                union CORELINK_EXPORT pun
                {
                    static_assert(std::is_arithmetic<t>::value, "Non-numeric values are not supported");
                    t val;
                    std::array<uint8_t, sz> bytes;
                };
            }

            /**
             * @brief Mark the endianness of data in the memory
             * @details this enum facilitates different types of endianness-es of data in memory
             * C++ 20 has std::endian but since we are still floating between 11-17, we define some rudimentary items
             * ourselves
             */
            CORELINK_EXPORT enum class endianness
            {
                big, little, unknown
            };

            /**
             * Determine the endianness of the current system architecture
             * @return
             */
            CORELINK_EXPORT inline endianness sys_endianness()
            {
                _internals::pun<int32_t> _{0x01020304};
                switch (_.bytes[0])
                {
                    case 1:
                        return endianness::big;
                    case 4:
                        return endianness::little;
                    default:
                        return endianness::unknown;
                }
            }

            /**
             * Convert a numeric value to an array of bytes.
             * will raise a compiler error if non-numeric type is indicated.
             * @tparam num_typ numeric type
             * @tparam sz the number of bytes to convert. it is defaulted to sizeof(num_typ)
             * @tparam e endianness of data needed. it is defaulted to endianness::little
             * @param val value to convert in to bytes
             * @return array with bytes of data as signified by the endianness
             * if the required order is anything other than endianness::little or endianness::big
             * an empty buffer is returned
             */
            template<typename num_typ,
                    size_t sz = sizeof(num_typ),
                    endianness e = endianness::little>
            CORELINK_EXPORT inline std::vector<uint8_t> to_bytes(num_typ val)
            {
                // if system endianness and required endianness are same, we already
                // have the data in the buffer in the required endianness order.
                // if not, we just reverse the buffer order
                const auto sys_endian = sys_endianness();
                if (sys_endian == endianness::unknown || e == endianness::unknown) return {};

                _internals::pun<num_typ, sz> _{val};
                auto bytes = std::vector<uint8_t>(std::make_move_iterator(_.bytes.begin()),
                                                  std::make_move_iterator(_.bytes.end()));
                if (sys_endian != e) std::reverse(bytes.begin(), bytes.end());
                return bytes;
            }

            /**
             * Convert a byte array to a numeric value
             * will raise a compiler error if non-numeric type is indicated.
             * @tparam num_typ numeric type to convert to
             * @tparam e endianness of the data passed in the array. it is defaulted to endianness::little
             * @param bytes byte array
             * @param default_val in case the endianness of the system or the data is unknown,
             * we just return the default value
             * @return value of num_typ
             */
            template<typename num_typ, endianness e = endianness::little>
            CORELINK_EXPORT inline num_typ from_bytes(std::vector<uint8_t> bytes, num_typ default_val = num_typ())
            {
                // if the system endianness is the same as the required endianness,
                // we can just pun the bytes as the final value
                // however, if they are not the same, we reverse the bytes to get the desired
                // value
                const auto sys_endian = sys_endianness();
                if (sys_endian == endianness::unknown || e == endianness::unknown) return default_val;

                if (e != sys_endian) std::reverse(bytes.begin(), bytes.end());
                _internals::pun<num_typ> _{*reinterpret_cast<num_typ *>(bytes.data())};
                return _.val;
            }
        }
    }
}