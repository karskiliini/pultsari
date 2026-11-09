#include "mask.hpp"
#include "level.hpp"
#include "common.hpp"
#include "player.hpp"
#include "coord.hpp"
#include "building.hpp"
#include "vehicle.hpp"
#include <exception>
#include <stdexcept>
#include <algorithm>
#include <cmath>
#include <cstdlib>

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

namespace {

Vec toVec(const Coord& from, const Coord& to)
{
    return { (float)to.x - (float)from.x, (float)to.y - (float)from.y };
}

float length(const Vec& v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

}

bool Mask::glanceTick()
{
    const auto now = std::chrono::steady_clock::now();
    if (now - lastGlanceTick < std::chrono::milliseconds(GLANCE_DECIDE_MS)) return false;
    lastGlanceTick = now;
    return true;
}

Vec Mask::gazeDirection(const Vec& walk)
{
    // glancing sideways now and then: decided at most every GLANCE_DECIDE_MS,
    // so in real time the head turns and stays a moment instead of flickering
    if (glances && glanceTick()) {
        if (glanceTurns > 0) {
            --glanceTurns;
        } else if (common::random(GLANCE_CHANCE)) {
            glanceTurns = common::random(25) ? 2 : 1;
            float span = GLANCE_MAX_ANGLE - GLANCE_MIN_ANGLE;
            glanceAngle = GLANCE_MIN_ANGLE + span * (float)(std::rand() % 101) / 100.f;
            if (std::rand() % 2) glanceAngle = -glanceAngle;
        }
    }
    if (glanceTurns > 0) {
        return rotate(walk, glanceAngle);
    }

    // otherwise stare at the nearest person in front
    Player* p = level->getPlayer();
    const Coord& c = p->coord;

    const Person* nearest = nullptr;
    float nearestDist = FOCUS_RANGE;
    for (auto o : level->persons) {
        if (o == p) continue;
        Vec d = toVec(c, o->coord);
        float dist = length(d);
        if (dist >= nearestDist) continue;
        if (angleBetween(walk, d) > FOCUS_SEARCH_ANGLE) continue;
        if (!level->raycast(c, o->coord)) continue;
        nearest = o;
        nearestDist = dist;
    }

    if (nearest) return toVec(c, nearest->coord);
    return walk;
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
    const float sickness = (float)sight / (float)VISION_DISTANCE;

    // the eyes follow the walking direction; drunkenness narrows the cone
    // they turn gradually: never more than maxTurn degrees per update
    if (!aimed) {
        shownWalk = turnTowards(p->heading.dir(), p->heading.dir(), 0.f);
        shownGaze = shownWalk;
        aimed = true;
    }
    shownWalk = turnTowards(shownWalk, p->heading.dir(), maxTurn);
    shownGaze = turnTowards(shownGaze, gazeDirection(shownWalk), maxTurn);
    const Vec walk = shownWalk;
    const Vec gaze = shownGaze;
    const float cone = coneScale(p->stats.promilles);

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
            line[x] = ' ';

            if (litRoom && litRoom->hitBuilding(dc)) {
                line[x] = '.';
                continue;
            }

            // at the wheel the eyes follow the road (see driving.cpp)
            if (p->driving) {
                if (gazeSees(p, c, dc, level->isLit(dc) ? sight * 2 : sight) && level->raycast(c, dc)) {
                    line[x] = '.';
                }
                continue;
            }

            Vec d = toVec(c, dc);
            float dist = length(d);
            if (dist > FOCUS_RANGE) continue;

            float range = visionRange(angleBetween(gaze, d), angleBetween(walk, d), cone);
            range *= sickness * (level->isLit(dc) ? 1.f : DARK_SCALE);
            range = std::max(range, NEAR_RANGE);

            if (dist <= range && level->raycast(c, dc)) {
                line[x] = '.';
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