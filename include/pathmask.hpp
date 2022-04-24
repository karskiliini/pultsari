#ifndef PATHMASK_HPP
#define PATHMASK_HPP

#include "coord.hpp"
#include "direction.hpp"
#include <vector>
#include <string>

class Level;

namespace PathNS {

class PathMask {
public:
    PathMask(Level* level);
    virtual ~PathMask() = default;
    uint32_t& at(const Coord& c);
    bool findPath(const Coord& coord, const Coord& target);
    DirectionNS::Direction traceBack(const Coord& c);

    std::vector<std::vector<uint32_t>> mask;
    Level* level;
    bool skipCop = false;
    bool skipVaras = false;
    bool skipConvict = false;
};

}

#endif
