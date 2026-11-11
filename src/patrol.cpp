#include "patrol.hpp"
#include "building.hpp"
#include "common.hpp"
#include "level.hpp"
#include "persons.hpp"
#include "player.hpp"
#include <cstdlib>

namespace Patrol {

// outdoors, off the carriageway (whoever may be standing there)
static bool openGround(const Level& level, const Coord& c)
{
    if (c.x >= common::SIZEX || c.y >= common::SIZEY) return false;
    if (level.hitBuilding(c)) return false;
    const Road* h = nullptr; const Road* v = nullptr;
    level.roadsAt(c, h, v);
    return !h && !v;
}

// nobody and nothing standing there
static bool free(const Level& level, const Coord& c)
{
    return !level.hit(c) && !level.getVehicleAt(c);
}

bool onPavement(const Level& level, const Coord& c)
{
    if (!openGround(level, c)) return false;
    for (const auto& r : level.roads) {
        const uint32_t at = r.horizontal ? c.y : c.x;
        if (at + 1 == r.pos || at == r.pos + 3) return true;   // the row or column beside the kerb
    }
    return false;
}

bool inAlley(const Level& level, const Coord& c)
{
    if (!openGround(level, c)) return false;
    // walls close on both sides, left and right or above and below
    auto wallWithin = [&](int dx, int dy, int reach) {
        for (int k = 1; k <= reach; ++k) {
            const int x = (int)c.x + dx * k, y = (int)c.y + dy * k;
            if (x < 0 || y < 0) return false;
            if (level.hitBuilding(Coord{ (uint32_t)x, (uint32_t)y })) return true;
        }
        return false;
    };
    return (wallWithin(-1, 0, 3) && wallWithin(1, 0, 3)) || (wallWithin(0, -1, 2) && wallWithin(0, 1, 2));
}

Coord pavementSpot(const Level& level)
{
    if (level.roads.empty()) return Coord{ 0, 0 };
    for (int tries = 0; tries < 200; ++tries) {
        const Road& r = level.roads[std::rand() % level.roads.size()];
        const uint32_t side = common::random(50) ? (r.pos > 0 ? r.pos - 1 : r.pos + 3) : r.pos + 3;
        const uint32_t along = std::rand() % (r.horizontal ? common::SIZEX : common::SIZEY);
        const Coord c = r.horizontal ? Coord{ along, side } : Coord{ side, along };
        if (onPavement(level, c) && free(level, c)) return c;
    }
    return Coord{ 0, 0 };
}

Coord alleySpot(const Level& level)
{
    // look just outside a random building's walls: alleys run along them
    for (int tries = 0; tries < 600 && !level.buildings.empty(); ++tries) {
        const Building* b = level.buildings[std::rand() % level.buildings.size()];
        const uint32_t off = 1 + std::rand() % 2;
        Coord c;
        switch (std::rand() % 4) {
            case 0: c = { b->wallLeft >= off ? b->wallLeft - off : 0, b->wallTop + std::rand() % (b->wallBot - b->wallTop + 1) }; break;
            case 1: c = { b->wallRight + off, b->wallTop + std::rand() % (b->wallBot - b->wallTop + 1) }; break;
            case 2: c = { b->wallLeft + std::rand() % (b->wallRight - b->wallLeft + 1), b->wallTop >= off ? b->wallTop - off : 0 }; break;
            default: c = { b->wallLeft + std::rand() % (b->wallRight - b->wallLeft + 1), b->wallBot + off }; break;
        }
        if (inAlley(level, c) && free(level, c)) return c;
    }
    for (int tries = 0; tries < 2000; ++tries) {
        const Coord c { (uint32_t)(std::rand() % common::SIZEX), (uint32_t)(std::rand() % common::SIZEY) };
        if (inAlley(level, c) && free(level, c)) return c;
    }
    return Coord{ 0, 0 };
}

uint32_t wantedCops(uint32_t moscowStage)
{
    return 30 + 4 * std::min<uint32_t>(moscowStage, 8);
}

static Person* spawnCop(Level& level, const Coord& c)
{
    if (c.x == 0 && c.y == 0) return nullptr;
    return level.createPerson(c, poliisi);
}

void populateCops(Level& level, uint32_t moscowStage)
{
    const uint32_t n = wantedCops(moscowStage) + std::rand() % 4;
    for (uint32_t i = 0; i < n; ++i) {
        if (!spawnCop(level, i % 2 ? alleySpot(level) : pavementSpot(level)))
            level.createPerson(level.freePosition(), poliisi);
    }
}

// cops in the drunk's part of town: just beyond the edges of the screen
static bool nearby(const Coord& a, const Coord& b)
{
    return std::abs((int)a.x - (int)b.x) <= (int)common::VIEWPORT_W &&
           std::abs((int)a.y - (int)b.y) <= (int)common::VIEWPORT_H;
}

static bool onScreen(const Coord& a, const Coord& b)
{
    return std::abs((int)a.x - (int)b.x) < (int)common::VIEWPORT_W / 2 + 4 &&
           std::abs((int)a.y - (int)b.y) < (int)common::VIEWPORT_H / 2 + 4;
}

void patrolTick(Level& level)
{
    if (level.isWarLevel || level.roads.empty()) return;
    const PlayerNS::Player* pl = level.getPlayer();
    uint32_t cops = 0, local = 0;
    for (auto p : level.persons) {
        if (p->type != poliisi || p->stats.health == 0) continue;
        ++cops;
        if (pl && nearby(p->coord, pl->coord)) ++local;
    }
    // the drunk's own streets are never long without a patrol
    if (pl && local < LOCAL_COPS && cops < wantedCops(level.stage) * 3 / 2 && common::random(25)) {
        for (int tries = 0; tries < 300; ++tries) {
            const Coord c = common::random(60) ? pavementSpot(level) : alleySpot(level);
            if (c.x == 0 && c.y == 0) continue;
            if (!nearby(c, pl->coord) || onScreen(c, pl->coord)) continue;
            if (spawnCop(level, c)) return;
        }
    }
    // and the rest of the city fills up again, slowly
    if (cops >= wantedCops(level.stage) || !common::random(2)) return;
    for (int tries = 0; tries < 30; ++tries) {
        const Coord c = common::random(50) ? alleySpot(level) : pavementSpot(level);
        if (c.x == 0 && c.y == 0) continue;
        if (pl && onScreen(c, pl->coord)) continue;   // nobody pops up in plain view
        if (spawnCop(level, c)) return;
    }
}

}
