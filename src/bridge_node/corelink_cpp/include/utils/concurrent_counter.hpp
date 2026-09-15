#pragma once

#include "commons/base_includes.hpp"
#include "commons/platform_macros.hpp"
#include "commons/reader_writer_lock_shim.hpp"

namespace corelink
{
    namespace utils
    {
        /**
         * Concurrent counter class
         * @tparam numeric_type integral numeric type
         */
        template<class numeric_type = uint32_t>
        class CORELINK_EXPORT concurrent_counter
        {
        private:
            /**
             * @brief counter
             */
            std::atomic<numeric_type> m_counter;
            /**
             * @brief counting variable reset value. will be defaulted to 0
             */
            const numeric_type m_reset_value;
            /**
             * @brief counting variable upper bound for reset
             */
            const numeric_type m_upper_bound;
            /**
             * @brief counting variable lower bound for reset
             */
            const numeric_type m_lower_bound;

        public:
            /**
             * @brief Type of the counter
             */
            typedef numeric_type counter_type;

            // make sure that the type is an integral type
            static_assert(std::is_integral<numeric_type>::value, "Not a integer numeric type");

            /**
             * @brief Construct a new counter object. Initializes the counter with a
             * - Upper bound of max(numeric_type)
             * - Lower bound of 0
             * - Reset value of 0
             * - Initial value of 0
             */
            explicit concurrent_counter()
                    : m_counter(0), m_reset_value(0), m_lower_bound(0),
                      m_upper_bound(std::numeric_limits<numeric_type>::max())
            {}

            /**
             * @brief Construct a new counter object
             * @param upper_bound upper bound of the counter variable after which the counter will be reset
             * @param lower_bound lower bound of the counter variable after which the counter will be reset
             */
            explicit concurrent_counter(numeric_type upper_bound, numeric_type lower_bound)
                    : m_counter(0), m_reset_value(0), m_upper_bound(upper_bound), m_lower_bound(lower_bound)
            {
            }

            /**
             * @brief Construct a new counter object
             * @param reset_value value to which the counter will be reset to
             * @param upper_bound upper bound of the counter variable after which the counter will be reset
             * @param lower_bound lower bound of the counter variable after which the counter will be reset
             */
            concurrent_counter(numeric_type reset_value, numeric_type upper_bound, numeric_type lower_bound)
                    : m_counter(0), m_reset_value(reset_value), m_upper_bound(upper_bound), m_lower_bound(lower_bound)
            {
            }

            /**
             * @brief Construct a new counter object
             * @param initial_value the initial value that will be loaded in to the counter at initialisation
             * @param reset_value value to which the counter will be reset to
             * @param upper_bound upper bound of the counter variable after which the counter will be reset
             * @param lower_bound lower bound of the counter variable after which the counter will be reset
             */
            concurrent_counter(numeric_type initial_value, numeric_type reset_value,
                               numeric_type upper_bound, numeric_type lower_bound)
                    : m_counter(initial_value), m_reset_value(reset_value), m_upper_bound(upper_bound),
                      m_lower_bound(lower_bound)
            {
            }

            virtual ~concurrent_counter() = default;

            /**
             * Get the underlying counter. Do not access
             * @return underlying counter instance
             */
            inline std::atomic<numeric_type> &operator()()
            {
                return m_counter;
            }

            /**
             * @brief overloaded post increment operator
             */
            inline numeric_type operator++(int32_t)
            {
                if (m_counter < (m_upper_bound - 1)) return m_counter.fetch_add(1);

                m_counter.store(m_reset_value);
                return m_counter;
            }

            /**
             * @brief overloaded pre increment operator
             */
            inline numeric_type operator++()
            {
                if (m_counter < (m_upper_bound - 1)) ++m_counter;
                else m_counter.store(m_reset_value);
                return m_counter;
            }

            /**
             * @brief overloaded post decrement operator
             */
            inline numeric_type operator--(int32_t)
            {
                if (m_counter > m_lower_bound) return m_counter.fetch_sub(1);

                m_counter.store(m_reset_value);
                return m_counter;
            }

            /**
             * @brief overloaded pre decrement operator
             */
            inline numeric_type operator--()
            {
                if (m_counter > m_lower_bound) --m_counter;
                else m_counter.store(m_reset_value);
                return m_counter;
            }

            /**
             * @brief type conversion operator
             * @return the current counter value
             */
            inline explicit operator numeric_type() const
            {
                return m_counter.load();
            }

            /**
             * @brief Returns the counter to its reset value
             *
             */
            inline void reset()
            {
                m_counter.store(m_reset_value);
            }

            /**
             * @brief overloaded ostream operator for capturing the counter value
             * @param os std::ostream instance
             * @param rhs instance of the counter class
             * @return std::ostream instance
             */
            friend std::ostream &operator<<(std::ostream &os, const concurrent_counter &rhs)
            {
                os << rhs.m_counter;
                return os;
            }
        };
    }
}