#include "tracer.hpp"
#include "common.hpp"
#include "level.hpp"
#include <cmath>
#include <cstdlib>

namespace {
constexpr size_t MAX_TRACERS = 48;   // a firefight never needs more; the oldest go first
}

bool tracerDot(const Tracer& t, uint32_t k, float& x, float& y)
{
    if (t.delayMs > 0 || t.len <= 0) return false;
    const float d = t.head - k * TRACER_GAP;
    if (d < 0 || d > t.len) return false;    // not out of the barrel yet, or already arrived
    x = t.x0 + (t.x1 - t.x0) * d / t.len;
    y = t.y0 + (t.y1 - t.y0) * d / t.len;
    return true;
}

uint8_t tracerColour(uint32_t k, TracerKind kind)
{
    static const uint8_t bullet[TRACER_DOTS] = { 231, 228, 214, 202, 88 };
    static const uint8_t cannon[TRACER_DOTS] = { 230, 221, 208, 166, 94 };
    if (k >= TRACER_DOTS) return 0;
    return kind == TracerKind::Cannon ? cannon[k] : bullet[k];
}

void Level::addTracer(float x0, float y0, float x1, float y1, TracerKind kind, uint32_t delayMs)
{
    // turn by turn a streak would hang in the air from one move to the next
    if (!common::animsEnabled || !common::realtimeEnabled) return;
    const float len = std::hypot(x1 - x0, y1 - y0);
    if (len < 0.5f) return;
    if (tracers.size() >= MAX_TRACERS) tracers.erase(tracers.begin());
    Tracer t;
    t.x0 = x0; t.y0 = y0; t.x1 = x1; t.y1 = y1;
    t.len = len;
    t.speed = kind == TracerKind::Cannon ? CANNON_TRACER_SPEED : TRACER_SPEED;
    t.delayMs = (int32_t)delayMs;
    t.kind = kind;
    tracers.push_back(t);
    common::tracerEvent(x0, y0, x1, y1, (int)kind, delayMs);
}

void Level::addTracer(const Coord& from, const Coord& to, bool hit, TracerKind kind, uint32_t delayMs)
{
    const float x0 = from.x, y0 = from.y;
    float x1 = to.x, y1 = to.y;
    if (!hit) {
        // a miss flies on past him, a little to one side
        const float dx = x1 - x0, dy = y1 - y0;
        const float len = std::hypot(dx, dy);
        if (len < 0.5f) return;
        const float on = 6 + std::rand() % 5, side = ((std::rand() % 15) - 7) / 10.0f;
        const float ux = dx / len, uy = dy / len;
        x1 += ux * on - uy * side;
        y1 += uy * on + ux * side;
    }
    addTracer(x0, y0, x1, y1, kind, delayMs);
}

void Level::tracerFrame(uint32_t ms)
{
    for (size_t i = 0; i < tracers.size();) {
        Tracer& t = tracers[i];
        int32_t flyMs = (int32_t)ms;
        if (t.delayMs > 0) {
            // whatever is left of the frame after the wait is spent flying
            flyMs = t.delayMs >= (int32_t)ms ? 0 : (int32_t)ms - t.delayMs;
            t.delayMs = t.delayMs >= (int32_t)ms ? t.delayMs - (int32_t)ms : 0;
            if (t.delayMs == 0 && flyMs == 0) flyMs = 1;
        }
        t.head += t.speed * flyMs / 1000.0f;
        if (t.head - (TRACER_DOTS - 1) * TRACER_GAP > t.len) tracers.erase(tracers.begin() + i);
        else ++i;
    }
}
