#ifndef GAZE_HPP
#define GAZE_HPP

namespace VisionNS {

// How much the newest step weighs in the walking direction (the rest is
// the earlier direction). 0.3 gives:
//   d w d w d w ...  -> about 45 degrees (north-east)
//   d d d d d w      -> about 23 degrees (still mostly right)
//   d d d d d w w    -> about 46 degrees
constexpr float HEADING_WEIGHT = 0.3f;

// Vision ranges in cells.
constexpr float NEAR_RANGE = 2.5f;      // seen all around, also behind
constexpr float FOCUS_RANGE = 30.f;     // straight where the eyes point
constexpr float WALK_RANGE = 10.f;      // peripheral vision ahead while walking
// Ranges above are for lit places; in the dark everything is seen this much
// shorter (but never below NEAR_RANGE).
constexpr float DARK_SCALE = 0.5f;

// The player stares at the nearest person within this angle of the walking
// direction (90 degrees each side = the 180 degree half in front).
constexpr float FOCUS_SEARCH_ANGLE = 90.f;

// Glancing sideways, chance per turn in percent.
constexpr unsigned GLANCE_CHANCE = 25;
constexpr float GLANCE_MIN_ANGLE = 60.f;
constexpr float GLANCE_MAX_ANGLE = 120.f;

// Drunkenness narrows the vision cone. Below SOBER_PROMILLES the cone is
// full width, at DRUNK_PROMILLES and above its angles are DRUNK_CONE_SCALE
// of normal (tunnel vision).
constexpr unsigned SOBER_PROMILLES = 5;
constexpr unsigned DRUNK_PROMILLES = 30;
constexpr float DRUNK_CONE_SCALE = 0.35f;

struct Vec {
    float x = 0.f;
    float y = 0.f;
};

// Weighted average of the steps walked. Screen y grows downwards.
class Heading {
public:
    void step(int dx, int dy);
    void reset();

    Vec dir() const { return v; }
    // degrees counter-clockwise from right, as seen on screen (up = 90)
    float degrees() const;

private:
    Vec v { 1.f, 0.f };
};

// Angle between two vectors in degrees, 0..180.
float angleBetween(const Vec& a, const Vec& b);

// Rotate a vector by given degrees (positive = counter-clockwise on screen).
Vec rotate(const Vec& v, float degrees);

// Cone width multiplier for given promilles, 1.0 = sober.
float coneScale(unsigned promilles);

// How far a cell can be seen. offGaze / offWalk are the cell's angles from
// where the eyes point and from the walking direction. scale narrows the
// cone (see coneScale).
float visionRange(float offGaze, float offWalk, float scale = 1.f);

}

#endif
