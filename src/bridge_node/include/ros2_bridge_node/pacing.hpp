#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace ros2_bridge_node
{

// Spread all fragments of one frame across the usable part of its frame
// period. reserve_fraction leaves time for headers, scheduling jitter, and
// other streams:
//
//   gap = (1 / frame_rate) * (1 - reserve_fraction) / fragment_count
inline int64_t calculatePacingUs(double frame_rate_hz,
                                 double reserve_fraction,
                                 std::size_t fragment_count)
{
    if (!std::isfinite(frame_rate_hz) || frame_rate_hz <= 0.0)
    {
        throw std::invalid_argument("frame_rate_hz must be finite and > 0");
    }
    if (!std::isfinite(reserve_fraction) || reserve_fraction < 0.0 ||
        reserve_fraction >= 1.0)
    {
        throw std::invalid_argument("reserve_fraction must be in [0, 1)");
    }
    if (fragment_count == 0)
    {
        throw std::invalid_argument("fragment_count must be > 0");
    }

    const double frame_period_us = 1'000'000.0 / frame_rate_hz;
    const double usable_period_us = frame_period_us * (1.0 - reserve_fraction);
    return static_cast<int64_t>(std::floor(usable_period_us /
                                           static_cast<double>(fragment_count)));
}

} 