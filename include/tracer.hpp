#ifndef TRACER_HPP
#define TRACER_HPP

// Tracers: every shot fired leaves a streak of light racing from the muzzle, a few tiny dots
// fading from white-hot to dark red. Pure looks: who is hit is settled by the shooter as
// before. World coordinates are cell centres (a cell's index is its centre).

#include <cstdint>

enum class TracerKind : uint8_t { Bullet, Cannon };

struct Tracer {
    float x0, y0, x1, y1;   // muzzle and where the round stops
    float len;              // cells from one to the other
    float head = 0;         // how far the white-hot head has flown
    float speed;            // cells per second
    int32_t delayMs = 0;    // the later rounds of a burst wait their turn
    TracerKind kind = TracerKind::Bullet;
};

constexpr float TRACER_SPEED = 60.0f;        // cells per second: three cells a frame
constexpr float CANNON_TRACER_SPEED = 45.0f;
constexpr uint32_t TRACER_DOTS = 5;          // head first
constexpr float TRACER_GAP = 0.45f;          // cells between dots

// where dot k (0 = the head) is now; false while it is not in the air
bool tracerDot(const Tracer& t, uint32_t k, float& x, float& y);

// the dot's xterm-256 colour: white, yellow, orange, red, dark red
uint8_t tracerColour(uint32_t k, TracerKind kind);

#endif
