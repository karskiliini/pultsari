#include "trenches.hpp"
#include "common.hpp"
#include "level.hpp"
#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace TrenchNS;

namespace {

int rnd(int lo, int hi) { return lo + std::rand() % (hi - lo + 1); }

bool onMap(int x, int y) { return x >= 0 && y >= 0 && x < (int)common::SIZEX && y < (int)common::SIZEY; }

// digs a cell; a trench is never filled back in by a crater or wire
void dig(Level& l, int x, int y, Trench t)
{
    if (!onMap(x, y)) return;
    const Trench old = l.trenchAt(Coord{ (uint32_t)x, (uint32_t)y });
    if ((t == Trench::Crater || t == Trench::Wire) && old != Trench::None) return;
    l.setTrench(Coord{ (uint32_t)x, (uint32_t)y }, t);
}

void digRect(Level& l, int x0, int y0, int x1, int y1, Trench t)
{
    for (int y = std::min(y0, y1); y <= std::max(y0, y1); ++y)
        for (int x = std::min(x0, x1); x <= std::max(x0, x1); ++x) dig(l, x, y, t);
}

// A line north to south, three cells wide, zigzagging around `centre`: every few rows it
// steps sideways (a traverse, so a shell or a burst never sweeps the whole line). Returns
// the centre column of each row.
std::vector<int> digLine(Level& l, int centre, int swing, Trench t)
{
    std::vector<int> cx(common::SIZEY, centre);
    int x = centre + rnd(-swing, swing);
    int y = 0;
    while (y < (int)common::SIZEY) {
        const int len = rnd(4, 8);
        for (int k = 0; k < len && y < (int)common::SIZEY; ++k, ++y) {
            cx[y] = x;
            digRect(l, x - 1, y, x + 1, y, t);
        }
        const int next = std::max(centre - swing, std::min(centre + swing, x + rnd(-3, 3)));
        // the traverse: dig across from the old column to the new on the last row
        if (y > 0) digRect(l, x, y - 1, next, y - 1, t);
        x = next;
    }
    return cx;
}

// a winding communication trench, two cells wide, from (x, y) west to the map's edge
void digCommunication(Level& l, int x, int y)
{
    int run = rnd(4, 8);
    while (x >= 0) {
        digRect(l, x, y, x, y + 1, Trench::Own);
        --x;
        if (--run == 0) {
            run = rnd(5, 9);
            const int ny = std::max(1, std::min((int)common::SIZEY - 3, y + rnd(-2, 2)));
            digRect(l, x + 1, y, x + 1, ny + 1, Trench::Own);
            y = ny;
        }
    }
}

} // namespace

Trench Level::trenchAt(const Coord& c) const
{
    if (trench.empty() || c.x >= common::SIZEX || c.y >= common::SIZEY) return Trench::None;
    return (Trench)trench[c.y * common::SIZEX + c.x];
}

void Level::setTrench(const Coord& c, Trench t)
{
    if (c.x >= common::SIZEX || c.y >= common::SIZEY) return;
    if (trench.empty()) trench.assign(common::SIZEX * common::SIZEY, (uint8_t)Trench::None);
    trench[c.y * common::SIZEX + c.x] = (uint8_t)t;
}

void generateTrenches(Level& l)
{
    l.ownLineX.clear();
    l.trench.assign(common::SIZEX * common::SIZEY, (uint8_t)Trench::None);

    // our main line
    const std::vector<int> own = digLine(l, OWN_X, 3, Trench::Own);
    l.ownLineX = own;

    // firing bays out to the east every so often
    for (int y = rnd(3, 10); y < (int)common::SIZEY - 2; y += rnd(10, 20))
        digRect(l, own[y] + 2, y, own[y] + 3, y + 1, Trench::Own);

    // dead ends pushed out into no man's land: listening posts
    for (int n = rnd(1, 3); n > 0; --n) {
        const int y = rnd(5, common::SIZEY - 7);
        digRect(l, own[y] + 2, y, own[y] + 2 + rnd(5, 12), y + 1, Trench::Own);
    }

    // communication trenches back to the rear: one always near where a new man arrives
    const int startY = std::max(2, std::min((int)common::SIZEY - 4, (int)common::PLAYER_START_Y + rnd(-6, 6)));
    digCommunication(l, own[startY], startY);
    for (int n = rnd(1, 3); n > 0; --n) {
        const int y = rnd(4, common::SIZEY - 6);
        digCommunication(l, own[y], y);
    }

    // their line, and their bays
    const std::vector<int> enemy = digLine(l, ENEMY_X, 4, Trench::Enemy);
    for (int y = rnd(3, 10); y < (int)common::SIZEY - 2; y += rnd(8, 16))
        digRect(l, enemy[y] - 3, y, enemy[y] - 2, y + 1, Trench::Enemy);

    // no man's land: shell craters and coils of wire
    for (int n = rnd(20, 40); n > 0; --n) {
        const int x = rnd(20, 58), y = rnd(1, common::SIZEY - 2);
        dig(l, x, y, Trench::Crater);
        if (std::rand() % 2) { dig(l, x + 1, y, Trench::Crater); dig(l, x, y + 1, Trench::Crater); }
    }
    for (int n = rnd(2, 4); n > 0; --n) {
        const int x = rnd(25, 55), y = rnd(0, common::SIZEY - 16);
        for (int k = rnd(6, 15); k > 0; --k) dig(l, x + (k % 3 == 0 ? 1 : 0), y + k, Trench::Wire);
    }
}

Coord nearestTrench(const Level& l, const Coord& c, Trench kind)
{
    for (int r = 0; r < 60; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::abs(dx) != r && std::abs(dy) != r) continue;
                const int x = (int)c.x + dx, y = (int)c.y + dy;
                if (!onMap(x, y)) continue;
                const Coord t { (uint32_t)x, (uint32_t)y };
                if (l.trenchAt(t) == kind && !l.hit(t)) return t;
            }
        }
    }
    return c;
}

Coord randomTrenchCell(const Level& l, Trench kind, const Coord& near, uint32_t rx, uint32_t ry)
{
    for (int tries = 0; tries < 200; ++tries) {
        const int x = (int)near.x + rnd(-(int)rx, (int)rx), y = (int)near.y + rnd(-(int)ry, (int)ry);
        if (!onMap(x, y)) continue;
        const Coord c { (uint32_t)x, (uint32_t)y };
        if (l.trenchAt(c) == kind && !l.hit(c)) return c;
    }
    return nearestTrench(l, near, kind);
}

bool Level::behindTheLine(const Coord& c) const
{
    if (c.y >= ownLineX.size()) return false;
    return (int)c.x < ownLineX[c.y] - 3;
}

int ownTrenchEastX(const Level& l, uint32_t y)
{
    if (y >= common::SIZEY) return -1;
    for (int x = (int)FRONT_W / 2; x >= 0; --x)
        if (l.inOwnTrench(Coord{ (uint32_t)x, y })) return x;
    return -1;
}
