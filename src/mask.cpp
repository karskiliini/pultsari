#include "mask.hpp"
#include "common.hpp"
#include "player.hpp"
#include "coord.hpp"
#include <exception>
#include <stdexcept>

namespace VisionNS {

using std::string;
using PlayerNS::Player;

Mask::Mask(Level* level)
{
    Mask::level = level;

    string line;
    for (uint32_t y = 0; y < common::SIZEY; ++y) {
        for (uint32_t x = 0; x < common::SIZEX; ++x) {
            line += " ";
        }
        mask.push_back(line);
    }
}

void Mask::enable()
{
    enabled = true;
}

void Mask::init()
{
    if (!enabled) return;

    if (!level) throw std::out_of_range("level not found!");
    Player* p = level->getPlayer();
    if (!p) throw std::out_of_range("player not found!");

    const Coord& c = p->coord;

    for (uint32_t y = 0; y < common::SIZEY; ++y) {
        for (uint32_t x = 0; x < common::SIZEX; ++x)
        {
            Coord dc { x, y };
            auto& line = mask[y];

            if (c.distance(dc) < VISION_DISTANCE) {
                if (level->raycast(c, dc)) {
                    line[x] = '.';
                } else {
                    line[x] = ' ';
                }
            } else {
                line[x] = ' ';
            }
        }
    }
}

bool Mask::visible(const Coord& c) const {
    if (!enabled) return true;

    auto& line = mask[c.y];
    return line[c.x] == '.';
}

}