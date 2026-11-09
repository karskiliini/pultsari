#ifndef PATHMASK_HPP
#define PATHMASK_HPP

#include "coord.hpp"
#include "direction.hpp"

class Level;

namespace PathNS {

class PathMask {
public:
    explicit PathMask(Level* level) : level(level) { }

    // first step from `from` towards `to`, or none when no route exists nearby
    DirectionNS::Direction firstStep(const Coord& from, const Coord& to) const;

    Level* level;
    bool skipCop = false;
    bool skipVaras = false;
    bool skipConvict = false;

private:
    bool blocked(const Coord& c) const;
};

}

#endif
