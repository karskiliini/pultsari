#include "common.hpp"
#include <thread>
#include <chrono>
#include <iostream>

using std::cout;
using std::endl;

namespace common {

bool random(uint32_t pct)
{
    return (uint32_t)(std::rand() % 101) <= pct;
}

bool checkBounds(Coord& coord) {
    auto ox = coord.x;
    auto oy = coord.y;

    coord.x = (coord.x == SIZEX) ? SIZEX - 1 : coord.x;
    coord.x = (coord.x > SIZEX)  ? 0 : coord.x;
    coord.y = (coord.y == SIZEY) ? SIZEY - 1 : coord.y;
    coord.y = (coord.y > SIZEY)  ? 0 : coord.y;

    return !((ox == coord.x) && (oy == coord.y));
}

void sleep(uint32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}