#include "core/corelink_network_global_packet_hold.hpp"

namespace corelink
{
    size_t core::memory::assign_or_override_slot(rvref<std::vector<uint8_t>> packet)
    {
        // <-- this is only because if someone issues a clear_packet, you do not want to override that
        // ideally, you would have a per packet lock, but, that is too expensive sometimes on the memory.
        // TODO: Work on a better way to minimize the number of locks, but provide full sync.
        size_t ret_val = current_slot++;
        commons::writer_lock lock(mtx);
        packet_escrow[ret_val] = std::move(packet);
        return ret_val;
    }

    lvref<std::vector<uint8_t>> core::memory::get_packet(size_t slot)
    {
        commons::reader_lock lock(mtx);
        auto &p = packet_escrow[slot];
        return p;
    }

    void core::memory::clear_packet(size_t slot)
    {
        commons::writer_lock lock(mtx);
        if (!packet_escrow[slot].empty())
            packet_escrow[slot].clear();
    }
}