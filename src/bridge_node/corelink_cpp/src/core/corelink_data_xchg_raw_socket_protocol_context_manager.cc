#include "core/corelink_data_xchg_raw_socket_protocol_context_manager.hpp"

namespace corelink
{
    namespace core
    {
        namespace network
        {
            corelink_data_xchg_raw_socket_protocol_context_manager::corelink_data_xchg_raw_socket_protocol_context_manager(
                    size_t concurrency_level)
                    : m_context_runner_thread_running(false),
                      m_concurrency_level(concurrency_level)
            {
                m_dummy_work = std::make_shared<asio::executor_work_guard<asio::io_context::executor_type>>(
                        m_context.get_executor());
            }

            void corelink_data_xchg_raw_socket_protocol_context_manager::start_context_manager()
            {
                // if there is at least one thread running, don't restart threads or add more
                if (m_context_runner_thread_running)
                    return;
                for (size_t count = 0; count < m_concurrency_level; ++count)
                {
                    // add a thread to the pool
                    corelink_thread th(
                            [&]()
                            {
                                // thread is running. mark it
                                m_context_runner_thread_running = true;
                                // this call will block till ALL the work in the ASIO queue is done.
                                try
                                {
                                    m_context.run();
                                }
                                catch (asio::error_code &error)
                                {
                                    // okay so m_context.run(error); returned. Which means the context was either freed
                                    // or there was an error
                                    m_context_runner_thread_running = false;
                                    // check if there is a valid error and if the context handler is init
                                    if (context_error_handler)
                                    {
                                        // log the error
                                        std::stringstream error_str;
                                        error_str << "Context runner error: Code [" << error.value()
                                                  << "] Name[" << error.category().name()
                                                  << "] Message: "
                                                  << error.message() << "\n";
                                        context_error_handler(error_str.str());
                                    }
                                }
                            }
                    );
                    m_context_runner_threads.emplace_back(std::move(th));
                    // detach yourself from the context thread. what this allows is that after the function call ends the threads
                    // won't be bound to the original thread. This is equivalent to daemon-ising the threads
                    m_context_runner_threads.at(count).detach();
                }
            }

            void corelink_data_xchg_raw_socket_protocol_context_manager::stop_context_manager()
            {
                try
                {
                    // if previously shut down, no need to do again
                    if (m_context_runner_thread_running)
                    {
                        // free the dummy object
                        m_dummy_work.reset();
                        // stop the context threads
                        m_context.stop();
                    }
                }
                catch (std::exception &) // this is here just so that if context.stop throws we can catch it
                {
                }
            }
        }
    }
}