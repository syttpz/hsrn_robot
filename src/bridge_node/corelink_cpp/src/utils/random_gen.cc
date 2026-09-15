#include "utils/random_gen.hpp"


uint64_t corelink::utils::random_numbers::get_random_int()
{
    std::uniform_int_distribution<uint64_t> dist;
    return dist(mt);
}