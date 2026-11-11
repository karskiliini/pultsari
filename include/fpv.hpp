#ifndef FPV_HPP
#define FPV_HPP

// FPV suicide drones: small quadcopters with a grenade strapped on. At the front the
// sky is full of them. Each one picks a Russian soldier out in the open, chases him,
// and when it is close enough locks on and dives in. Soldiers shoot at them (see
// antiDroneFire); the ones that miss hit whoever stands in the way.

#include "coord.hpp"
#include <cstdint>

class Level;
class Person;
struct Drone;

constexpr uint32_t FPV_MAX = 5;          // at most this many in the air at once
constexpr float FPV_SPEED = 8.0f;        // cells per second while hunting
constexpr float FPV_DIVE_SPEED = 9.0f;   // cells per second in the final dive: a quick drunk can still step aside
constexpr float FPV_DIVE_RANGE = 5.0f;   // locks on and dives from this close

// a Russian soldier, as the drone pilots see it: soldiers, air defence, the conscripts
// of the meat assaults at the front, and the player once he is called up
bool isRussianSoldier(const Level& level, const Person* p);

// the soldier out under the open sky the drone at (x, y) goes for, or null if there is none
Person* fpvPickPrey(const Level& level, float x, float y);

// a new FPV drone at (x, y) heading for `prey` (picked on the way if null)
void launchFpv(Level& level, float x, float y, Person* prey);

// world tick: at the front new drones come in from the Ukrainian side, and all of them whine
void fpvTick(Level& level);

// one frame of flight; true when it has hit and should blow up at `blastAt`
bool fpvFrame(Level& level, Drone& d, float dt, Coord& blastAt);

// the grenade goes off: deadly where it lands, a few cells of shrapnel around it
void fpvBlast(Level& level, const Coord& at);

#endif
