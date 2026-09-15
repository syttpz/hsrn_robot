#include "ros2_bridge_node/fragment.hpp"

#include <algorithm>
#include <cstring>

namespace bridge_node
{

// [frag header (9 byte)] + [corelink packet/payload]
auto pack_packet(uint32_t image_number, uint32_t sequence_number, bool is_last_fragment, const std::vector<uint8_t> &payload) -> std::vector<uint8_t>{

    
    if(payload.size() > kMaxFragmentPayload){
        throw std::length_error("Max fragment payload size exceeded");
    }else if(payload.empty()){
        throw std::domain_error("Empty payload");
    }

    // packet
    std::vector<uint8_t> packet (kFragmentHeaderSize + payload.size());
    std::memcpy(packet.data(), &image_number, 4);
    std::memcpy(packet.data() + 4, &sequence_number, 4);
    packet[8] = is_last_fragment ? 1 : 0;
    std::memcpy(packet.data() + 9, payload.data(), payload.size());

    return packet;
}

auto unpack_packet(const std::vector<uint8_t> &packet) -> std::tuple<uint32_t, uint32_t, bool, std::vector<uint8_t>>{
    if(packet.size() < kFragmentHeaderSize){
        throw std::length_error("Packet smaller than fragment header");
    }

    //extract header 4 + 4 + 1
    FragmentHeader Frheader;

    std::memcpy(&Frheader.image_number, packet.data(), 4);
    std::memcpy(&Frheader.sequence_number, packet.data() + 4, 4);
    std::memcpy(&Frheader.is_last_fragment, packet.data()+8, 1);

    //payload 
    std::vector<uint8_t> payload(packet.begin() + kFragmentHeaderSize, packet.end());
    
    return {Frheader.image_number, Frheader.sequence_number, Frheader.is_last_fragment, payload};
}

std::optional<std::vector<uint8_t>>
Reassembler::feed(const std::vector<uint8_t> &packet)
{
    const auto [image_number, sequence_number, is_last_fragment, payload] = unpack_packet(packet);

    evict_stale(image_number);

    PartialFrame &frame = m_in_progress[image_number];
    frame.chunks[sequence_number] = payload;
    if(is_last_fragment){
        frame.seen_last = true;
        frame.last_seq = sequence_number;
    }


    if(!frame.seen_last || frame.chunks.size() != static_cast<std::size_t>(frame.last_seq) + 1){
        return std::nullopt;
    }

    // Concatenate in sequence order 
    std::vector<uint8_t> assembled;
    std::size_t total = 0;
    for(const auto &[seq, chunk] : frame.chunks){
        (void)seq;
        total += chunk.size();
    }
    assembled.reserve(total);
    for(const auto &[seq, chunk] : frame.chunks){
        (void)seq;
        assembled.insert(assembled.end(), chunk.begin(), chunk.end());
    }

    m_in_progress.erase(image_number);
    return assembled;
}


void Reassembler::evict_stale(uint32_t newest_image_number)
{
    if(newest_image_number > m_newest_image_number){
        m_newest_image_number = newest_image_number;
    }

    for(auto it = m_in_progress.begin(); it != m_in_progress.end(); ){
        if(m_newest_image_number > it->first &&
           m_newest_image_number - it->first > kReassemblyWindow){
            it = m_in_progress.erase(it);
        }else{
            ++it;
        }
    }
}


std::vector<std::vector<uint8_t>>
Slicer::slice(const uint8_t *data, std::size_t len)
{
    std::vector<std::vector<uint8_t>> packets;
    if(len == 0){
        return packets;  // reject empty payload
    }

    const uint32_t image_number = cur_image_num++;

    // ceil(len / kMaxFragmentPayload) fragments, chunked in order
    const std::size_t total = (len + kMaxFragmentPayload - 1) / kMaxFragmentPayload;
    packets.reserve(total);

    for(std::size_t i = 0; i < total; ++i){
        const std::size_t offset = i * kMaxFragmentPayload;
        const std::size_t chunk = std::min(kMaxFragmentPayload, len - offset);
        const bool is_last_fragment = (i + 1 == total);

        std::vector<uint8_t> payload(data + offset, data + offset + chunk);
        packets.push_back(
                pack_packet(image_number, static_cast<uint32_t>(i), is_last_fragment, payload));
    }

    return packets;
}

} 
