#pragma once

#include "commons/base_includes.hpp"
#include <random>

namespace corelink
{
    namespace utils
    {
        /**
         * @brief Houses and wraps all utils related to random number generation
         */
        namespace random_numbers
        {
            static thread_local std::random_device rd;
            static thread_local std::mt19937_64 mt(rd());

            /**
             * @details pseudo-random number generator based on the mersenne prime number generator class
             * using a uniform distribution model
             * @tparam int_type C++ integral types
             * @return generated prime number of chosen integral type
             */
            CORELINK_EXPORT uint64_t get_random_int();
        }
    }
}