#include "common.hpp"
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdio>
#include <cstdlib>

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

void sound(const char* name, float volume)
{
    static const bool web = std::getenv("PULTSARI_WEB") != nullptr;
    if (!web) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "\033]777;snd;%s;%.2f\007", name, volume);
    cout << buf << std::flush;
}

}