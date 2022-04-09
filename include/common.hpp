#ifndef COMMON_HPP
#define COMMON_HPP

#include <cstdint>
#include <string>

namespace common {

static bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

#if 0
constexpr uint32_t distance(uint32_t ox, uint32_t oy, uint32_t tx, uint32_t ty) {

    uint32_t distance = 0;
    while ((ox != tx) || (oy != ty))
    {
        if (ox < tx) ++ox;
        else if (ox > tx) --ox;

        if (oy < ty) ++oy;
        else if (oy > ty) --oy;

        ++distance;
    }

    return distance;
}
#endif

} // namespace common

#endif