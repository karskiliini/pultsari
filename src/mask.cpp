#include "mask.hpp"
#include "level.hpp"
#include "common.hpp"
#include "player.hpp"
#include "coord.hpp"
#include "building.hpp"
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

    // standing in a lit room shows the whole room
    const Building* litRoom = nullptr;
    for (auto b : level->buildings) {
        if ((b->lit || b->walkable) && b->hitBuilding(c)) { litRoom = b; break; }
    }

    // HIV narrows the world as it progresses
    const uint32_t sight = Disease::visionDistance(VISION_DISTANCE, p->diseaseStage());

    uint32_t startX = (level->cameraX > 0) ? (uint32_t)level->cameraX : 0;
    uint32_t startY = (level->cameraY > 0) ? (uint32_t)level->cameraY : 0;
    uint32_t endX = startX + common::VIEWPORT_W;
    uint32_t endY = startY + common::VIEWPORT_H;
    if (endX > common::SIZEX) endX = common::SIZEX;
    if (endY > common::SIZEY) endY = common::SIZEY;

    for (uint32_t y = startY; y < endY; ++y) {
        for (uint32_t x = startX; x < endX; ++x)
        {
            Coord dc { x, y };
            auto& line = mask[y];

            if (litRoom && litRoom->hitBuilding(dc)) {
                line[x] = '.';
            } else if (c.distance(dc) < (level->isLit(dc) ? sight * 2 : sight)) {
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
    if (c.y >= common::SIZEY || c.x >= common::SIZEX) return false;

    auto& line = mask[c.y];
    return line[c.x] == '.';
}

}