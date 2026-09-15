#pragma once

#if defined(CORELINK_USE_CONCURRENT_QUEUE)

#include "commons/base_includes.hpp"
#include "commons/platform_macros.hpp"
#include "commons/sequence_container_includes.hpp"
#include "commons/reader_writer_lock_shim.hpp"

/**
 * @def CORELINK_USE_CONCURRENT_QUEUE
 * @brief If not defined, concurrent_queue will be excluded from compilation.
 */

namespace corelink
{
    namespace utils
    {
        namespace containers
        {
            /**
             * @brief Synchronized queue class for concurrent access
             * @tparam Queue element type
             */
            template<typename item_type>
            class CORELINK_EXPORT concurrent_queue
                    : public std::enable_shared_from_this<concurrent_queue<item_type>>
            {
            private:
                /**
                 * @typedef underlying queue type
                 */
                using queue_type = std::deque<item_type>;

                /**
                 * @var underlying container
                 */
                queue_type m_queue_container;
                /**
                 * @var synchronizer mutex
                 */
                commons::reader_writer_mutex m_sync_mutex;

            public:
                /**
                 * @brief Construct a queue object
                 */
                concurrent_queue()
                = default;

                /**
                 * @brief Construct a queue object with an initial size
                 * @param initial_size default number of elements the queue should start with
                 */
                explicit concurrent_queue(size_t initial_size)
                {
                    m_queue_container.resize(initial_size);
                }

                /**
                 * @brief Destroy the synchronized queue object
                 */
                virtual ~concurrent_queue()
                {
                    clear();
                }

                /**
                 * @brief adds item to the container
                 * @param value value to be added to the container
                 * @return reference to itself. allows for chaining.
                 */
                concurrent_queue<item_type> &push(const item_type &value)
                {
                    commons::writer_lock scope_lock(m_sync_mutex);
                    m_queue_container.emplace_back(value);
                    return *this;
                }

                /**
                 * @details peek the front of the queue
                 * @return reference to the front of the queue
                 */
                item_type &peek()
                {
                    commons::reader_lock scope_lock(m_sync_mutex);
                    return m_queue_container.front();
                }

                /**
                 * @brief drops item from the container based on the key
                 */
                inline void pop()
                {
                    commons::writer_lock scope_lock(m_sync_mutex);
                    if (!m_queue_container.empty())
                    {
                        m_queue_container.pop_front();
                    }
                }

                /**
                 * @brief clears the queue
                 * @return a reference to itself. allows for chaining.
                 */
                concurrent_queue<item_type> &clear()
                {
                    commons::writer_lock scope_lock(m_sync_mutex);
                    m_queue_container.clear();
                    return *this;
                }

                /**
                 * @details get the current queue size. Please do not use this function to check if the queue is empty
                 * Instead use the empty() function
                 * @return queue depth
                 */
                inline size_t size()
                {
                    commons::reader_lock scoped_lock(m_sync_mutex);
                    return m_queue_container.size();
                }

                /**
                 * @details Test whether queue is empty or not
                 * @return true if the queue is empty.
                 */
                inline bool empty()
                {
                    commons::reader_lock scoped_lock(m_sync_mutex);
                    return m_queue_container.empty();
                }
            };
        }
    }
}
#endif //CORELINK_USE_CONCURRENT_QUEUE