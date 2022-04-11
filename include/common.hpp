#ifndef COMMON_HPP
#define COMMON_HPP

#include "coord.hpp"
#include <cstdint>
#include <string>

namespace common {

constexpr uint32_t SIZEX = 80;
constexpr uint32_t SIZEY = 28;
constexpr uint32_t PLAYER_START_X = 10;
constexpr uint32_t PLAYER_START_Y = SIZEY/2;

static bool random(uint32_t pct)
{
    return (uint32_t)(rand() % 100) <= pct;
}

bool checkBounds(Coord& coord);

void sleep(uint32_t ms);

} // namespace common

#endif