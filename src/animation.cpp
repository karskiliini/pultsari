#include "animation.hpp"
#include "common.hpp"
#include <iostream>

using std::string;
using std::cout;
using std::endl;

Animation::Animation(const Coord& pos, string txt, uint32_t frames) : coord(pos), text(txt), ttl(frames)
{
}

bool Animation::act()
{
    if (!common::animsEnabled)
    {
        return false;
    }

    if (ttl == 0) return false;

    coord.x += coordMove.x;
    coord.y += coordMove.y;
    --ttl;

    return (ttl > 0);
}
