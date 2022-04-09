#ifndef COMMON_HPP
#define COMMON_HPP

#include "coord.hpp"
#include <cstdint>
#include <string>

namespace common {

constexpr uint32_t SIZEX = 80;
constexpr uint32_t SIZEY = 28;

static bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

bool checkBounds(Coord& coord);

} // namespace common

#endif