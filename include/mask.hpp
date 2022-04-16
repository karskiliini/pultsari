#ifndef MASK_HPP
#define MASK_HPP

#include "coord.hpp"
#include <vector>
#include <string>

class Level;

namespace VisionNS {

constexpr uint32_t VISION_DISTANCE = 12;

class Mask {
public:
    Mask(Level* level);
    virtual ~Mask() = default;

    void enable();
    void init();
    bool visible(const Coord& c) const;

    bool enabled = false;
    std::vector<std::string> mask;
    Level* level;
};

}

#endif
