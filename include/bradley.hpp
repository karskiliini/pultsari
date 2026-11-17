#ifndef BRADLEY_HPP
#define BRADLEY_HPP

// A Bradley fighting vehicle in the assault on the third front level: it drives in from the
// east, stops a short way off our trench, lets out its infantry and works the line over with
// its 25 mm cannon (fragmentation rounds). A grenade beside it knocks it out.

#include "vehicle.hpp"

class Bradley : public Vehicle {
public:
    Bradley(const Coord& front, uint32_t stopX);
    virtual void update();
    virtual std::string name() const { return "Bradley"; }
    virtual std::vector<Coord> cells() const;
    virtual std::string glyphAt(uint32_t index) const;
    virtual const char* colorAt(uint32_t index) const;

    uint32_t stopX;             // where it halts: a few cells east of our trench
    bool unloaded = false;
    uint32_t gunCooldown = 3;
};

// where a Bradley coming in on row y stops: 6..10 cells east of our trench's lip
uint32_t bradleyStopX(const Level& level, uint32_t y);

#endif
