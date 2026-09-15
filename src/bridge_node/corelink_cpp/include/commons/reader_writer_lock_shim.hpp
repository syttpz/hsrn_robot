#pragma once

#include "platform_macros.hpp"
#include "concurrency_includes.hpp"

namespace corelink
{
    namespace commons
    {
        using exclusive_mutex = std::mutex;
        using exclusive_lock = std::lock_guard<exclusive_mutex>;
#if defined(CORELINK_CPP17_SUPPORTED)
        using reader_writer_mutex = std::shared_mutex;
        using reader_lock = std::shared_lock<reader_writer_mutex>;
#elif defined(CORELINK_CPP14_SUPPORTED)
        using reader_writer_mutex = std::shared_timed_mutex;
        using reader_lock = std::shared_lock<reader_writer_mutex>;
#else
        using reader_writer_mutex = exclusive_mutex;
        using reader_lock = exclusive_lock;
#endif
        using writer_lock = std::lock_guard<reader_writer_mutex>;
    }
}