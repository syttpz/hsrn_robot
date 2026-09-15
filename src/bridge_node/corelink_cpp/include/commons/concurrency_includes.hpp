#pragma once

#include <thread>
#include <mutex>

#if defined(CORELINK_CPP14_SUPPORTED) || defined (CORELINK_CPP17_SUPPORTED)
#include <shared_mutex>
#endif

#include <condition_variable>
#include <future>

#if !defined(CORELINK_USE_OTHER_THREAD)
using corelink_thread = std::thread;
#endif