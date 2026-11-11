#ifndef MASK_HPP
#define MASK_HPP

#include "coord.hpp"
#include "gaze.hpp"
#include <chrono>
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
    // how far the field of vision may turn per init(), degrees; 180 = at once
    // (see TURN_PER_STEP / TURN_PER_FRAME)
    float maxTurn = 180.f;
    std::vector<std::string> mask;
    Level* level;

private:
    // where the eyes point this turn
    Vec gazeDirection(const Vec& walk);

    // whether a sideways glance may be decided now (at most every GLANCE_DECIDE_MS)
    bool glanceTick();

    uint32_t glanceTurns = 0;
    float glanceAngle = 0.f;
    std::chrono::steady_clock::time_point lastGlanceTick {};

    // where the field of vision points now; it turns towards the walking
    // direction and the gaze at most maxTurn degrees per update
    bool aimed = false;
    Vec shownWalk { 1.f, 0.f };
    Vec shownGaze { 1.f, 0.f };
};

}

#endif
