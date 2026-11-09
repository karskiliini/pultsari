#ifndef FSB_HPP
#define FSB_HPP

#include "coord.hpp"
#include <cstdint>
#include <vector>

class Car;

// How far the player has walked down the carriageway. Crossing a street is fine;
// strolling down the middle of Tverskaja for a kilometre is not: the longer the
// run, the likelier the FSB sends its black cars to collect a new recruit.
struct RoadRun {
    static constexpr uint32_t SAFE = 25;     // this many road tiles are always safe
    static constexpr uint32_t WARN = 35;     // a black Aurus starts crawling alongside
    static constexpr uint32_t CERTAIN = 80;  // by now the cars have almost surely come
    static constexpr uint32_t DECAY = 4;     // each tile off the road forgives this much

    uint32_t tiles = 0;

    // one tile walked, on the road or off it
    void step(bool onRoad);
    // the chance the FSB is sent for this tile of road (0..1)
    float dispatchChance() const;
};

// The FSB watching the streets: first a black car tails the player, then two
// come at full speed, stop beside him and the men inside drag him to the front.
struct FsbPatrol {
    enum class Stage { Idle, Tailing, Dispatched, Grabbing };
    Stage stage = Stage::Idle;
    RoadRun run;
    Coord lastPos;
    bool havePos = false;
    bool radioHeard = false;      // the second warning, while tailing
    std::vector<Car*> cars;       // checked against the level's vehicles before use
    uint32_t timer = 0;           // world ticks left in this stage
    uint32_t cooldown = 0;        // world ticks before the FSB bothers again
};

#endif
