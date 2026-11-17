#ifndef TRENCHES_HPP
#define TRENCHES_HPP

// The front's trenches: a layer of ground over the whole map. Our own line zigzags north to
// south near the rear (the left edge), communication trenches run back to the rear, the
// Ukrainian line lies to the east, and no man's land between them is shell craters and wire.
// A man down in a trench is out of the riflemen's sight; the FPV drones find him anyway.

#include "coord.hpp"
#include <cstdint>

class Level;

enum class Trench : uint8_t { None, Own, Enemy, Crater, Wire };

namespace TrenchNS {
    constexpr uint32_t OWN_X = 12;      // our main line wanders around this column
    constexpr uint32_t ENEMY_X = 70;    // and theirs around this one
    constexpr uint32_t FRONT_W = 90;    // where the fighting is; east of this is their rear
}

// dig a fresh trench system for a war level
void generateTrenches(Level& level);

// the trench cell of `kind` nearest to `c` (c itself if there is none)
Coord nearestTrench(const Level& level, const Coord& c, Trench kind);

// a free cell of `kind` within rx/ry of `near`, else anywhere of that kind; `near` if none at all
Coord randomTrenchCell(const Level& level, Trench kind, const Coord& near, uint32_t rx, uint32_t ry);

// the easternmost cell of our own line on row y (bays and dead ends included), -1 if none
int ownTrenchEastX(const Level& level, uint32_t y);

#endif
