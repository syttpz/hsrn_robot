#pragma once

#if defined(CORELINK_USE_TCP) || defined(CORELINK_USE_UDP)

#include "commons/asio_includes.hpp"
#include "commons/concurrency_includes.hpp"
#include "commons/stream_includes.hpp"
#include "commons/base_includes.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            /**
             * @brief ASIO sockets context manager for async communication
             * @details This class houses functions to control the context runner threads which handle the async back and forth between hosts.
             * This employs a thread pooling mechanism on io_context to make concurrent connections and use
             * ASIO default thread pool model to handle all TCP and UDP related calls.
             */
            class CORELINK_EXPORT corelink_data_xchg_raw_socket_protocol_context_manager
            {
            private:
                /**
                 * @brief dummy work. this is to make sure that io_context.run() does not exit if there is nothing to do.
                 */
                std::shared_ptr<asio::executor_work_guard<asio::io_context::executor_type>> m_dummy_work;

                /**
                 * @brief context runner thread pool
                 */
                std::vector<corelink_thread> m_context_runner_threads;
                /**
                 * @brief Flag: true if the context manager threads are running. False if they shut down.
                 */
                bool m_context_runner_thread_running;
                /**
                 * @brief Concurrency level. this is essentially a user parameter to decide how many threads should be pooled to
                 * handle requests.
                 */
                size_t m_concurrency_level;

            protected:
                /**
                 * @brief The ASIO service or the new term, context. This handles all underlying initialization of a platform
                 */
                asio::io_context m_context;

            public:
                /**
                 * @brief The context error handler. This callback is called when the context thread(s) face an issue
                 */
                std::function<void(const std::string &)> context_error_handler = [](const std::string &error_string)
                {
                    std::cerr << error_string << "\n";
                };

                /**
                 * @brief Parameterized constructor.
                 * @param concurrency_level Number of concurrent request handlers. default is 1
                 */
                explicit corelink_data_xchg_raw_socket_protocol_context_manager(size_t concurrency_level = 1);

                /**
                 * @brief deleted copy constructor. context manager should NOT BE COPIED/MOVED, ONLY PASSED BY REFERENCE.
                 */
                corelink_data_xchg_raw_socket_protocol_context_manager(
                        const corelink_data_xchg_raw_socket_protocol_context_manager &) = delete;

                /**
                 * @brief deleted move constructor. context manager should NOT BE COPIED/MOVED, ONLY PASSED BY REFERENCE.
                 */
                corelink_data_xchg_raw_socket_protocol_context_manager(
                        corelink_data_xchg_raw_socket_protocol_context_manager &&) noexcept = delete;

                /**
                 *
                 * @brief deleted move assignment operator. context manager should NOT BE COPIED/MOVED, ONLY PASSED BY REFERENCE.
                 */
                corelink_data_xchg_raw_socket_protocol_context_manager &operator=(
                        corelink_data_xchg_raw_socket_protocol_context_manager &&) = delete;

                /**
                 * @brief deleted copy assignment operator. context manager should NOT BE COPIED/MOVED, ONLY PASSED BY REFERENCE.
                 */
                corelink_data_xchg_raw_socket_protocol_context_manager &operator=(
                        const corelink_data_xchg_raw_socket_protocol_context_manager &) = delete;

                /**
                 * @brief Destructor
                 */
                ~corelink_data_xchg_raw_socket_protocol_context_manager()
                {}

                /**
                 * @brief Get reference to the underlying ASIO context
                 * @return
                 */
                inline lvref<asio::io_context> get_context()
                { return m_context; }

                /**
                 * @brief starts the context runner threads
                 */
                void start_context_manager();

                /**
                 * @brief Stop the context runner safely
                 */
                void stop_context_manager();
            };
        }
    }
}
#endif