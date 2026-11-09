#ifndef MASK_HPP
#define MASK_HPP

#include "coord.hpp"
#include "gaze.hpp"
#include <vector>
#include <string>

class Level;

namespace VisionNS {

// base sight in unlit streets; HIV shrinks it (see Disease::visionDistance)
constexpr uint32_t VISION_DISTANCE = 12;

class Mask {
public:
    Mask(Level* level);
    virtual ~Mask() = default;

    void enable();
    void init();
    bool visible(const Coord& c) const;

    bool enabled = false;
    // random sideways glances (tests turn them off)
    bool glances = true;
    std::vector<std::string> mask;
    Level* level;

private:
    // where the eyes point this turn
    Vec gazeDirection(const Vec& walk);

    uint32_t glanceTurns = 0;
    float glanceAngle = 0.f;
};

}

#endif
