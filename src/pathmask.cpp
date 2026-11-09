#include "pathmask.hpp"
#include "level.hpp"
#include "persons.hpp"
#include "common.hpp"
#include <algorithm>
#include <deque>
#include <vector>

namespace PathNS {

bool PathMask::blocked(const Coord& c) const
{
    if (level->hitBuilding(c)) return true;
    Person* p = level->getPerson(c);
    if (!p) return false;
    if (p->type == poliisi && skipCop) return false;
    if (p->type == varas && skipVaras) return false;
    if (p->type == vanki && skipConvict) return false;
    return true;
}

DirectionNS::Direction PathMask::firstStep(const Coord& from, const Coord& to) const
{
    if (from == to) return DirectionNS::none;

    // search a box around both ends so a call stays cheap on the 400x200 map
    constexpr int MARGIN = 20;
    const int x0 = std::max(0, (int)std::min(from.x, to.x) - MARGIN);
    const int y0 = std::max(0, (int)std::min(from.y, to.y) - MARGIN);
    const int x1 = std::min((int)common::SIZEX - 1, (int)std::max(from.x, to.x) + MARGIN);
    const int y1 = std::min((int)common::SIZEY - 1, (int)std::max(from.y, to.y) + MARGIN);
    const int w = x1 - x0 + 1;
    const int h = y1 - y0 + 1;

    // first step direction taken from `from` for every reached cell; -1 = unvisited
    std::vector<int8_t> via(w * h, -1);
    std::deque<Coord> queue;

    const int dx[4] = { 0, 1, 0, -1 };
    const int dy[4] = { -1, 0, 1, 0 };
    const DirectionNS::Direction dirs[4] = {
        DirectionNS::up, DirectionNS::right, DirectionNS::down, DirectionNS::left
    };

    auto index = [&](int x, int y) { return (y - y0) * w + (x - x0); };
    via[index(from.x, from.y)] = 4;
    queue.push_back(from);

    while (!queue.empty()) {
        Coord c = queue.front();
        queue.pop_front();
        const int8_t first = via[index(c.x, c.y)];

        for (int i = 0; i < 4; ++i) {
            const int nx = (int)c.x + dx[i];
            const int ny = (int)c.y + dy[i];
            if (nx < x0 || ny < y0 || nx > x1 || ny > y1) continue;
            if (via[index(nx, ny)] != -1) continue;

            Coord n { (uint32_t)nx, (uint32_t)ny };
            const int8_t step = first == 4 ? (int8_t)i : first;
            if (n == to) return dirs[step];
            if (blocked(n)) continue;

            via[index(nx, ny)] = step;
            queue.push_back(n);
        }
    }
    return DirectionNS::none;
}

}
