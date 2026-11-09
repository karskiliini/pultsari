#ifndef PATROL_HPP
#define PATROL_HPP

#include "coord.hpp"
#include <cstdint>

class Level;

// Militsija on the streets: placing patrols on the pavements and in the alleys
// between the blocks, and sending new ones when too few are left.
namespace Patrol {

// a free cell on the pavement right beside a carriageway; (0, 0) if none was found
Coord pavementSpot(const Level& level);
// a free cell in a narrow gap between two buildings; (0, 0) if none was found
Coord alleySpot(const Level& level);
// is this cell on a pavement, or in an alley?
bool onPavement(const Level& level, const Coord& c);
bool inAlley(const Level& level, const Coord& c);

// cops kept just beyond the edges of the drunk's screen
constexpr uint32_t LOCAL_COPS = 8;
// how many cops a Moscow level keeps on its streets (more towards the Kremlin)
uint32_t wantedCops(uint32_t moscowStage);
// the level's first patrols: half on the pavements, half in the alleys
void populateCops(Level& level, uint32_t moscowStage);
// world tick: when cops have been killed or gone, a new patrol walks in out of sight
void patrolTick(Level& level);

}

#endif
