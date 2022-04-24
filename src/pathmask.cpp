#include "pathmask.hpp"
#include "common.hpp"
#include "player.hpp"
#include "coord.hpp"
#include <exception>
#include <stdexcept>
#include <iostream>

namespace PathNS {

using std::string;
using PlayerNS::Player;

PathMask::PathMask(Level* level)
{
    PathMask::level = level;

    std::vector<uint32_t> line;
    for (uint32_t y = 0; y < common::SIZEY; ++y) {
        for (uint32_t x = 0; x < common::SIZEX; ++x) {
            line.push_back(0);
        }
        mask.push_back(line);
    }
}

uint32_t& PathMask::at(const Coord& c)
{
    if ((c.x < 0) || (c.y < 0) || (c.x >= common::SIZEX) || (c.y >= common::SIZEY)) {
        static uint32_t u = 0;
        return u;
    }

    auto& s = mask[c.y];
    return s[c.x];
}

bool hitPerson(PathNS::PathMask& mask, Coord coord)
{
    if (!mask.level->hitPerson(coord))
    {
        return false;
    } else {
        auto person = mask.level->getPerson(coord);
        auto type = person->type;

        if ((type == poliisi) && mask.skipCop) return false;
        if ((type == varas) && mask.skipVaras) return false;
        if ((type == vanki) && mask.skipConvict) return false;
    }

    return true;
}

bool recursePath(PathNS::PathMask& mask, uint32_t value)
{
    PlayerNS::Player* player = mask.level->getPlayer();
    const Coord& pCoord = player->coord;

    for (uint32_t y = 0; y < common::SIZEY; ++y) {
        for (uint32_t x = 0; x < common::SIZEX; ++x) {
            Coord dirs[4];
            dirs[0] = {x, y-1};
            dirs[1] = {x+1, y};
            dirs[2] = {x, y+1};
            dirs[3] = {x-1, y};

            for (uint32_t i = 0; i < 4; ++i)
            {
                Coord coord(x, y);
                if ((mask.at(coord) == 0) && (mask.at(dirs[i]) == value - 1))
                {
                    if (!hitPerson(mask, coord) && !mask.level->hitBuilding(coord))
                    {
                        mask.at(coord) = value;
                    }

                    if (pCoord == coord) {
                        mask.at(coord) = value;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool PathMask::findPath(const Coord& coord, const Coord& target)
{
    uint32_t value = 1;
    at(coord) = value;

    bool found = false;
    while (!found) {
        found = recursePath(*this, ++value);
        if (value > 100) // give up
        {
            return false;
        }
    }
    return true;
}

DirectionNS::Direction PathMask::traceBack(const Coord& c)
{
    const uint32_t& value = at(c);
    const uint32_t& next = value - 1;

    Coord dirs[4];
    dirs[0] = { c.x,     c.y - 1 };
    dirs[1] = { c.x + 1, c.y     };
    dirs[2] = { c.x,     c.y + 1 };
    dirs[3] = { c.x - 1, c.y     };

    std::vector<uint32_t> foundDirs;

    for (uint32_t i = 0; i < 4; ++i) {
        if (at(dirs[i]) == next) {
            // found npc character?
            if (next == 1) {
                switch(i) {
                    case 0: return DirectionNS::down;
                    case 1: return DirectionNS::left;
                    case 2: return DirectionNS::up;
                    case 3: return DirectionNS::right;
                    default:
                            return DirectionNS::none;
                }
            }
            foundDirs.push_back(i);
        }
    }
    if (foundDirs.size() == 0) {
        return DirectionNS::none;
    }

    // correct path found, randomize which one is picked
    uint32_t pick = (std::rand() % foundDirs.size());

    // std::cout << "size: " << foundDirs.size() << " pick: " << pick << std::endl;
    return traceBack(dirs[foundDirs[pick]]);
}

}