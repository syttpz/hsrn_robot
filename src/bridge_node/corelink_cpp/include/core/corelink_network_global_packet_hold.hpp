#pragma once

#include "commons/base_includes.hpp"
#include "commons/sequence_container_includes.hpp"
#include "commons/reader_writer_lock_shim.hpp"
#include "utils/concurrent_counter.hpp"

namespace corelink
{
    namespace core
    {
        namespace memory
        {
#ifndef MAX_SLOT_COUNT
            /** @def MAX_SLOT_COUNT
             *  @brief Maximum number of packets to be held in escrow
             */
#define MAX_SLOT_COUNT 1024U
#endif
            static_assert(std::is_integral<decltype(MAX_SLOT_COUNT)>::value &&
                          MAX_SLOT_COUNT > 0,
                          "MAX_SLOT_COUNT needs to be an unsigned integer greater than 0");

            /**
             * @var packet_escrow
             * @details Global packet hold. This is a stop gap solution. In the future, I would like to use placement new
             * and perhaps pre-allocate global storage more nicely.
             * @warning Please do not access outside this file.
             */
            static std::array<std::vector<uint8_t>, MAX_SLOT_COUNT> packet_escrow;
            /**
             * @var current_slot
             * @details Current slot index
             * @warning Please do not access outside this file.
             */
//            static size_t current_slot = 0;
            static utils::concurrent_counter<size_t> current_slot{MAX_SLOT_COUNT, 0};
            /**
             * @var mtx
             * @details synchronizing mutex for concurrent access. I would have loved optimistic concurrency, but engines and TP
             * libraries do things unpredictably, so have to stick to a tried and tested solution. Will return to this
             * later when exploring OC.
             * @warning Please do not access outside this file.
             */
            static commons::reader_writer_mutex mtx;

            /**
             * @details Add an element to the global packet "escrow"
             * @param packet packet to be transported
             * @return index of the "escrow" slot packet was added to
             */
            CORELINK_EXPORT size_t assign_or_override_slot(rvref<std::vector<uint8_t>> packet);

            /**
             * @details Get the packet from global packet "escrow"
             * @param slot index of the "escrow" slot packet was added to returned by <b>assign_or_override_slot</b>
             * @return reference to the packet
             */
            CORELINK_EXPORT lvref<std::vector<uint8_t>> get_packet(size_t slot);

            /**
             * @details Clear the packet held in "escrow"
             * @param slot index of the "escrow" slot packet was added to returned by <b>assign_or_override_slot</b>
             */
            CORELINK_EXPORT void clear_packet(size_t slot);
        }
    }
}