#include "gaze.hpp"
#include <algorithm>
#include <cmath>

namespace VisionNS {

namespace {

constexpr float PI = 3.14159265f;

float lerp(float from, float to, float t)
{
    return from + (to - from) * t;
}

// value falls linearly from 'high' at angle a0 to 'low' at angle a1
float falloff(float angle, float a0, float a1, float high, float low)
{
    if (angle <= a0) return high;
    if (angle >= a1) return low;
    return lerp(high, low, (angle - a0) / (a1 - a0));
}

}

void Heading::step(int dx, int dy)
{
    if (dx == 0 && dy == 0) return;

    Vec n { v.x * (1.f - HEADING_WEIGHT) + dx * HEADING_WEIGHT,
            v.y * (1.f - HEADING_WEIGHT) + dy * HEADING_WEIGHT };

    // exact reversal can cancel out: then simply turn around
    float len = std::sqrt(n.x * n.x + n.y * n.y);
    if (len < 0.001f) {
        v = { (float)dx, (float)dy };
        return;
    }
    v = n;
}

void Heading::reset()
{
    v = { 1.f, 0.f };
}

float Heading::degrees() const
{
    return std::atan2(-v.y, v.x) * 180.f / PI;
}

float angleBetween(const Vec& a, const Vec& b)
{
    float la = std::sqrt(a.x * a.x + a.y * a.y);
    float lb = std::sqrt(b.x * b.x + b.y * b.y);
    if (la < 0.0001f || lb < 0.0001f) return 0.f;

    float c = (a.x * b.x + a.y * b.y) / (la * lb);
    c = std::max(-1.f, std::min(1.f, c));
    return std::acos(c) * 180.f / PI;
}

Vec rotate(const Vec& v, float degrees)
{
    // screen y points down, so negate the angle to rotate counter-clockwise
    float r = -degrees * PI / 180.f;
    float c = std::cos(r);
    float s = std::sin(r);
    return { v.x * c - v.y * s, v.x * s + v.y * c };
}

Vec turnTowards(const Vec& from, const Vec& to, float maxDegrees)
{
    const float lf = std::sqrt(from.x * from.x + from.y * from.y);
    const float lt = std::sqrt(to.x * to.x + to.y * to.y);
    if (lt < 0.0001f) return lf < 0.0001f ? Vec{ 1.f, 0.f } : Vec{ from.x / lf, from.y / lf };
    if (lf < 0.0001f) return { to.x / lt, to.y / lt };
    const Vec f { from.x / lf, from.y / lf };

    // angles counter-clockwise as seen on screen (y grows downwards)
    float diff = (std::atan2(-to.y, to.x) - std::atan2(-f.y, f.x)) * 180.f / PI;
    while (diff > 180.f) diff -= 360.f;
    while (diff <= -180.f) diff += 360.f;
    if (diff < -179.9f) diff = 180.f; // an about-turn, give or take rounding: counter-clockwise
    if (std::fabs(diff) <= maxDegrees) return { to.x / lt, to.y / lt };
    return rotate(f, diff > 0.f ? maxDegrees : -maxDegrees);
}

float coneScale(unsigned promilles)
{
    if (promilles <= SOBER_PROMILLES) return 1.f;
    if (promilles >= DRUNK_PROMILLES) return DRUNK_CONE_SCALE;
    float t = (float)(promilles - SOBER_PROMILLES) / (float)(DRUNK_PROMILLES - SOBER_PROMILLES);
    return lerp(1.f, DRUNK_CONE_SCALE, t);
}

float visionRange(float offGaze, float offWalk, float scale)
{
    // where the eyes point: long and narrow, fading towards the sides
    float g1 = 20.f * scale, g2 = 60.f * scale, g3 = 100.f * scale;
    float gaze = (offGaze <= g2)
        ? falloff(offGaze, g1, g2, FOCUS_RANGE, 9.f)
        : falloff(offGaze, g2, g3, 9.f, NEAR_RANGE);

    // corner of the eye towards the walking direction
    float walk = falloff(offWalk, 45.f * scale, 90.f * scale, WALK_RANGE, NEAR_RANGE);

    return std::max({ gaze, walk, NEAR_RANGE });
}

}
